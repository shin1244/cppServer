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
	pktMapSnapshot  = 12
)

const (
	screenW = 720
	screenH = 720

	gridSize     = 100 // 서버 SpatialGrid 셀 크기와 동일 (내 주변 그리드 시각화)
	visionRadius = 280 // 전장의 안개: 이 반경 밖은 검게
)

type PlayerView struct{ x, y float32 }
type BulletView struct{ x, y float32 }
type Wall struct{ x, y, size float32 } // 월드 좌표 기준 좌상단 + 한 변 길이

type Game struct {
	conn net.Conn

	mu      sync.Mutex
	players map[int32]*PlayerView
	bullets map[int32]*BulletView
	walls   []Wall

	myID         int32
	camX, camY   float32 // 카메라 좌상단(월드 좌표)
	lastKeys     byte
	prevMouse    bool
	disconnected bool
	errText      string

	fog *ebiten.Image // 중앙에 원형 구멍이 뚫린 검은 오버레이 (정적)
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

	g.mu.Lock()
	// 카메라: 내 플레이어를 화면 중앙에 둔다. 아직 내 위치를 모르면 직전 값을 유지.
	if me, ok := g.players[g.myID]; ok {
		g.camX = me.x - screenW/2
		g.camY = me.y - screenH/2
	}
	camX, camY := g.camX, g.camY

	drawGrid(screen, camX, camY)

	// 벽 (정적 지형). 화면에 걸치는 것만 그린다.
	wallColor := color.RGBA{90, 98, 110, 255}
	for _, w := range g.walls {
		sx, sy := w.x-camX, w.y-camY
		if sx+w.size < 0 || sy+w.size < 0 || sx > screenW || sy > screenH {
			continue
		}
		vector.DrawFilledRect(screen, sx, sy, w.size, w.size, wallColor, false)
	}

	for id, p := range g.players {
		sx, sy := p.x-camX, p.y-camY
		vector.DrawFilledCircle(screen, sx, sy, 13, color.RGBA{20, 24, 31, 255}, true)
		vector.DrawFilledCircle(screen, sx, sy, 10, colorFor(id), true)
		if id == g.myID {
			// 내 캐릭터 표시용 링
			vector.StrokeCircle(screen, sx, sy, 17, 2, color.RGBA{20, 24, 31, 255}, true)
		}
	}
	for _, b := range g.bullets {
		sx, sy := b.x-camX, b.y-camY
		vector.DrawFilledCircle(screen, sx, sy, 5, color.RGBA{20, 24, 31, 255}, true)
		vector.DrawFilledCircle(screen, sx, sy, 4, color.RGBA{250, 205, 70, 255}, true)
	}

	nPlayers, nBullets := len(g.players), len(g.bullets)
	myID := g.myID
	disconnected, errText := g.disconnected, g.errText
	g.mu.Unlock()

	// 전장의 안개: 중앙 원형만 남기고 나머지를 검게. (플레이어가 항상 중앙이라 정적 이미지 사용)
	screen.DrawImage(g.fog, nil)

	status := fmt.Sprintf("WASD 이동 | 좌클릭 사격 | myID: %d | players: %d | bullets: %d", myID, nPlayers, nBullets)
	if disconnected {
		status = "disconnected"
		if errText != "" {
			status += ": " + errText
		}
	}
	ebitenutil.DebugPrintAt(screen, status, 12, 12)
	if _, ok := hasMe(g, myID); !ok && !disconnected {
		ebitenutil.DebugPrintAt(screen, "대기 중... 서버는 슬롯 4개가 모두 차야 매치가 시작됩니다", 12, 32)
	}
}

func hasMe(g *Game, myID int32) (struct{}, bool) {
	g.mu.Lock()
	_, ok := g.players[myID]
	g.mu.Unlock()
	return struct{}{}, ok
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
	case pktConnect:
		// 서버가 접속 클라에게 자기 id 를 알려주는 경우(현재 서버는 안 보냄, 향후 대비)
		if pid, ok := readID(body); ok && g.myID < 0 {
			g.myID = pid
		}

	case pktJoin:
		// IdPacket{ int id }. 접속 직후 받는 첫 Join = 내 슬롯 idx.
		if pid, ok := readID(body); ok {
			if g.myID < 0 {
				g.myID = pid
			}
			if _, exists := g.players[pid]; !exists {
				g.players[pid] = &PlayerView{}
			}
		}

	case pktLeave:
		if pid, ok := readID(body); ok {
			delete(g.players, pid)
		}

	// 플레이어 관련. 서버 버그로 위치 갱신이 RemovePlayer id 로 오므로
	// body 길이로 구분: 12바이트(Vec2Packet)=이동, 4바이트(IdPacket)=제거.
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

	// 총알 관련. 12바이트=이동, 4바이트=제거.
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
		if oid, x, y, ok := readVec2(body); ok {
			g.bullets[oid] = &BulletView{x: x, y: y}
		}

	case pktMapSnapshot:
		// body: cellSize(2) width(2) height(2) wallCount(2) 뒤에 wallCount×{x(2),y(2)}
		if len(body) < 8 {
			return
		}
		cellSize := float32(binary.LittleEndian.Uint16(body[0:2]))
		wallCount := int(binary.LittleEndian.Uint16(body[6:8]))
		off := 8
		walls := make([]Wall, 0, wallCount)
		for i := 0; i < wallCount && off+4 <= len(body); i++ {
			cx := float32(binary.LittleEndian.Uint16(body[off : off+2]))
			cy := float32(binary.LittleEndian.Uint16(body[off+2 : off+4]))
			off += 4
			walls = append(walls, Wall{x: cx * cellSize, y: cy * cellSize, size: cellSize})
		}
		g.walls = walls
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

// 마우스 클릭 지점을 월드 좌표로 변환해 사격 방향으로 보낸다(카메라 오프셋 보정).
func (g *Game) sendAttack() {
	mx, my := ebiten.CursorPosition()
	g.mu.Lock()
	worldX := float32(mx) + g.camX
	worldY := float32(my) + g.camY
	g.mu.Unlock()

	body := make([]byte, 8)
	binary.LittleEndian.PutUint32(body[0:4], math.Float32bits(worldX))
	binary.LittleEndian.PutUint32(body[4:8], math.Float32bits(worldY))
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

// 카메라 오프셋에 맞춰 월드 격자선을 그린다.
func drawGrid(screen *ebiten.Image, camX, camY float32) {
	gridColor := color.RGBA{225, 228, 234, 255}

	startX := float32(math.Floor(float64(camX/gridSize))) * gridSize
	for x := startX; x < camX+screenW; x += gridSize {
		sx := x - camX
		vector.StrokeLine(screen, sx, 0, sx, screenH, 1, gridColor, true)
	}
	startY := float32(math.Floor(float64(camY/gridSize))) * gridSize
	for y := startY; y < camY+screenH; y += gridSize {
		sy := y - camY
		vector.StrokeLine(screen, 0, sy, screenW, sy, 1, gridColor, true)
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

// 화면 중앙에 원형 구멍이 뚫린 검은 오버레이. 가장자리는 부드럽게 페이드.
func makeFog() *ebiten.Image {
	img := ebiten.NewImage(screenW, screenH)
	pix := make([]byte, screenW*screenH*4)
	cx, cy := float64(screenW)/2, float64(screenH)/2
	const fade = 60.0 // 경계 페이드 폭
	for y := 0; y < screenH; y++ {
		for x := 0; x < screenW; x++ {
			dx, dy := float64(x)-cx, float64(y)-cy
			d := math.Sqrt(dx*dx + dy*dy)
			var a float64
			switch {
			case d <= visionRadius-fade:
				a = 0
			case d >= visionRadius:
				a = 255
			default:
				a = (d - (visionRadius - fade)) / fade * 255
			}
			i := (y*screenW + x) * 4
			// 프리멀티플라이드 알파: 검정이라 RGB=0
			pix[i+3] = byte(a)
		}
	}
	img.WritePixels(pix)
	return img
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
		myID:    -1,
		fog:     makeFog(),
	}

	// 서버는 accept 시점에 자동 Join 을 처리하므로 클라에서 Join 을 또 보내지 않는다.
	go game.receiveLoop()

	ebiten.SetWindowSize(screenW, screenH)
	ebiten.SetWindowTitle("cppServer test client")

	if err := ebiten.RunGame(game); err != nil {
		panic(err)
	}
}
