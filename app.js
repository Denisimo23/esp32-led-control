// --- BLE Константы (Должны совпадать с прошивкой ESP32-S3) ---
const SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";

// --- Состояние приложения ---
let bleDevice = null;
let bleCharacteristic = null;
let customColors = ["#ff0000", "#ff7f00", "#ffff00", "#00ff00", "#0000ff", "#4b0082", "#9400d3"];

// --- DOM Элементы ---
const btnConnect = document.getElementById('btnConnect');
const statusText = document.getElementById('statusText');
const mainControls = document.getElementById('mainControls');
const paletteContainer = document.getElementById('paletteContainer');

const rangeSensitivity = document.getElementById('rangeSensitivity');
const rangeSpeed = document.getElementById('rangeSpeed');
const rangeBrightness = document.getElementById('rangeBrightness');

// --- Функции рендеринга Конструктора палитры ---
function renderPalette() {
    paletteContainer.innerHTML = '';
    customColors.forEach((color, idx) => {
        const wrapper = document.createElement('div');
        wrapper.className = 'color-picker-wrapper';
        
        const input = document.createElement('input');
        input.type = 'color';
        input.value = color;
        input.addEventListener('change', (e) => {
            customColors[idx] = e.target.value;
        });

        wrapper.appendChild(input);
        paletteContainer.appendChild(wrapper);
    });
}

// --- Управление BLE сессией ---
btnConnect.addEventListener('click', async () => {
    if (bleDevice && bleDevice.gatt.connected) {
        disconnectBLE();
        return;
    }
    await connectBLE();
});

async function connectBLE() {
    try {
        statusText.innerHTML = "Статус: <span>Поиск устройства...</span>";
        bleDevice = await navigator.bluetooth.requestDevice({
            filters: [{ name: 'ESP32-S3 Visualizer' }],
            optionalServices: [SERVICE_UUID]
        });

        statusText.innerHTML = "Статус: <span>Подключение...</span>";
        const server = await bleDevice.gatt.connect();
        const service = await server.getPrimaryService(SERVICE_UUID);
        bleCharacteristic = await service.getCharacteristic(CHARACTERISTIC_UUID);

        bleDevice.addEventListener('gattserverdisconnected', onDisconnected);
        
        statusText.className = "status connected";
        statusText.innerHTML = "Статус: <span>Подключено</span>";
        btnConnect.textContent = "Отключиться";
        btnConnect.classList.add('btn-disconnect');
        mainControls.style.display = "flex";
        
        // Автоматическая отправка текущей палитры при коннекте
        await syncPalette();
    } catch (error) {
        console.error("Ошибка подключения BLE:", error);
        statusText.className = "status disconnected";
        statusText.innerHTML = "Статус: <span>Ошибка соединения</span>";
    }
}

function disconnectBLE() {
    if (bleDevice) {
        bleDevice.gatt.disconnect();
    }
}

function onDisconnected() {
    statusText.className = "status disconnected";
    statusText.innerHTML = "Статус: <span>Отключено</span>";
    btnConnect.textContent = "Подключиться по BLE";
    btnConnect.classList.remove('btn-disconnect');
    mainControls.style.display = "none";
    bleCharacteristic = null;
}

// --- Низкоуровневая передача данных по BLE ---
async function sendString(str) {
    if (!bleCharacteristic) return;
    try {
        const encoder = new TextEncoder();
        await bleCharacteristic.writeValue(encoder.encode(str));
    } catch (e) {
        console.error("Ошибка при отправке пакета в BLE:", e);
    }
}

// --- Обработчики Слайдеров ---
async function sendParam(prefix, value, labelId) {
    document.getElementById(labelId).textContent = value;
    await sendString(`${prefix}:${value}`);
}

rangeSensitivity.addEventListener('input', (e) => sendParam('S', e.target.value, 'valSensitivity'));
rangeSpeed.addEventListener('input', (e) => sendParam('V', e.target.value, 'valSpeed'));
rangeBrightness.addEventListener('input', (e) => sendParam('B', e.target.value, 'valBrightness'));

// --- Переключение Основных режимов (1-4) ---
async function setMode(modeNum) {
    for (let i = 1; i <= 4; i++) {
        const btn = document.getElementById(`modeBtn${i}`);
        if (i === modeNum) btn.classList.add('active');
        else btn.classList.remove('active');
    }
    await sendString(`M:${modeNum}`);
}

for (let i = 1; i <= 4; i++) {
    document.getElementById(`modeBtn${i}`).addEventListener('click', () => setMode(i));
}

// --- Переключение Поведения в тишине (0-3) ---
async function setBgStyle(bgNum) {
    for (let i = 0; i <= 3; i++) {
        const btn = document.getElementById(`bgBtn${i}`);
        if (i === bgNum) btn.classList.add('active');
        else btn.classList.remove('active');
    }
    await sendString(`G:${bgNum}`);
}

for (let i = 0; i <= 3; i++) {
    document.getElementById(`bgBtn${i}`).addEventListener('click', () => setBgStyle(i));
}

// --- Синхронизация и сборка палитры цветов ---
document.getElementById('btnPaletteAdd').addEventListener('click', () => {
    if (customColors.length < 8) {
        customColors.push("#ffffff");
        renderPalette();
    }
});

document.getElementById('btnPaletteReset').addEventListener('click', () => {
    customColors = ["#ff0000"];
    renderPalette();
});

async function syncPalette() {
    if (!bleCharacteristic) return;
    // Форматируем массив HEX-цветов строк под парсер "C:0xFF0000,0x00FF00"
    const payload = "C:" + customColors.map(c => "0x" + c.substring(1)).join(",");
    await sendString(payload);
}

document.getElementById('btnPaletteSync').addEventListener('click', syncPalette);

// --- Первичный запуск ---
renderPalette();
