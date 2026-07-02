package main

import (
	"encoding/binary"
	"flag"
	"fmt"
	"image/color"
	"io"
	"math"
	"net"
	"os"
	"sync"
	"time"

	"github.com/hajimehoshi/ebiten/v2"
	"github.com/hajimehoshi/ebiten/v2/ebitenutil"
	"github.com/hajimehoshi/ebiten/v2/vector"
)

// 입력 키 비트 (서버 Player::Update 와 동일)
//   0x01 up / 0x02 down / 0x04 left / 0x08 right
const (
	keyUp    = 1 << 0
	keyDown  = 1 << 1
	keyLeft  = 1 << 2
	keyRight = 1 << 3
)

// PacketId — 반드시 서버 cppServer/Protocol.h 의 enum 과 값이 일치해야 한다.
const (
	pktConnect      = 0
	pktDisconnect   = 1
	pktJoin         = 2
	pktLeave        = 3
	pktMovePlayer   = 4
	pktMoveBullet   = 5
	pktAttack       = 6
	pktRemovePlayer = 7
	pktRemoveBullet = 8
	pktHidePlayer   = 9
	pktHideBullet   = 10
	pktChat         = 11
)

// 서버 월드는 1000x1000 (World.h 의 W, Y). 화면을 월드와 1:1로 맞추면
// 마우스 좌표 = 월드 좌표가 되어 사격 방향 계산이 정확해진다.
const (
	worldW  = 1000
	worldH  = 1000
	screenW = worldW
	screenH = worldH
)

type PlayerView struct{ x, y float32 }
type BulletView struct{ x, y float32 }

type Game struct {
	conn net.Conn

	mu      sync.Mutex
	players map[int32]*PlayerView
	bullets map[int32]*BulletView

	lastKeys     byte
	prevMouse    bool
	disconnected bool
	errText      string
}

func (g *Game) Update() error {
	if g.disconnected {
		return nil
	}

	var keys byte
	if ebiten.IsKeyPressed(ebiten.KeyW) {
		keys |= keyUp
	}
	if ebiten.IsKeyPressed(ebiten.KeyS) {
		keys |= keyDown
	}
	if ebiten.IsKeyPressed(ebiten.KeyA) {
		keys |= keyLeft
	}
	if ebiten.IsKeyPressed(ebiten.KeyD) {
		keys |= keyRight
	}
	if keys != g.lastKeys {
		g.sendMove(keys)
		g.lastKeys = keys
	}

	mouseDown := ebiten.IsMouseButtonPressed(ebiten.MouseButtonLeft)
	if mouseDown && !g.prevMouse {
		g.sendAttack()
	}
	g.prevMouse = mouseDown
	return nil
}

func (g *Game) Draw(screen *ebiten.Image) {
	screen.Fill(color.RGBA{245, 246, 248, 255})
	drawGrid(screen)

	g.mu.Lock()
	for id, p := range g.players {
		vector.DrawFilledCircle(screen, p.x, p.y, 13, color.RGBA{20, 24, 31, 255}, true)
		vector.DrawFilledCircle(screen, p.x, p.y, 10, colorFor(id), true)
	}
	for _, b := range g.bullets {
		vector.DrawFilledCircle(screen, b.x, b.y, 5, color.RGBA{20, 24, 31, 255}, true)
		vector.DrawFilledCircle(screen, b.x, b.y, 4, color.RGBA{250, 205, 70, 255}, true)
	}
	nPlayers, nBullets := len(g.players), len(g.bullets)
	disconnected, errText := g.disconnected, g.errText
	g.mu.Unlock()

	status := fmt.Sprintf("WASD 이동 | 좌클릭 사격 | players: %d | bullets: %d", nPlayers, nBullets)
	if disconnected {
		status = "disconnected"
		if errText != "" {
			status += ": " + errText
		}
	}
	ebitenutil.DebugPrintAt(screen, status, 12, 12)
	if nPlayers == 0 && !disconnected {
		ebitenutil.DebugPrintAt(screen, "대기 중... 서버는 슬롯 4개가 모두 차야 매치가 시작됩니다 (클라 4개 접속 필요)", 12, 32)
	}
}

func (g *Game) Layout(int, int) (int, int) { return screenW, screenH }

// ---- 수신 ----

func (g *Game) receiveLoop() {
	header := make([]byte, 4)
	for {
		if _, err := io.ReadFull(g.conn, header); err != nil {
			g.setDisconnected(err)
			return
		}
		size := binary.LittleEndian.Uint16(header[0:2])
		id := binary.LittleEndian.Uint16(header[2:4])
		if size < 4 || size > 4096 {
			g.setDisconnected(fmt.Errorf("bad packet size %d", size))
			return
		}
		body := make([]byte, int(size)-4)
		if _, err := io.ReadFull(g.conn, body); err != nil {
			g.setDisconnected(err)
			return
		}
		g.handlePacket(id, body)
	}
}

func (g *Game) handlePacket(id uint16, body []byte) {
	g.mu.Lock()
	defer g.mu.Unlock()

	switch id {
	case pktJoin:
		// IdPacket{ int id } — 새 플레이어 등장. 위치는 이후 이동 패킷으로 채워진다.
		if pid, ok := readID(body); ok {
			if _, exists := g.players[pid]; !exists {
				g.players[pid] = &PlayerView{x: worldW / 2, y: worldH / 2}
			}
		}

	case pktLeave:
		if pid, ok := readID(body); ok {
			delete(g.players, pid)
		}

	// 플레이어 관련. 서버 버그로 위치 갱신이 RemovePlayer id 로 오므로
	// body 길이로 구분한다: 12바이트(Vec2Packet)면 이동, 4바이트(IdPacket)면 제거.
	case pktMovePlayer, pktRemovePlayer, pktHidePlayer:
		if oid, x, y, ok := readVec2(body); ok {
			p := g.players[oid]
			if p == nil {
				p = &PlayerView{}
				g.players[oid] = p
			}
			p.x, p.y = x, y
		} else if pid, ok := readID(body); ok {
			delete(g.players, pid)
		}

	// 총알 관련. 마찬가지로 12바이트면 이동, 4바이트면 제거.
	case pktMoveBullet, pktRemoveBullet, pktHideBullet:
		if oid, x, y, ok := readVec2(body); ok {
			b := g.bullets[oid]
			if b == nil {
				b = &BulletView{}
				g.bullets[oid] = b
			}
			b.x, b.y = x, y
		} else if bid, ok := readID(body); ok {
			delete(g.bullets, bid)
		}

	case pktAttack:
		// AttackPacket 을 서버가 브로드캐스트하게 되면 여기서 총알 생성 처리.
		if oid, x, y, ok := readVec2(body); ok {
			g.bullets[oid] = &BulletView{x: x, y: y}
		}
	}
}

// IdPacket body: int32 id (4바이트)
func readID(body []byte) (int32, bool) {
	if len(body) < 4 {
		return 0, false
	}
	return int32(binary.LittleEndian.Uint32(body[0:4])), true
}

// Vec2Packet body: int32 id, float32 x, float32 y (12바이트)
func readVec2(body []byte) (id int32, x, y float32, ok bool) {
	if len(body) < 12 {
		return 0, 0, 0, false
	}
	id = int32(binary.LittleEndian.Uint32(body[0:4]))
	x = math.Float32frombits(binary.LittleEndian.Uint32(body[4:8]))
	y = math.Float32frombits(binary.LittleEndian.Uint32(body[8:12]))
	return id, x, y, true
}

// ---- 송신 ----

func (g *Game) sendMove(keys byte) {
	g.writePacket(pktMovePlayer, []byte{keys})
}

func (g *Game) sendAttack() {
	mx, my := ebiten.CursorPosition()
	body := make([]byte, 8)
	binary.LittleEndian.PutUint32(body[0:4], math.Float32bits(float32(mx)))
	binary.LittleEndian.PutUint32(body[4:8], math.Float32bits(float32(my)))
	g.writePacket(pktAttack, body)
}

func (g *Game) writePacket(id uint16, body []byte) {
	buf := make([]byte, 4+len(body))
	binary.LittleEndian.PutUint16(buf[0:2], uint16(len(buf)))
	binary.LittleEndian.PutUint16(buf[2:4], id)
	copy(buf[4:], body)
	_, _ = g.conn.Write(buf)
}

func (g *Game) setDisconnected(err error) {
	g.mu.Lock()
	defer g.mu.Unlock()
	g.disconnected = true
	if err != nil {
		g.errText = err.Error()
	}
}

// ---- 그리기 유틸 ----

func drawGrid(screen *ebiten.Image) {
	gridColor := color.RGBA{225, 228, 234, 255}
	for x := float32(0); x <= screenW; x += 50 {
		vector.StrokeLine(screen, x, 0, x, screenH, 1, gridColor, true)
	}
	for y := float32(0); y <= screenH; y += 50 {
		vector.StrokeLine(screen, 0, y, screenW, y, 1, gridColor, true)
	}
}

func colorFor(id int32) color.Color {
	palette := []color.Color{
		color.RGBA{230, 80, 80, 255},
		color.RGBA{70, 135, 230, 255},
		color.RGBA{65, 175, 105, 255},
		color.RGBA{205, 155, 45, 255},
	}
	return palette[int(id)&3]
}

func main() {
	addr := flag.String("addr", "127.0.0.1:5050", "server address")
	flag.Parse()

	conn, err := net.DialTimeout("tcp", *addr, 3*time.Second)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	defer conn.Close()

	game := &Game{
		conn:    conn,
		players: make(map[int32]*PlayerView),
		bullets: make(map[int32]*BulletView),
	}

	// 서버는 accept 시점에 자동으로 Join 을 처리하므로(NetworkCore.cpp)
	// 클라에서 Join 을 또 보내면 슬롯을 2개 먹는다. 그래서 여기서는 보내지 않는다.
	go game.receiveLoop()

	// 창은 보기 편하게 축소, 렌더 해상도는 월드와 1:1(Layout)로 유지.
	ebiten.SetWindowSize(720, 720)
	ebiten.SetWindowTitle("cppServer test client")

	if err := ebiten.RunGame(game); err != nil {
		panic(err)
	}
}
