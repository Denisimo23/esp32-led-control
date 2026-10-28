/* ============================================================
   Ceiling Light — Web Bluetooth controller
   ESP32-S3 + FastLED + 48 режимов
   ============================================================ */

const SERVICE_UUID = "a1b2c3d4-e5f6-4444-8888-123456789abc";

const CHAR_UUIDS = {
  mode:      "a1b2c3d4-e5f6-4444-8888-123456789abd",
  state:     "a1b2c3d4-e5f6-4444-8888-123456789abe",
  color:     "a1b2c3d4-e5f6-4444-8888-123456789abf",
  speed:     "a1b2c3d4-e5f6-4444-8888-123456789ac0",
  mic:       "a1b2c3d4-e5f6-4444-8888-123456789ac1",
  notify:    "a1b2c3d4-e5f6-4444-8888-123456789ac2",
  brightness:"a1b2c3d4-e5f6-4444-8888-123456789ac3",
  palette:   "a1b2c3d4-e5f6-4444-8888-123456789ac4",
  live:      "a1b2c3d4-e5f6-4444-8888-123456789ac5",
  info:      "a1b2c3d4-e5f6-4444-8888-123456789ac6"
};

const MODES = [
  "Выключено",
  "Белый",
  "Тёплый белый",
  "Красный",
  "Зелёный",
  "Синий",
  "Жёлтый",
  "Фиолетовый",
  "Бирюзовый",
  "Свой цвет (HSV)",
  "Палитра (цвет)",
  "Градиент палитры",
  "Радуга",
  "Радуга бегущая",
  "Цветовая заливка",
  "Дыхание",
  "Свеча",
  "Огонь",
  "Океанская волна",
  "Метеор",
  "Искры",
  "Плазма",
  "Звук: VU-метр",
  "Звук: вспышка бита",
  "Звук: спектр",
  "Звук: радуга басов",
  "Звук: стробоскоп",
  "Звук: каскад",
  "Звук: волна",
  "Звук: диск",
  "Звук: огонь",
  "Звук: пульс",
  "Звук: круги",
  "Звук: зеркальные бары",
  "Звук: цветомузыка",
  "Звук: палитра бита",
  "Новый год",
  "Хэллоуин",
  "Романтика",
  "Дискотека",
  "Океан",
  "Лес",
  "Космос",
  "Полиция",
  "Молния",
  "Рассвет",
  "Северное сияние",
  "Матрица"
];

const PALETTES = [
  { name: "Радуга", colors: ["#ff0000", "#ffff00", "#00ff00", "#00ffff", "#0000ff", "#ff00ff", "#ff0000"] },
  { name: "Огонь", colors: ["#000000", "#7a0000", "#ff3000", "#ffd000", "#ffffff"] },
  { name: "Океан", colors: ["#001450", "#0050b4", "#00a8d2", "#96dcff"] },
  { name: "Лес", colors: ["#071f0a", "#0f5a18", "#35a03c", "#a0e08a"] },
  { name: "Лава", colors: ["#000000", "#6e0000", "#ff4a00", "#ffff40"] },
  { name: "Облака", colors: ["#0000ff", "#4060ff", "#ffffff", "#4060ff"] },
  { name: "Вечеринка", colors: ["#ff00ff", "#00ffff", "#ffff00", "#ff0000", "#00ff00"] },
  { name: "Радужные полосы", colors: ["#ff0000", "#ffff00", "#00ff00", "#00ffff", "#0000ff", "#ff00ff", "#ff0000"] },
  { name: "Осень", colors: ["#6e1400", "#d23c00", "#ffaa14", "#8c1e00"] },
  { name: "Новый год", colors: ["#c80000", "#00b428", "#ffc828", "#c80000"] },
  { name: "Валентин", colors: ["#780014", "#ff146e", "#ffbeda", "#ff146e"] },
  { name: "Неон", colors: ["#00ffff", "#ff00ff", "#ffff00", "#00ffff"] },
  { name: "Своя палитра", colors: ["#ff2828", "#28ff50", "#2878ff"] }
];

const PRESETS = [
  { mode: 12, name: "Радуга", colors: ["#ff2d55", "#ffcc00", "#00e5ff"] },
  { mode: 17, name: "Огонь", colors: ["#7a0000", "#ff5a00", "#ffd000"] },
  { mode: 18, name: "Волна", colors: ["#001450", "#00a8d2", "#96dcff"] },
  { mode: 15, name: "Дыхание", colors: ["#35f0c0", "#4e8cff", "#ff3d81"] },
  { mode: 22, name: "VU-метр", colors: ["#35f0c0", "#ffd25c", "#ff3d81"] },
  { mode: 24, name: "Спектр", colors: ["#00ffff", "#4e8cff", "#ff3d81"] },
  { mode: 39, name: "Дискотека", colors: ["#ff00ff", "#00ffff", "#ffff00"] },
  { mode: 42, name: "Космос", colors: ["#05051e", "#501478", "#c864ff"] },
  { mode: 47, name: "Матрица", colors: ["#001a00", "#00a020", "#80ff80"] }
];

const $ = (id) => document.getElementById(id);

let device = null;
let server = null;
let chars = {};
let connected = false;
let lastUserAction = 0;
const pendingWrites = new Map();
const writeTimers = new Map();

/* ======================= UI HELPERS ======================= */

function markUserAction() {
  lastUserAction = Date.now();
}

function setStatus(kind, text) {
  const el = $("status");
  el.className = "status" + (kind ? " " + kind : "");
  $("statusText").textContent = text;
}

let toastTimer = 0;
function toast(text) {
  const el = $("toast");
  el.textContent = text;
  el.classList.add("show");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => el.classList.remove("show"), 2600);
}

function setFill(el) {
  const min = Number(el.min) || 0;
  const max = Number(el.max) || 255;
  const value = Number(el.value);
  const p = max === min ? 0 : ((value - min) / (max - min)) * 100;
  el.style.setProperty("--p", Math.max(0, Math.min(100, p)).toFixed(1) + "%");
}

function groupName(mode) {
  if (mode === 0) return "Выключено";
  if (mode <= 9) return "Статичные";
  if (mode <= 21) return "Динамические";
  if (mode <= 35) return "Звуковые";
  return "Особые";
}

/* ======================= COLOR ======================= */

function clamp(v, a, b) {
  return Math.max(a, Math.min(b, v));
}

function hexToRgb(hex) {
  const m = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  if (!m) return { r: 0, g: 0, b: 0 };
  return { r: parseInt(m[1], 16), g: parseInt(m[2], 16), b: parseInt(m[3], 16) };
}

function rgbToHex(r, g, b) {
  const n = (1 << 24) + (clamp(r, 0, 255) << 16) + (clamp(g, 0, 255) << 8) + clamp(b, 0, 255);
  return "#" + n.toString(16).slice(1);
}

function hsvToRgb(h, s, v) {
  h = (h / 255) * 6;
  s = s / 255;
  v = v / 255;
  const i = Math.floor(h);
  const f = h - i;
  const p = v * (1 - s);
  const q = v * (1 - f * s);
  const t = v * (1 - (1 - f) * s);
  let r, g, b;
  switch (i % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }
  return { r: Math.round(r * 255), g: Math.round(g * 255), b: Math.round(b * 255) };
}

function rgbToHsv(r, g, b) {
  r /= 255; g /= 255; b /= 255;
  const max = Math.max(r, g, b);
  const min = Math.min(r, g, b);
  const d = max - min;
  let h = 0;
  if (d !== 0) {
    if (max === r) h = ((g - b) / d + (g < b ? 6 : 0)) / 6;
    else if (max === g) h = ((b - r) / d + 2) / 6;
    else h = ((r - g) / d + 4) / 6;
  }
  const s = max === 0 ? 0 : d / max;
  return {
    h: Math.round(h * 255),
    s: Math.round(s * 255),
    v: Math.round(max * 255)
  };
}

function hsvToHex(h, s, v) {
  const c = hsvToRgb(h, s, v);
  return rgbToHex(c.r, c.g, c.b);
}

/* ======================= BLE WRITE QUEUE ======================= */

async function flushWrite(name, fast) {
  if (!connected || !chars[name]) return;
  const payload = pendingWrites.get(name);
  if (!payload) return;
  pendingWrites.delete(name);

  const ch = chars[name];
  try {
    if (fast && ch.writeValueWithoutResponse) {
      await ch.writeValueWithoutResponse(payload);
    } else {
      await ch.writeValue(payload);
    }
  } catch (err) {
    try {
      await ch.writeValue(payload);
    } catch (err2) {
      console.warn("BLE write failed:", name, err2);
      toast("Не удалось отправить команду");
    }
  }
}

function queueWrite(name, bytes, { fast = false, immediate = false } = {}) {
  if (!connected || !chars[name]) return;
  pendingWrites.set(name, bytes);

  if (immediate) {
    const t = writeTimers.get(name);
    if (t) clearTimeout(t);
    writeTimers.delete(name);
    flushWrite(name, fast);
    return;
  }

  if (writeTimers.has(name)) return;
  writeTimers.set(name, setTimeout(() => {
    writeTimers.delete(name);
    flushWrite(name, fast);
  }, 45));
}

/* ======================= SEND COMMANDS ======================= */

function sendMode(mode) {
  queueWrite("mode", new Uint8Array([mode & 0xff]), { immediate: true });
}

function sendPower(on) {
  queueWrite("state", new Uint8Array([on ? 1 : 0]), { immediate: true });
}

function sendSpeed(value) {
  queueWrite("speed", new Uint8Array([clamp(value, 1, 255)]), { fast: true });
}

function sendBrightness(value) {
  queueWrite("brightness", new Uint8Array([clamp(value, 0, 255)]), { fast: true });
}

function sendMic() {
  const mode = Number($("micMode").value);
  const sens = Number($("micSens").value);
  queueWrite("mic", new Uint8Array([mode, sens]), { fast: true });
}

function sendColor(fast = true) {
  const h = Number($("hue").value);
  const s = Number($("sat").value);
  const v = Number($("val").value);
  queueWrite("color", new Uint8Array([h, s, v]), { fast });
}

function sendPalette(fast = true) {
  const id = Number($("paletteSelect").value);
  const shift = Number($("hueShift").value);
  const c1 = hexToRgb($("c1").value);
  const c2 = hexToRgb($("c2").value);
  const c3 = hexToRgb($("c3").value);
  const data = new Uint8Array([
    id, shift,
    c1.r, c1.g, c1.b,
    c2.r, c2.g, c2.b,
    c3.r, c3.g, c3.b
  ]);
  queueWrite("palette", data, { fast });
}

/* ======================= UI STATE ======================= */

function updateModeUI(mode) {
  $("modeTitle").textContent = MODES[mode] || "—";
  $("modeGroup").textContent = groupName(mode);
  $("modeSelect").value = String(mode);
  document.querySelectorAll(".preset").forEach((b) => {
    b.classList.toggle("active", Number(b.dataset.mode) === mode);
  });
}

function updateColorUI(send = true) {
  const h = Number($("hue").value);
  const s = Number($("sat").value);
  const v = Number($("val").value);
  const hex = hsvToHex(h, s, v);

  $("hueVal").textContent = h;
  $("satVal").textContent = s;
  $("valVal").textContent = v;
  $("colorPicker").value = hex;
  $("colorPreview").style.background = hex;
  $("colorHex").textContent = hex.toUpperCase();

  setFill($("hue"));
  setFill($("sat"));
  setFill($("val"));

  if (send) sendColor(true);
}

function updatePaletteUI(send = true, fast = true) {
  const id = Number($("paletteSelect").value);
  const palette = PALETTES[id] || PALETTES[0];

  $("paletteBadge").textContent = palette.name;
  $("customPalette").classList.toggle("hidden", id !== 12);

  let colors = palette.colors;
  if (id === 12) {
    colors = [$("c1").value, $("c2").value, $("c3").value, $("c1").value];
  }
  $("paletteStrip").style.background = "linear-gradient(90deg, " + colors.join(", ") + ")";

  $("shiftVal").textContent = $("hueShift").value;
  setFill($("hueShift"));

  if (send) sendPalette(fast);
}

function updateMicUI(send = true) {
  $("micSensVal").textContent = $("micSens").value;
  setFill($("micSens"));
  if (send) sendMic();
}

function updateSpeedUI(send = true) {
  $("speedVal").textContent = $("speed").value;
  setFill($("speed"));
  if (send) sendSpeed(Number($("speed").value));
}

function updateBrightnessUI(send = true) {
  $("brightnessVal").textContent = $("brightness").value;
  setFill($("brightness"));
  if (send) sendBrightness(Number($("brightness").value));
}

/* ======================= APPLY FROM DEVICE ======================= */

function applySingle(name, view) {
  switch (name) {
    case "mode":
      updateModeUI(view.getUint8(0));
      break;
    case "state":
      $("power").checked = view.getUint8(0) !== 0;
      break;
    case "color":
      $("hue").value = view.getUint8(0);
      $("sat").value = view.getUint8(1);
      $("val").value = view.getUint8(2);
      updateColorUI(false);
      break;
    case "speed":
      $("speed").value = view.getUint8(0);
      updateSpeedUI(false);
      break;
    case "brightness":
      $("brightness").value = view.getUint8(0);
      updateBrightnessUI(false);
      break;
    case "mic":
      $("micMode").value = String(view.getUint8(0));
      $("micSens").value = view.getUint8(1);
      updateMicUI(false);
      break;
    case "palette":
      if (view.byteLength >= 11) {
        $("paletteSelect").value = String(view.getUint8(0));
        $("hueShift").value = view.getUint8(1);
        $("c1").value = rgbToHex(view.getUint8(2), view.getUint8(3), view.getUint8(4));
        $("c2").value = rgbToHex(view.getUint8(5), view.getUint8(6), view.getUint8(7));
        $("c3").value = rgbToHex(view.getUint8(8), view.getUint8(9), view.getUint8(10));
        updatePaletteUI(false);
      }
      break;
  }
}

function applySnapshot(view) {
  if (view.byteLength < 12) return;
  updateModeUI(view.getUint8(1));
  $("power").checked = view.getUint8(2) !== 0;
  $("brightness").value = view.getUint8(3);
  $("speed").value = view.getUint8(4);
  $("hue").value = view.getUint8(5);
  $("sat").value = view.getUint8(6);
  $("val").value = view.getUint8(7);
  $("micMode").value = String(view.getUint8(8));
  $("micSens").value = view.getUint8(9);
  $("paletteSelect").value = String(view.getUint8(10));
  $("hueShift").value = view.getUint8(11);

  updateBrightnessUI(false);
  updateSpeedUI(false);
  updateColorUI(false);
  updateMicUI(false);
  updatePaletteUI(false);
}

function onStateNotify(event) {
  // Не дёргаем ползунки, пока пользователь сам ими управляет.
  if (Date.now() - lastUserAction < 900) return;
  applySnapshot(event.target.value);
}

function onLiveNotify(event) {
  const view = event.target.value;
  if (view.byteLength < 1) return;
  updateVU(view.getUint8(0));
}

/* ======================= VU METER ======================= */

let vuPeak = 0;
function updateVU(level) {
  const pct = clamp(level / 255 * 100, 0, 100);
  $("vuLevel").style.width = pct.toFixed(1) + "%";
  $("vuValue").textContent = Math.round(pct) + "%";

  if (pct >= vuPeak) {
    vuPeak = pct;
    const peak = $("vuPeak");
    peak.style.left = "calc(" + pct.toFixed(1) + "% - 1px)";
    peak.style.opacity = "1";
  }
}

setInterval(() => {
  if (vuPeak > 0) {
    vuPeak = Math.max(0, vuPeak - 1.2);
    const peak = $("vuPeak");
    peak.style.left = "calc(" + vuPeak.toFixed(1) + "% - 1px)";
    if (vuPeak <= 0) peak.style.opacity = "0";
  }
}, 90);

/* ======================= CONNECT ======================= */

async function subscribe(ch, handler) {
  if (!ch) return false;
  try {
    await ch.startNotifications();
    ch.addEventListener("characteristicvaluechanged", handler);
    return true;
  } catch (err) {
    console.warn("Notify subscribe failed:", err);
    return false;
  }
}

async function readAll() {
  const order = ["mode", "state", "color", "speed", "brightness", "mic", "palette"];
  for (const name of order) {
    if (!chars[name]) continue;
    try {
      const view = await chars[name].readValue();
      applySingle(name, view);
    } catch (err) {
      console.warn("Read failed:", name, err);
    }
  }

  if (chars.info) {
    try {
      const view = await chars.info.readValue();
      const text = new TextDecoder().decode(view);
      $("fwInfo").textContent = text;
    } catch (err) {
      console.warn(err);
    }
  }
}

async function connect() {
  if (!navigator.bluetooth) {
    toast("Этот браузер не поддерживает Web Bluetooth");
    return;
  }

  try {
    setStatus("busy", "Поиск...");
    device = await navigator.bluetooth.requestDevice({
      filters: [{ namePrefix: "CeilingLight" }],
      optionalServices: [SERVICE_UUID]
    });

    device.addEventListener("gattserverdisconnected", onDisconnected);

    setStatus("busy", "Подключение...");
    server = await device.gatt.connect();
    const service = await server.getPrimaryService(SERVICE_UUID);

    chars = {};
    for (const [name, uuid] of Object.entries(CHAR_UUIDS)) {
      try {
        chars[name] = await service.getCharacteristic(uuid);
      } catch (err) {
        console.warn("No characteristic:", name, err);
      }
    }

    if (!chars.mode || !chars.state || !chars.color) {
      throw new Error("Не найдены характеристики. Проверь прошивку.");
    }

    await subscribe(chars.notify, onStateNotify);
    await subscribe(chars.live, onLiveNotify);
    await readAll();

    connected = true;
    $("btnConnect").hidden = true;
    $("btnDisconnect").hidden = false;
    $("controls").classList.remove("hidden");
    setStatus("on", "Подключено");
    toast("Лампа на связи");
  } catch (err) {
    console.error(err);
    connected = false;
    server = null;
    setStatus("err", "Ошибка");
    $("btnConnect").disabled = false;
    toast(err && err.message ? err.message : "Не удалось подключиться");
  }
}

async function disconnect() {
  try {
    if (server && server.connected) server.disconnect();
  } catch (err) {
    console.warn(err);
  }
  onDisconnected();
}

function onDisconnected() {
  connected = false;
  server = null;
  chars = {};
  pendingWrites.clear();
  writeTimers.forEach((t) => clearTimeout(t));
  writeTimers.clear();
  device = null;

  $("btnConnect").hidden = false;
  $("btnConnect").disabled = false;
  $("btnDisconnect").hidden = true;
  $("controls").classList.add("hidden");
  setStatus("", "Не подключено");
  updateVU(0);
}

/* ======================= BUILD UI ======================= */

function buildModeSelect() {
  const sel = $("modeSelect");
  const off = document.createElement("option");
  off.value = "0";
  off.textContent = "Выключено";
  sel.appendChild(off);

  const groups = [
    { label: "Статичные", from: 1, to: 9 },
    { label: "Динамические", from: 10, to: 21 },
    { label: "Звуковые", from: 22, to: 35 },
    { label: "Особые", from: 36, to: 47 }
  ];

  for (const g of groups) {
    const og = document.createElement("optgroup");
    og.label = g.label;
    for (let i = g.from; i <= g.to; i++) {
      const o = document.createElement("option");
      o.value = String(i);
      o.textContent = i + " · " + MODES[i];
      og.appendChild(o);
    }
    sel.appendChild(og);
  }
}

function buildPresets() {
  const wrap = $("presets");
  for (const p of PRESETS) {
    const b = document.createElement("button");
    b.type = "button";
    b.className = "preset";
    b.dataset.mode = String(p.mode);
    b.innerHTML =
      '<span class="ico" style="background:linear-gradient(135deg,' +
      p.colors.join(",") +
      ')"></span><span>' + p.name + "</span>";
    b.addEventListener("click", () => {
      updateModeUI(p.mode);
      sendMode(p.mode);
      if (!$("power").checked) {
        $("power").checked = true;
        sendPower(true);
      }
    });
    wrap.appendChild(b);
  }
}

function buildPaletteSelect() {
  const sel = $("paletteSelect");
  PALETTES.forEach((p, i) => {
    const o = document.createElement("option");
    o.value = String(i);
    o.textContent = p.name;
    sel.appendChild(o);
  });
}

/* ======================= EVENTS ======================= */

$("controls").addEventListener("pointerdown", markUserAction, true);
$("controls").addEventListener("input", markUserAction, true);

$("btnConnect").addEventListener("click", connect);
$("btnDisconnect").addEventListener("click", disconnect);

$("power").addEventListener("change", (e) => {
  const on = e.target.checked;
  sendPower(on);
  if (on && Number($("modeSelect").value) === 0) {
    updateModeUI(1);
    sendMode(1);
  }
});

$("modeSelect").addEventListener("change", (e) => {
  const mode = Number(e.target.value);
  updateModeUI(mode);
  sendMode(mode);
  $("power").checked = mode !== 0;
});

$("btnRandom").addEventListener("click", () => {
  const mode = 1 + Math.floor(Math.random() * (MODES.length - 1));
  updateModeUI(mode);
  sendMode(mode);
  $("power").checked = true;
  sendPower(true);
});

$("btnSync").addEventListener("click", async () => {
  if (!connected) return;
  await readAll();
  toast("Настройки синхронизированы");
});

$("btnAllOff").addEventListener("click", () => {
  $("power").checked = false;
  updateModeUI(0);
  sendMode(0);
  sendPower(false);
});

$("brightness").addEventListener("input", () => {
  updateBrightnessUI(true);
});

$("speed").addEventListener("input", () => {
  updateSpeedUI(true);
});

$("hue").addEventListener("input", () => updateColorUI(true));
$("sat").addEventListener("input", () => updateColorUI(true));
$("val").addEventListener("input", () => updateColorUI(true));

$("colorPicker").addEventListener("input", (e) => {
  const rgb = hexToRgb(e.target.value);
  const hsv = rgbToHsv(rgb.r, rgb.g, rgb.b);
  $("hue").value = hsv.h;
  $("sat").value = hsv.s;
  $("val").value = hsv.v;
  updateColorUI(true);
});

$("paletteSelect").addEventListener("change", () => {
  updatePaletteUI(true, false);
});

$("hueShift").addEventListener("input", () => {
  updatePaletteUI(true, true);
});

["c1", "c2", "c3"].forEach((id) => {
  $(id).addEventListener("input", () => {
    updatePaletteUI(true, true);
  });
});

$("micMode").addEventListener("change", () => updateMicUI(true));
$("micSens").addEventListener("input", () => updateMicUI(true));

/* ======================= INIT ======================= */

function init() {
  buildModeSelect();
  buildPresets();
  buildPaletteSelect();

  updateModeUI(12);
  updateBrightnessUI(false);
  updateSpeedUI(false);
  updateColorUI(false);
  updateMicUI(false);
  updatePaletteUI(false);

  if (!navigator.bluetooth) {
    $("btnConnect").disabled = true;
    $("hint").textContent =
      "Web Bluetooth не поддерживается. Открой сайт в Chrome/Edge на Android. На iPhone — через браузер Bluefy.";
    $("hint").classList.add("warn");
    setStatus("err", "Нет Bluetooth");
  } else if (!window.isSecureContext) {
    $("hint").textContent = "Нужен HTTPS (GitHub Pages) или localhost. Web Bluetooth не работает по HTTP.";
    $("hint").classList.add("warn");
  }
}

init();
