const path = require("path");
const GLOBAL = "C:/Users/shin/AppData/Roaming/npm/node_modules";
const pptxgen = require(path.join(GLOBAL, "pptxgenjs"));

const pres = new pptxgen();
pres.layout = "LAYOUT_16x9"; // 10" x 5.625"
pres.author = "신준규";
pres.title = "IOCP 기반 실시간 대전 게임 서버 포트폴리오";

// ---- Palette --------------------------------------------------------------
const INK = "16202E";   // dark navy background
const INK2 = "22303F";  // dark card on dark bg
const TEAL = "0E8F84";  // accent
const TEALDK = "0B6F66";
const TEALLT = "5EEAD4";
const HEAD = "1E293B";   // headings on light
const SLATE = "3A4A5C";  // body text
const MUTED = "6B7A8D";  // muted labels
const CARD = "F4F7F9";   // light card
const TINT = "E7F2F1";   // teal tint card
const BORDER = "D5DDE5";
const WHITE = "FFFFFF";
const REDBG = "FBEDEC";  // warning tint
const RED = "B0413A";

const FONT = "맑은 고딕";       // Malgun Gothic – standard Windows Korean font
const MONO = "Consolas";

const W = 10, H = 5.625;

// ---- Helpers --------------------------------------------------------------
const shadow = () => ({ type: "outer", color: "000000", blur: 7, offset: 3, angle: 90, opacity: 0.10 });

function card(slide, x, y, w, h, fill = WHITE, opts = {}) {
  slide.addShape(pres.shapes.ROUNDED_RECTANGLE, {
    x, y, w, h, rectRadius: 0.06,
    fill: { color: fill },
    line: opts.line || { color: BORDER, width: 1 },
    shadow: opts.shadow ? shadow() : undefined,
  });
}

// Content-slide header: small kicker + big title
function header(slide, kicker, title) {
  slide.background = { color: WHITE };
  slide.addText(kicker.toUpperCase(), {
    x: 0.6, y: 0.42, w: 8.8, h: 0.3, margin: 0,
    fontFace: FONT, fontSize: 12, bold: true, color: TEAL, charSpacing: 2,
  });
  slide.addText(title, {
    x: 0.6, y: 0.72, w: 8.8, h: 0.62, margin: 0,
    fontFace: FONT, fontSize: 27, bold: true, color: HEAD,
  });
}

// Dashed placeholder box for user to drop captures into later
function placeholder(slide, x, y, w, h, label) {
  slide.addShape(pres.shapes.ROUNDED_RECTANGLE, {
    x, y, w, h, rectRadius: 0.06,
    fill: { color: "F8FAFB" },
    line: { color: "A9B7C4", width: 1.25, dashType: "dash" },
  });
  slide.addText(label, {
    x: x + 0.15, y, w: w - 0.3, h, margin: 0,
    fontFace: FONT, fontSize: 12.5, color: "8A98A6", italic: true,
    align: "center", valign: "middle",
  });
}

// Bulleted list
function bullets(slide, x, y, w, h, items, opts = {}) {
  slide.addText(
    items.map((t, i) => ({
      text: t,
      options: { bullet: { indent: 14 }, breakLine: true, paraSpaceAfter: opts.gap ?? 9 },
    })),
    {
      x, y, w, h, margin: 0,
      fontFace: FONT, fontSize: opts.fontSize || 14.5, color: opts.color || SLATE,
      lineSpacingMultiple: 1.05, valign: "top",
    }
  );
}

// Two comparison columns; returns nothing
function compareCard(slide, x, y, w, h, opts) {
  card(slide, x, y, w, h, opts.fill || CARD, { shadow: true, line: { color: opts.border || BORDER, width: 1 } });
  slide.addText(opts.title, {
    x: x + 0.25, y: y + 0.18, w: w - 0.5, h: 0.34, margin: 0,
    fontFace: FONT, fontSize: 15, bold: true, color: opts.titleColor || HEAD,
  });
  if (opts.tag) {
    slide.addText(opts.tag, {
      x: x + 0.25, y: y + 0.52, w: w - 0.5, h: 0.26, margin: 0,
      fontFace: MONO, fontSize: 11, color: opts.titleColor || MUTED,
    });
  }
  slide.addText(
    opts.lines.map((t) => ({ text: t, options: { bullet: { indent: 13 }, breakLine: true, paraSpaceAfter: 7 } })),
    {
      x: x + 0.28, y: y + (opts.tag ? 0.86 : 0.62), w: w - 0.55, h: h - (opts.tag ? 1.0 : 0.78),
      margin: 0, fontFace: FONT, fontSize: 12.5, color: SLATE, valign: "top",
    }
  );
}

// Result / takeaway callout strip
function callout(slide, x, y, w, h, label, text) {
  card(slide, x, y, w, h, TINT, { line: { color: TEALLT, width: 1 } });
  slide.addText(label.toUpperCase(), {
    x: x + 0.25, y: y + 0.12, w: w - 0.5, h: 0.25, margin: 0,
    fontFace: FONT, fontSize: 10.5, bold: true, color: TEALDK, charSpacing: 1.5,
  });
  slide.addText(text, {
    x: x + 0.25, y: y + 0.36, w: w - 0.5, h: h - 0.48, margin: 0,
    fontFace: FONT, fontSize: 13, color: HEAD, valign: "top", lineSpacingMultiple: 1.02,
  });
}

// Flowchart node: rounded box with optional number badge and sub-lines
function flowNode(slide, x, y, w, h, opts) {
  const kind = opts.kind || "normal";
  let fill = WHITE, line = { color: BORDER, width: 1.25 }, tcol = HEAD, badgeFill = TEALDK, badgeTxt = WHITE, subCol = MUTED;
  if (kind === "start") { fill = TEAL; line = { color: TEALDK, width: 1 }; tcol = WHITE; badgeFill = WHITE; badgeTxt = TEALDK; subCol = "E7F2F1"; }
  else if (kind === "accent") { fill = TINT; line = { color: TEALLT, width: 1.25 }; }
  slide.addShape(pres.shapes.ROUNDED_RECTANGLE, { x, y, w, h, rectRadius: 0.07, fill: { color: fill }, line, shadow: shadow() });
  let tx = x + 0.24;
  if (opts.num != null) {
    const bd = 0.34;
    const by = y + (opts.sub ? 0.15 : (h - bd) / 2);
    slide.addShape(pres.shapes.OVAL, { x: x + 0.18, y: by, w: bd, h: bd, fill: { color: badgeFill } });
    slide.addText(String(opts.num), { x: x + 0.18, y: by, w: bd, h: bd, margin: 0, fontFace: FONT, fontSize: 12.5, bold: true, color: badgeTxt, align: "center", valign: "middle" });
    tx = x + 0.62;
  }
  const tw = x + w - tx - 0.15;
  if (opts.sub) {
    slide.addText(opts.title, { x: tx, y: y + 0.1, w: tw, h: 0.3, margin: 0, fontFace: FONT, fontSize: opts.fs || 13.5, bold: true, color: tcol, valign: "middle" });
    slide.addText(
      opts.sub.map((t, i) => ({ text: t, options: { breakLine: i < opts.sub.length - 1 } })),
      { x: tx, y: y + 0.42, w: tw, h: h - 0.5, margin: 0, fontFace: FONT, fontSize: 10.5, color: subCol, valign: "top", lineSpacingMultiple: 1.02 }
    );
  } else {
    slide.addText(opts.title, { x: tx, y, w: tw, h, margin: 0, fontFace: FONT, fontSize: opts.fs || 13.5, bold: true, color: tcol, valign: "middle" });
  }
}

function vArrow(slide, cx, y, len) {
  slide.addShape(pres.shapes.LINE, { x: cx, y, w: 0, h: len, line: { color: "9AA8B5", width: 2, endArrowType: "triangle" } });
}
function hArrow(slide, x, cy, len) {
  slide.addShape(pres.shapes.LINE, { x, y: cy, w: len, h: 0, line: { color: "9AA8B5", width: 2, endArrowType: "triangle" } });
}

// =========================================================================
// SLIDE 1 — TITLE
// =========================================================================
{
  const s = pres.addSlide();
  s.background = { color: INK };
  // subtle corner block motif
  s.addShape(pres.shapes.RECTANGLE, { x: 0, y: 0, w: W, h: 0.12, fill: { color: TEAL } });

  s.addText("C++17  ·  WINDOWS IOCP  ·  GO CLIENT / LOAD BOT", {
    x: 0.7, y: 1.02, w: 8.6, h: 0.32, margin: 0,
    fontFace: FONT, fontSize: 13, bold: true, color: TEALLT, charSpacing: 2,
  });
  s.addText("IOCP 기반 실시간 대전 게임 서버", {
    x: 0.68, y: 1.42, w: 8.7, h: 0.85, margin: 0,
    fontFace: FONT, fontSize: 40, bold: true, color: WHITE,
  });
  s.addText("400명 규모 부하에 대응하는 top-down 슈팅 게임 서버", {
    x: 0.7, y: 2.32, w: 8.6, h: 0.4, margin: 0,
    fontFace: FONT, fontSize: 17, color: "C7D2DE",
  });
  s.addText(
    "네트워크 I/O · 메모리 배치 · AOI 탐색 구조를 직접 구현하고 부하 상황에서 비교하며\n서버 설계의 병목과 안정성을 검증한 프로젝트",
    { x: 0.7, y: 2.82, w: 8.3, h: 0.7, margin: 0, fontFace: FONT, fontSize: 13.5, color: "9FB0C0", lineSpacingMultiple: 1.15 }
  );

  // keyword chips
  const chips = ["IOCP", "30Hz Tick", "400 CCU", "AOI Grid"];
  let cx = 0.7;
  chips.forEach((c) => {
    const cw = 0.5 + c.length * 0.135;
    s.addShape(pres.shapes.ROUNDED_RECTANGLE, {
      x: cx, y: 3.75, w: cw, h: 0.46, rectRadius: 0.23,
      fill: { color: INK2 }, line: { color: TEAL, width: 1 },
    });
    s.addText(c, { x: cx, y: 3.75, w: cw, h: 0.46, margin: 0, fontFace: FONT, fontSize: 13, bold: true, color: TEALLT, align: "center", valign: "middle" });
    cx += cw + 0.22;
  });

  s.addText("신준규  ·  포트폴리오", {
    x: 0.7, y: 4.95, w: 6, h: 0.3, margin: 0, fontFace: FONT, fontSize: 12, color: "7C8B9B",
  });
  s.addNotes("표지. 이 프로젝트는 '동작하는 서버'가 아니라 실시간 게임 서버의 핵심 설계 선택지를 직접 구현·비교하는 데 목적이 있음을 강조.");
}

// =========================================================================
// SLIDE 2 — 프로젝트 개요
// =========================================================================
{
  const s = pres.addSlide();
  header(s, "Overview", "프로젝트 개요");

  s.addText("단순히 동작하는 서버가 아니라, 실시간 게임 서버의 핵심 설계 선택지를 직접 실험하기 위한 프로젝트입니다.", {
    x: 0.6, y: 1.42, w: 5.35, h: 0.7, margin: 0,
    fontFace: FONT, fontSize: 15, bold: true, color: TEALDK, lineSpacingMultiple: 1.15,
  });
  bullets(s, 0.62, 2.28, 5.35, 3.0, [
    "C++17 기반 Windows IOCP 서버",
    "Go 렌더링 클라이언트 + Go 헤드리스 부하 봇 구현",
    "방 2개 · 방당 최대 200명 · 총 400명 동시 접속 부하 구조",
    "이동 · 사격 · 벽 파괴 · 피격 후 관전 전환",
    "시야(LOS) + 반경 기반 AOI 전송",
  ], { gap: 11 });

  // right: quick-fact stat grid
  const stats = [
    ["2", "게임 방 (World)"],
    ["200", "방당 최대 인원"],
    ["400", "총 동시 접속(CCU)"],
    ["4096", "방당 최대 총알"],
    ["32", "IOCP 워커 스레드"],
    ["33ms", "로직 틱 (~30Hz)"],
  ];
  const gx = 6.25, gy = 1.5, gw = 1.68, gh = 1.14, gapx = 0.13, gapy = 0.14;
  stats.forEach((st, i) => {
    const col = i % 2, row = Math.floor(i / 2);
    const x = gx + col * (gw + gapx), y = gy + row * (gh + gapy);
    card(s, x, y, gw, gh, i % 2 === row % 2 ? TINT : CARD, { shadow: true });
    s.addText(st[0], { x, y: y + 0.16, w: gw, h: 0.5, margin: 0, fontFace: FONT, fontSize: 26, bold: true, color: TEALDK, align: "center" });
    s.addText(st[1], { x: x + 0.05, y: y + 0.68, w: gw - 0.1, h: 0.36, margin: 0, fontFace: FONT, fontSize: 11, color: MUTED, align: "center" });
  });
  s.addNotes("규모 수치는 World.h(MAX_PLAYER=200, MAX_BULLETS=4096), main.cpp(워커 32개, TICK_MS=33), worlds(2)에서 확인.");
}

// =========================================================================
// SLIDE 3 — 아키텍처 개요 (스레드 분리 흐름)
// =========================================================================
{
  const s = pres.addSlide();
  header(s, "Architecture", "아키텍처 — 책임을 나눈 두 종류의 스레드");

  s.addText("네트워크 I/O와 게임 상태 갱신의 책임을 분리했습니다. 무작정 멀티스레드로 나누지 않았습니다.", {
    x: 0.6, y: 1.4, w: 8.8, h: 0.35, margin: 0, fontFace: FONT, fontSize: 14.5, color: SLATE,
  });

  // Flow: two big boxes with a queue between
  const by = 2.05, bh = 1.9;
  // Left: IOCP workers
  card(s, 0.6, by, 3.7, bh, CARD, { shadow: true, line: { color: TEAL, width: 1.25 } });
  s.addText("IOCP 워커 스레드 × 32", { x: 0.75, y: by + 0.18, w: 3.4, h: 0.35, margin: 0, fontFace: FONT, fontSize: 15, bold: true, color: TEALDK });
  bullets(s, 0.8, by + 0.62, 3.35, 1.2, ["WSARecv 수신 완료 처리", "패킷 프레이밍 · 파싱", "스레드세이프 큐에 push"], { fontSize: 12.5, gap: 6 });

  // Middle queue chip
  s.addShape(pres.shapes.ROUNDED_RECTANGLE, { x: 4.42, y: by + 0.6, w: 1.16, h: 0.68, rectRadius: 0.1, fill: { color: INK }, });
  s.addText("전달 큐", { x: 4.42, y: by + 0.66, w: 1.16, h: 0.3, margin: 0, fontFace: FONT, fontSize: 12.5, bold: true, color: WHITE, align: "center" });
  s.addText("[실험 1]", { x: 4.42, y: by + 0.94, w: 1.16, h: 0.26, margin: 0, fontFace: FONT, fontSize: 10, color: TEALLT, align: "center" });
  // arrows
  s.addShape(pres.shapes.LINE, { x: 4.3, y: by + 0.94, w: 0.12, h: 0, line: { color: MUTED, width: 2, endArrowType: "triangle" } });
  s.addShape(pres.shapes.LINE, { x: 5.58, y: by + 0.94, w: 0.14, h: 0, line: { color: MUTED, width: 2, endArrowType: "triangle" } });

  // Right: logic thread
  card(s, 5.72, by, 3.68, bh, CARD, { shadow: true, line: { color: TEAL, width: 1.25 } });
  s.addText("로직 스레드 × 1  ·  33ms 틱", { x: 5.87, y: by + 0.18, w: 3.4, h: 0.35, margin: 0, fontFace: FONT, fontSize: 15, bold: true, color: TEALDK });
  bullets(s, 5.92, by + 0.62, 3.35, 1.2, ["큐 소비 → 월드 업데이트", "이동 · 충돌 · 피격 판정", "AOI 대상에게 WSASend"], { fontSize: 12.5, gap: 6 });

  callout(s, 0.6, 4.28, 8.8, 0.95, "핵심 설계",
    "게임 상태(플레이어 · 총알 · 맵)를 단일 로직 스레드가 소유하므로 상태 자체에는 락이 필요 없습니다. 락은 세션 송신 버퍼, 큐 push/swap 등 스레드 경계 지점에만 집중됩니다.");
  s.addNotes("main.cpp: 워커 32개 detach, 로직 루프 33ms. 상태 소유권을 단일 스레드에 두어 락 경합을 경계로 몰아넣은 것이 핵심.");
}

// =========================================================================
// SLIDE 3b — 서버 실행 플로우: 시작과 준비
// =========================================================================
{
  const s = pres.addSlide();
  header(s, "Server Startup", "서버 실행 플로우 — 시작과 준비");
  s.addText("시작 시 네트워크와 월드 기반을 준비하고, 방이 가득 차면 게임을 시작합니다.", {
    x: 0.6, y: 1.4, w: 8.8, h: 0.35, margin: 0, fontFace: FONT, fontSize: 14.5, color: SLATE,
  });

  // phase labels
  s.addText("① 네트워크 · 스레드 부팅", { x: 0.7, y: 1.82, w: 3.6, h: 0.3, margin: 0, fontFace: FONT, fontSize: 12.5, bold: true, color: TEALDK });
  s.addText("② 월드 · 맵 준비 → 게임 시작", { x: 5.3, y: 1.82, w: 4.1, h: 0.3, margin: 0, fontFace: FONT, fontSize: 12.5, bold: true, color: TEALDK });

  // left column
  const LX = 0.7, LW = 3.6, lh = 0.55, lgap = 0.30;
  let ly = 2.15;
  const lnodes = [
    ["1", "Winsock 초기화"],
    ["2", "IOCP 생성"],
    ["3", "Listen Socket · bind · listen"],
    ["4", "Worker ×32 + Accept Thread 시작"],
  ];
  lnodes.forEach((nd, i) => {
    flowNode(s, LX, ly, LW, lh, { num: nd[0], title: nd[1] });
    if (i < lnodes.length - 1) vArrow(s, LX + LW / 2, ly + lh, lgap);
    ly += lh + lgap;
  });

  // connector: phase 1 -> phase 2
  hArrow(s, LX + LW + 0.05, 3.5, 0.6);

  // right column
  const RX = 5.3, RW = 4.1;
  let ry = 2.15;
  flowNode(s, RX, ry, RW, 0.55, { num: "5", title: "World 2개 생성" });
  vArrow(s, RX + RW / 2, ry + 0.55, 0.18); ry += 0.73;
  flowNode(s, RX, ry, RW, 0.95, { num: "6", title: "Map 생성", sub: ["랜덤 벽 배치 · 연결성 검사(BFS)", "스폰 좌표 설정 · 연결될 때까지 재생성"] });
  vArrow(s, RX + RW / 2, ry + 0.95, 0.18); ry += 1.13;
  flowNode(s, RX, ry, RW, 0.55, { num: "7", title: "방당 200명 접속 대기" });
  vArrow(s, RX + RW / 2, ry + 0.55, 0.18); ry += 0.73;
  flowNode(s, RX, ry, RW, 0.68, { num: "8", title: "게임 시작", sub: ["→ 맵 스냅샷 전송 후 33ms 로직 틱 시작"], kind: "accent" });

  s.addNotes("main.cpp 시작 순서: WSAStartup → IOCP → listen → worker 32개 detach + Accepter → worlds(2) Init → Map.Generate(랜덤 벽 → IsFullyConnected(BFS) 재시도 → GenerateSpawnPoints). TryStartMatch는 방이 가득 차면 running=true + BroadcastMapSnapshot.");
}

// =========================================================================
// SLIDE 3c — 33ms 로직 틱: 매 틱 반복
// =========================================================================
{
  const s = pres.addSlide();
  header(s, "Logic Tick · 30Hz", "33ms 로직 틱 — 매 틱 반복");
  s.addText("게임이 시작되면 33ms마다 입력 처리 → 상태 갱신 → AOI 송신을 반복합니다.", {
    x: 0.6, y: 1.4, w: 8.8, h: 0.35, margin: 0, fontFace: FONT, fontSize: 14.5, color: SLATE,
  });

  // left flow
  const LX = 0.7, LW = 3.7;
  let ly = 2.0;
  flowNode(s, LX, ly, LW, 0.55, { title: "33ms Logic Tick", kind: "start" });
  vArrow(s, LX + LW / 2, ly + 0.55, 0.22); ly += 0.77;
  flowNode(s, LX, ly, LW, 1.0, { title: "수신 패킷 소비", sub: ["Join / Leave", "Move 입력 · Attack · Observe 요청"] });
  vArrow(s, LX + LW / 2, ly + 1.0, 0.22); ly += 1.22;
  flowNode(s, LX, ly, LW, 1.0, { title: "패킷 핸들링", sub: ["클라이언트는 좌표가 아닌 입력 키만 전송", "공격 요청은 서버가 쿨타임 검증"] });

  // arrow to World.Update card
  hArrow(s, LX + LW + 0.03, ly + 0.5, 0.4);

  // right: World.Update card
  const RX = 4.85, RW = 4.55, RY = 2.0, RH = 2.95;
  card(s, RX, RY, RW, RH, WHITE, { shadow: true });
  s.addText("World.Update — 매 틱 상태 갱신", {
    x: RX + 0.25, y: RY + 0.18, w: RW - 0.5, h: 0.34, margin: 0, fontFace: FONT, fontSize: 14.5, bold: true, color: TEALDK,
  });
  const steps = [
    "유저 이동 · 벽 충돌 판정",
    "총알 이동 · 벽 충돌 · 벽 파괴",
    "Grid 업데이트",
    "유저-총알 충돌 판정",
    "게임 종료 여부 판정 (생존 1명)",
    "Grid + LOS + 반경 기준 상황 송신 (AOI)",
  ];
  s.addText(
    steps.map((t) => ({ text: t, options: { bullet: { type: "number" }, breakLine: true, paraSpaceAfter: 9 } })),
    { x: RX + 0.32, y: RY + 0.64, w: RW - 0.62, h: RH - 0.82, margin: 0, fontFace: FONT, fontSize: 13, color: SLATE, valign: "top" }
  );

  // loop-back note
  s.addShape(pres.shapes.ROUNDED_RECTANGLE, { x: 0.6, y: 5.12, w: 8.8, h: 0.4, rectRadius: 0.2, fill: { color: INK } });
  s.addText("매 틱 반복  ·  위 과정을 33ms마다(≈30Hz) 수행  ·  생존자 1명이 되면 매치 종료로 정지", {
    x: 0.6, y: 5.12, w: 8.8, h: 0.4, margin: 0, fontFace: FONT, fontSize: 12, bold: true, color: "E7F2F1", align: "center", valign: "middle",
  });

  s.addNotes("HandleMove는 입력 키 바이트만 수신(SetKeys), HandleAttack은 CanFire()로 쿨타임 검증 후 Fire. World.Update 순서: UpdatePlayers→UpdateBullets→UpdateGrid→Collision→CheckMatchEnd→SendAOIUpdates.");
}

// =========================================================================
// SLIDE 4 — 설계 1: 예측 가능한 메모리 & 캐시 친화성
// =========================================================================
{
  const s = pres.addSlide();
  header(s, "Design · Memory", "예측 가능한 메모리와 캐시 친화성");

  s.addText("힙 할당을 최소화하고 고정 크기 배열·풀을 사용해, 부하 상황에서도 메모리 사용 패턴이 예측 가능하도록 설계했습니다.", {
    x: 0.6, y: 1.4, w: 5.35, h: 0.75, margin: 0, fontFace: FONT, fontSize: 14.5, bold: true, color: TEALDK, lineSpacingMultiple: 1.15,
  });
  bullets(s, 0.62, 2.28, 5.35, 2.1, [
    "세션은 ObjectPool<Session, 1000>에서 미리 확보한 배열로 할당/반납",
    "플레이어 슬롯·총알도 고정 배열(slots[200], bullets[4096])로 관리",
    "총알은 생성/삭제 대신 active 플래그로 재사용",
    "자주 접근하는 데이터를 연속된 메모리에 가깝게 배치",
  ], { gap: 10 });

  callout(s, 0.6, 4.55, 5.35, 0.75, "관점",
    "중요한 것은 평균 속도만이 아니라, 부하 시 메모리 사용량과 접근 패턴이 예측 가능하게 유지되는 것입니다.");

  // right: code basis + capture placeholder
  card(s, 6.15, 1.5, 3.25, 1.5, INK, {});
  s.addText("코드 근거", { x: 6.35, y: 1.62, w: 2.9, h: 0.28, margin: 0, fontFace: FONT, fontSize: 11, bold: true, color: TEALLT, charSpacing: 1.5 });
  s.addText([
    { text: "ObjectPool.h", options: { breakLine: true } },
    { text: "  T items[N] · freeList", options: { breakLine: true, color: "9FB0C0", fontSize: 10.5 } },
    { text: "NetworkCore.cpp", options: { breakLine: true } },
    { text: "  ObjectPool<Session,1000>", options: { breakLine: true, color: "9FB0C0", fontSize: 10.5 } },
    { text: "Bullet.cpp — Fire / Clear / IsActive", options: {} },
  ], { x: 6.35, y: 1.92, w: 2.9, h: 1.0, margin: 0, fontFace: MONO, fontSize: 11, color: WHITE, lineSpacingMultiple: 1.08 });

  placeholder(s, 6.15, 3.2, 3.25, 2.1, "[ 코드 캡처 삽입 ]\nObjectPool.h / 고정 배열 선언부");
  s.addNotes("힙 최소화·풀·고정배열이 캐시 친화성과 예측 가능성을 만든다는 점. 코드 캡처는 ObjectPool.h와 World.h의 배열 멤버 위주.");
}

// =========================================================================
// SLIDE 5 — 설계 2: 스레드 책임 분리 상세
// =========================================================================
{
  const s = pres.addSlide();
  header(s, "Design · Threading", "멀티스레드보다 명확한 책임 분리");

  s.addText("모든 로직을 스레드로 쪼개는 대신, '누가 무엇을 소유하는가'를 명확히 나누는 데 집중했습니다.", {
    x: 0.6, y: 1.4, w: 8.8, h: 0.35, margin: 0, fontFace: FONT, fontSize: 14.5, color: SLATE,
  });

  compareCard(s, 0.6, 2.0, 4.28, 2.35, {
    title: "IOCP 워커 스레드 × 32", titleColor: TEALDK, tag: "recv → parse → push",
    lines: ["GetQueuedCompletionStatus로 완료 수신", "WSARecv 결과를 프레임 단위로 파싱", "완성된 패킷을 큐에 push", "I/O 병렬성 확보가 목적"],
  });
  compareCard(s, 5.12, 2.0, 4.28, 2.35, {
    title: "로직 스레드 × 1", titleColor: TEALDK, tag: "consume → update → send",
    lines: ["틱마다 입력 큐 소비", "UpdatePlayers · UpdateBullets · Collision", "SendAOIUpdates로 상태 브로드캐스트", "게임 상태를 단독 소유 → 상태 락 불필요"],
  });

  callout(s, 0.6, 4.55, 8.8, 0.72, "락은 어디에만 있나",
    "세션 송신 버퍼, 큐 push/swap 같은 스레드 경계 지점에만 락을 둡니다. 플레이어·총알·맵 상태에는 락이 없습니다.");
  s.addNotes("워커=I/O, 로직=상태. 락을 경계로만 몰아넣은 구조. World.cpp의 Update 흐름 함수명 참고.");
}

// =========================================================================
// SLIDE 6 — 실험 1: 이벤트 큐 vs 더블 스왑 버퍼
// =========================================================================
{
  const s = pres.addSlide();
  header(s, "Experiment 1 · Queue", "이벤트 큐 vs 더블 스왑 버퍼");

  s.addText("워커 → 로직 스레드 전달 큐를 두 방식으로 구현해, 락 경합 비용을 비교했습니다.", {
    x: 0.6, y: 1.4, w: 8.8, h: 0.35, margin: 0, fontFace: FONT, fontSize: 14.5, color: SLATE,
  });

  compareCard(s, 0.6, 2.0, 3.05, 2.35, {
    title: "이벤트 큐", titleColor: HEAD, tag: "EventQueue.h",
    lines: ["단일 큐 + 뮤텍스", "push · pop 매 건마다 락", "구현이 단순"],
  });
  compareCard(s, 3.78, 2.0, 3.05, 2.35, {
    title: "더블 스왑 버퍼", titleColor: TEALDK, tag: "DoubleBuffer.h",
    lines: ["버퍼 2개 운용", "워커는 push 시에만 락", "로직은 틱당 1회 swap"],
    fill: TINT, border: TEALLT,
  });
  placeholder(s, 6.95, 2.0, 2.45, 2.35, "[ 벤치마크 그래프 ]\n큐 소비 시간 비교");

  callout(s, 0.6, 4.55, 8.8, 0.72, "결과 · 배운 점",
    "전체 틱 시간은 게임 로직에 묻혀 차이가 안 보였고, 순수 큐 소비 시간(Consume Time)을 분리 측정하자 더블 스왑이 약 2~3배 빠름 — 최적화는 구현보다 '무엇을 측정할지'가 관건.");
  s.addNotes("main.cpp의 totalConsumeTime로 소비 시간만 분리 측정. USE_EVENT_QUEUE 매크로로 토글.");
}

// =========================================================================
// SLIDE 7 — 실험 2: 링 버퍼 vs 가변 버퍼
// =========================================================================
{
  const s = pres.addSlide();
  header(s, "Experiment 2 · Recv Buffer", "링 버퍼 vs 가변 버퍼");

  s.addText("정상 부하에서는 비슷하지만, backpressure 상황에서 '실패하는 방식'이 완전히 달라집니다.", {
    x: 0.6, y: 1.4, w: 8.8, h: 0.35, margin: 0, fontFace: FONT, fontSize: 14.5, color: SLATE,
  });

  compareCard(s, 0.6, 2.0, 3.05, 2.35, {
    title: "링 버퍼", titleColor: TEALDK, tag: "RingBuffer.*",
    lines: ["고정 크기 · 메모리 상한 명확", "상한 내에서 세션을 격리", "오버플로(backpressure) 정책 필요"],
    fill: TINT, border: TEALLT,
  });
  compareCard(s, 3.78, 2.0, 3.05, 2.35, {
    title: "가변 버퍼", titleColor: RED, tag: "VecterBuffer.*",
    lines: ["필요한 만큼 grow", "악성 패킷에 취약", "세션마다 메모리 증가 → DoS/OOM 위험"],
    fill: REDBG, border: "E7C4C0",
  });
  placeholder(s, 6.95, 2.0, 2.45, 2.35, "[ 메모리 그래프 ]\n정상 vs 악성 부하");

  callout(s, 0.6, 4.55, 8.8, 0.72, "결론",
    "악성 봇(완성되지 않는 거대 패킷)으로 검증. 핵심은 '어느 게 빠른가'가 아니라 '최악의 경우 어떻게 무너지는가' — 링은 상한이, 가변은 cap이 반드시 필요.");
  s.addNotes("client/bot/main.go의 runMaliciousBot로 미완성 대형 패킷 전송. 링 버퍼 full/empty 오인 버그도 이 과정에서 수정.");
}

// =========================================================================
// SLIDE 8 — 실험 3: 전체 순회 vs 공간 그리드
// =========================================================================
{
  const s = pres.addSlide();
  header(s, "Experiment 3 · AOI", "전체 순회 vs 공간 그리드 (AOI)");

  s.addText("AOI 최적화의 핵심은 단순한 속도 개선이 아니라, 플레이어 수 증가에 대한 확장성입니다.", {
    x: 0.6, y: 1.4, w: 8.8, h: 0.35, margin: 0, fontFace: FONT, fontSize: 14.5, color: SLATE,
  });

  compareCard(s, 0.6, 2.0, 3.05, 2.35, {
    title: "전체 순회", titleColor: HEAD, tag: "O(N²) + O(N·M)",
    lines: ["모든 플레이어 쌍을 검사", "총알까지 곱해져 비용 급증", "N이 커질수록 급격히 악화"],
  });
  compareCard(s, 3.78, 2.0, 3.05, 2.35, {
    title: "공간 그리드", titleColor: TEALDK, tag: "≈ O(N·k)",
    lines: ["주변 셀 후보만 수집", "broad-phase만 교체", "분산될수록 이점 확대"],
    fill: TINT, border: TEALLT,
  });
  placeholder(s, 6.95, 2.0, 2.45, 2.35, "[ 틱 시간 그래프 ]\nN 증가에 따른 비교");

  callout(s, 0.6, 4.55, 8.8, 0.72, "핵심",
    "후보 수집만 교체하고 거리·시야(LOS) 필터는 공유해 두 방식의 결과를 동일하게 맞췄습니다. 그리드의 강점은 '빠름'보다 N에 대한 확장성(제곱 → 선형).");
  s.addNotes("SpatialGrid.*, World.cpp의 UpdateGrid/SendAOIUpdates, Map.cpp의 HasLineOfSight. USE_NOT_GRID 매크로로 토글.");
}

// =========================================================================
// SLIDE 9 — 게임 로직 & 네트워크 프로토콜
// =========================================================================
{
  const s = pres.addSlide();
  header(s, "Game Logic & Protocol", "게임 로직과 네트워크 프로토콜");

  s.addText("클라이언트는 입력만 보내고, 위치·충돌·피격·관전 같은 주요 상태는 서버가 결정합니다.", {
    x: 0.6, y: 1.4, w: 8.8, h: 0.35, margin: 0, fontFace: FONT, fontSize: 14.5, bold: true, color: TEALDK,
  });
  bullets(s, 0.62, 2.0, 5.35, 2.0, [
    "클라이언트: 이동 키와 공격 목표 좌표만 전송",
    "서버: 플레이어 이동 · 벽 충돌 · 총알 이동 · 피격 판정",
    "벽 파괴 · 플레이어/총알 제거 · 관전 대상 변경을 브로드캐스트",
    "서버 권위(authority) 방식으로 상태 일관성 확보",
  ], { gap: 10 });

  // packet format diagram
  s.addText("패킷 구조 — 작은 바이너리 프로토콜", { x: 0.62, y: 4.12, w: 5.3, h: 0.3, margin: 0, fontFace: FONT, fontSize: 12.5, bold: true, color: HEAD });
  const pf = [["size", TEAL], ["id", TEALDK], ["body", INK2]];
  let px = 0.62;
  const pw = [1.05, 1.05, 3.1];
  pf.forEach((seg, i) => {
    s.addShape(pres.shapes.RECTANGLE, { x: px, y: 4.45, w: pw[i], h: 0.62, fill: { color: seg[1] } });
    s.addText(seg[0], { x: px, y: 4.45, w: pw[i], h: 0.62, margin: 0, fontFace: MONO, fontSize: 14, bold: true, color: WHITE, align: "center", valign: "middle" });
    px += pw[i] + 0.04;
  });
  s.addText("[ 2B | 2B | … ]", { x: 0.62, y: 5.1, w: 5.2, h: 0.25, margin: 0, fontFace: MONO, fontSize: 10.5, color: MUTED });

  placeholder(s, 6.15, 2.0, 3.25, 3.3, "[ 게임 실행 화면 캡처 ]\n이동 · 사격 · 시야(전장의 안개)");
  s.addNotes("Protocol.h의 PacketHeader{size,id}. World_Net.cpp 핸들러, World.cpp 상태 갱신. 우측 캡처는 렌더링 클라이언트 실행 화면.");
}

// =========================================================================
// SLIDE 10 — 부하 테스트 & 관찰 지표
// =========================================================================
{
  const s = pres.addSlide();
  header(s, "Load Test & Metrics", "부하 테스트와 관찰 지표");

  s.addText("렌더링 클라이언트와 별도로 헤드리스 봇을 구현해 서버 부하를 재현했습니다.", {
    x: 0.6, y: 1.4, w: 8.8, h: 0.35, margin: 0, fontFace: FONT, fontSize: 14.5, color: SLATE,
  });
  bullets(s, 0.62, 2.0, 5.35, 2.15, [
    "Go 봇으로 다수의 가상 플레이어 접속",
    "이동/사격 패킷을 주기적으로 전송",
    "악성 모드: 미완성 대형 패킷으로 수신 버퍼 압박",
    "120초 동안 서버 지표를 자동 측정",
  ], { gap: 10 });

  // metrics measured chips
  const ms = ["평균 틱", "최고 틱", "최소 틱", "큐 소비 시간", "메모리 (Working Set / Peak)"];
  s.addText("측정 지표", { x: 0.62, y: 4.2, w: 5, h: 0.3, margin: 0, fontFace: FONT, fontSize: 12.5, bold: true, color: HEAD });
  let mx = 0.62, my = 4.55;
  ms.forEach((m) => {
    const mw = 0.42 + m.length * 0.115;
    if (mx + mw > 5.95) { mx = 0.62; my += 0.5; }
    s.addShape(pres.shapes.ROUNDED_RECTANGLE, { x: mx, y: my, w: mw, h: 0.4, rectRadius: 0.2, fill: { color: CARD }, line: { color: BORDER, width: 1 } });
    s.addText(m, { x: mx, y: my, w: mw, h: 0.4, margin: 0, fontFace: FONT, fontSize: 11, color: SLATE, align: "center", valign: "middle" });
    mx += mw + 0.13;
  });

  placeholder(s, 6.15, 2.0, 3.25, 3.3, "[ 벤치마크 콘솔 캡처 ]\nFINAL BENCHMARK RESULT (120s)");
  s.addNotes("client/bot/main.go, main.cpp의 TickBenchmark + GetProcessMemoryInfo. 우측은 최종 결과 콘솔 출력 캡처.");
}

// =========================================================================
// SLIDE 11 — 결과와 배운 점 (dark)
// =========================================================================
{
  const s = pres.addSlide();
  s.background = { color: INK };
  s.addShape(pres.shapes.RECTANGLE, { x: 0, y: 0, w: W, h: 0.12, fill: { color: TEAL } });

  s.addText("RESULTS & TAKEAWAYS", { x: 0.7, y: 0.55, w: 8.6, h: 0.3, margin: 0, fontFace: FONT, fontSize: 12, bold: true, color: TEALLT, charSpacing: 2 });
  s.addText("결과와 배운 점", { x: 0.68, y: 0.85, w: 8.7, h: 0.6, margin: 0, fontFace: FONT, fontSize: 30, bold: true, color: WHITE });
  s.addText("핵심 성과는 IOCP 서버 구현 자체보다, 설계 선택지를 직접 비교하며 병목과 실패 조건을 확인한 것입니다.", {
    x: 0.7, y: 1.5, w: 8.6, h: 0.35, margin: 0, fontFace: FONT, fontSize: 13.5, color: "9FB0C0",
  });

  const lessons = [
    ["메모리 배치", "평균 성능뿐 아니라 서버의 예측 가능성을 만든다."],
    ["멀티스레드", "많이 쓰는 게 아니라, 상태 소유권을 명확히 나누는 것이 중요하다."],
    ["큐 최적화", "전체 틱이 아니라 소비 시간 같은 정확한 지표로 봐야 한다."],
    ["버퍼 설계", "정상 부하보다 악성·역압 상황에서 차이가 드러난다."],
    ["AOI", "알고리즘 복잡도를 바꾸는 순간 확장성이 달라진다."],
  ];
  const ly = 2.1, lh = 0.6, lgap = 0.08;
  lessons.forEach((l, i) => {
    const y = ly + i * (lh + lgap);
    card(s, 0.7, y, 8.6, lh, INK2, { line: { color: "31414F", width: 1 } });
    s.addText(l[0], { x: 0.9, y, w: 2.1, h: lh, margin: 0, fontFace: FONT, fontSize: 14.5, bold: true, color: TEALLT, valign: "middle" });
    s.addShape(pres.shapes.LINE, { x: 3.05, y: y + 0.12, w: 0, h: lh - 0.24, line: { color: "3C4C5A", width: 1 } });
    s.addText(l[1], { x: 3.25, y, w: 5.85, h: lh, margin: 0, fontFace: FONT, fontSize: 13.5, color: "E2E8EF", valign: "middle" });
  });
  s.addNotes("5가지 배운 점으로 마무리. 각 실험 슬라이드의 결론을 한 줄로 압축한 것.");
}

pres.writeFile({ fileName: "portfolio.pptx" }).then((f) => console.log("wrote", f));
