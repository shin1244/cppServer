const path = require("path");
const GLOBAL = "C:/Users/shin/AppData/Roaming/npm/node_modules";
const pptxgen = require(path.join(GLOBAL, "pptxgenjs"));

// palette / fonts — matched to the main deck
const TEALDK = "0B6F66", TEAL = "0E8F84";
const HEAD = "1E293B", SLATE = "3A4A5C", MUTED = "6B7A8D";
const CARD = "F4F7F9", TINT = "E7F2F1", BORDER = "D5DDE5", WHITE = "FFFFFF";
const RED = "B0413A";
const FONT = "맑은 고딕";

const pres = new pptxgen();
pres.layout = "LAYOUT_16x9";
pres.title = "부하 테스트 결과 표";

const s = pres.addSlide();
s.background = { color: WHITE };

s.addText("BENCHMARK RESULTS", { x: 0.5, y: 0.42, w: 9, h: 0.3, margin: 0, fontFace: FONT, fontSize: 12, bold: true, color: TEAL, charSpacing: 2 });
s.addText("부하 테스트 결과  ·  120초 측정", { x: 0.5, y: 0.72, w: 9, h: 0.55, margin: 0, fontFace: FONT, fontSize: 26, bold: true, color: HEAD });

// ---- table --------------------------------------------------------------
const HC = (t) => ({ text: t, options: { fill: { color: TEALDK }, color: WHITE, bold: true, align: "center", valign: "middle", fontSize: 11.5 } });
const cell = (t, opts = {}) => ({ text: String(t), options: Object.assign({ align: "center", valign: "middle", fontSize: 11.5, color: SLATE }, opts) });
const label = (t, fill) => ({ text: t, options: { align: "left", valign: "middle", fontSize: 11.5, color: HEAD, bold: true, fill: { color: fill } } });

const rows = [];
rows.push([HC("구성 (A~D)"), HC("총 틱"), HC("평균 틱(ms)"), HC("최고 틱(ms)"), HC("최소 틱(ms)"), HC("큐 소비(ms)"), HC("메모리 MB")]);

// data: [label, ticks, avg, max, min, consume, mem]
const data = [
  ["A. 더블 스왑 · 링 버퍼", "2625", "13.16", "55.87", "7.14", "0.0116", "8 / 9"],
  ["B. 이벤트 큐 · 링 버퍼", "2628", "13.98", "1299.13", "7.17", "0.0140", "8 / 9"],
  ["C. 더블 스왑 · 가변 버퍼", "2624", "12.14", "283.46", "8.15", "0.6805", "9 / 10"],
  ["D. 더블 스왑 · 가변 버퍼", "2643", "25.32", "395.90", "21.65", "0.0092", "8 / 9"],
];
data.forEach((d, i) => {
  const bg = i % 2 === 0 ? WHITE : CARD;
  const row = [label(d[0], bg)];
  for (let c = 1; c < d.length; c++) {
    const opts = { fill: { color: bg } };
    // emphasise the notable outliers
    if (i === 1 && c === 3) { opts.color = RED; opts.bold = true; }      // B 최고 틱 1299ms 스파이크
    if (i === 2 && c === 5) { opts.color = RED; opts.bold = true; }      // C 큐 소비 0.68ms
    row.push(cell(d[c], opts));
  }
  rows.push(row);
});

s.addTable(rows, {
  x: 0.5, y: 1.65, w: 9.0, colW: [2.5, 0.85, 1.1, 1.2, 1.1, 1.15, 1.1],
  rowH: 0.52, fontFace: FONT,
  border: { type: "solid", pt: 0.75, color: BORDER },
  valign: "middle", autoPage: false,
});

// legend / reading guide
s.addText([
  { text: "A ↔ B", options: { bold: true, color: TEALDK } },
  { text: "  전달 큐 비교 (링·그리드 고정)   ", options: { color: SLATE } },
  { text: "A ↔ C", options: { bold: true, color: TEALDK } },
  { text: "  수신 버퍼 비교 (더블 스왑·그리드 고정)", options: { color: SLATE } },
], { x: 0.5, y: 4.55, w: 9.0, h: 0.3, margin: 0, fontFace: FONT, fontSize: 12 });

s.addText([
  { text: "· 최고 틱: 이벤트 큐(B)는 1299ms까지 튐 — 락 경합이 스파이크로 드러남.", options: { breakLine: true } },
  { text: "· 큐 소비 시간: 대부분 0.01ms대로 전체 틱(수 ms)에 묻힘 → 소비 시간을 분리 측정해야 차이가 보임.", options: { breakLine: true } },
  { text: "· 메모리: 8~10MB로 안정적. (표기: 현재 / Peak)", options: {} },
], { x: 0.5, y: 4.9, w: 9.0, h: 0.65, margin: 0, fontFace: FONT, fontSize: 11, color: MUTED, lineSpacingMultiple: 1.05 });

pres.writeFile({ fileName: "benchmark_table.pptx" }).then((f) => console.log("wrote", f));
