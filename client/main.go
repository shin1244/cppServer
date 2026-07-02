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

const (
	keyUp    = 1 << 0
	keyDown  = 1 << 1
	keyLeft  = 1 << 2
	keyRight = 1 << 3
)

const (
	packetIDConnect      = 0
	packetIDDisconnect   = 1
	packetIDMove         = 2
	packetIDAttack       = 3
	packetIDRemovePlayer = 4
	packetIDRemoveBullet = 5
	packetIDHidePlayer   = 6
	packetIDHideBullet   = 7
	packetIDChat         = 8
	packetIDJoin         = 9
)

const (
	screenW = 900
	screenH = 900
)

type Game struct {
	conn net.Conn

	mu      sync.Mutex
	players map[int32]*PlayerView
	bullets map[int32]*BulletView

	myID         int32
	lastKeys     byte
	prevMouse    bool
	disconnected bool
	errText      string
}

type PlayerView struct {
	x float32
	y float32
}

type BulletView struct {
	ownerID int32
	x       float32
	y       float32
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
		g.sendMouseAttack()
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
	myID := g.myID
	disconnected := g.disconnected
	errText := g.errText
	g.mu.Unlock()

	status := fmt.Sprintf("WASD move | left click shoot | myID: %d | players: %d | bullets: %d", myID, len(g.players), len(g.bullets))
	if disconnected {
		status = "disconnected"
		if errText != "" {
			status += ": " + errText
		}
	}
	ebitenutil.DebugPrintAt(screen, status, 12, 12)
}

func (g *Game) Layout(int, int) (int, int) {
	return screenW, screenH
}

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
	switch id {
	case packetIDConnect:
		if len(body) < 4 {
			return
		}
		playerID := int32(binary.LittleEndian.Uint32(body[0:4]))

		g.mu.Lock()
		if g.myID < 0 {
			g.myID = playerID
		}
		if _, ok := g.players[playerID]; !ok {
			g.players[playerID] = &PlayerView{x: float32(screenW / 2), y: float32(screenH / 2)}
		}
		g.mu.Unlock()

	case packetIDMove:
		if len(body) < 12 {
			return
		}
		objectID := int32(binary.LittleEndian.Uint32(body[0:4]))
		x := math.Float32frombits(binary.LittleEndian.Uint32(body[4:8]))
		y := math.Float32frombits(binary.LittleEndian.Uint32(body[8:12]))

		g.mu.Lock()
		if _, ok := g.players[objectID]; ok || objectID < 4 {
			p := g.players[objectID]
			if p == nil {
				p = &PlayerView{}
				g.players[objectID] = p
			}
			p.x = x
			p.y = y
		} else {
			b := g.bullets[objectID]
			if b == nil {
				b = &BulletView{}
				g.bullets[objectID] = b
			}
			b.x = x
			b.y = y
		}
		g.mu.Unlock()

	case packetIDAttack:
		if len(body) < 16 {
			return
		}
		bulletID := int32(binary.LittleEndian.Uint32(body[0:4]))
		ownerID := int32(binary.LittleEndian.Uint32(body[4:8]))
		x := math.Float32frombits(binary.LittleEndian.Uint32(body[8:12]))
		y := math.Float32frombits(binary.LittleEndian.Uint32(body[12:16]))

		g.mu.Lock()
		b := g.bullets[bulletID]
		if b == nil {
			b = &BulletView{}
			g.bullets[bulletID] = b
		}
		b.ownerID = ownerID
		b.x = x
		b.y = y
		g.mu.Unlock()

	case packetIDRemovePlayer, packetIDHidePlayer:
		if len(body) < 4 {
			return
		}
		playerID := int32(binary.LittleEndian.Uint32(body[0:4]))
		g.mu.Lock()
		delete(g.players, playerID)
		g.mu.Unlock()

	case packetIDRemoveBullet, packetIDHideBullet:
		if len(body) < 4 {
			return
		}
		bulletID := int32(binary.LittleEndian.Uint32(body[0:4]))
		g.mu.Lock()
		delete(g.bullets, bulletID)
		g.mu.Unlock()
	}
}

func (g *Game) sendJoin() {
	g.writePacket(packetIDJoin, nil)
}

func (g *Game) sendMove(keys byte) {
	g.writePacket(packetIDMove, []byte{keys})
}

func (g *Game) sendMouseAttack() {
	g.mu.Lock()
	me := g.players[g.myID]
	g.mu.Unlock()
	if me == nil {
		return
	}

	mouseX, mouseY := ebiten.CursorPosition()
	dx := float32(mouseX) - me.x
	dy := float32(mouseY) - me.y
	length := float32(math.Sqrt(float64(dx*dx + dy*dy)))
	if length == 0 {
		return
	}

	body := make([]byte, 8)
	binary.LittleEndian.PutUint32(body[0:4], math.Float32bits(dx/length))
	binary.LittleEndian.PutUint32(body[4:8], math.Float32bits(dy/length))
	g.writePacket(packetIDAttack, body)
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
		myID:    -1,
	}

	game.sendJoin()
	go game.receiveLoop()

	ebiten.SetWindowSize(screenW, screenH)
	ebiten.SetWindowTitle("cppServer test client")

	if err := ebiten.RunGame(game); err != nil {
		panic(err)
	}
}
