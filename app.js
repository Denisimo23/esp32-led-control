const SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";

let bleDevice = null;
let bleCharacteristic = null;
let activeMode = 1;

// Метаданные режимов для гибкой перестройки интерфейса
const modeMetadata = {
    1: { name: "Классический VU", sub: ["От края ленты", "Из центра к краям"], opt: null },
    2: { name: "Фиксация пика", sub: ["Падающая точка", "Растворение пика"], opt: null },
    3: { name: "Встречные столбики", sub: ["С краев к центру", "Из центра к краям"], opt: null },
    4: { name: "Радужный эффект", sub: ["Бегущий поток", "Амплитудный бит"], opt: null },
    5: { name: "Эффект Огня", sub: ["Классическое пламя", "Кастомная палитра"], opt: "Охлаждение (затухание)" },
    6: { name: "Цветомузыка", sub: ["Резкие вспышки", "Плавное угасание"], opt: null },
    7: { name: "Плазма / Жидкий свет", sub: ["Постоянный темп", "Частотный разгон"], opt: "Яркость в тишине" },
    8: { name: "Стробоскоп", sub: ["Статичный/Палитра", "Случайная смена"], opt: null },
    9: { name: "Искры / Небо", sub: ["Мерцание на месте", "Бегущие метеоры"], opt: "Скорость угасания" },
    10: { name: "Бегущая волна", sub: ["Симметрия из центра", "Направленный поток"], opt: null }
};

// Виртуальное кэширование структуры настроек для мгновенного отклика UI
const modeCache = {};
for(let i=1; i<=10; i++) {
    modeCache[i] = {
        subMode: 0,
        sensitivity: 50,
        speed: 30,
        option: 50,
        colors: ["#ff0000", "#00ff00", "#0000ff", "#ffff00"]
    };
}

// DOM элементы
const btnConnect = document.getElementById('btnConnect');
const statusText = document.getElementById('statusText');
const mainControls = document.getElementById('mainControls');
const modesContainer = document.getElementById('modesContainer');
const subModesContainer = document.getElementById('subModesContainer');
const paletteContainer = document.getElementById('paletteContainer');

// Генерация кнопок 10 режимов
function initModesGrid() {
    modesContainer.innerHTML = '';
    for (let i = 1; i <= 10; i++) {
        const btn = document.createElement('button');
        btn.className = `btn-mode ${i === activeMode ? 'active' : ''}`;
        btn.id = `mBtn${i}`;
        btn.innerHTML = `${i}. ${modeMetadata[i].name}`;
        btn.onclick = () => switchMode(i);
        modesContainer.appendChild(btn);
    }
}

// Смена режима и перестройка интерфейса
async function switchMode(modeNum) {
    document.getElementById(`mBtn${activeMode}`).classList.remove('active');
    activeMode = modeNum;
    document.getElementById(`mBtn${activeMode}`).classList.add('active');

    const meta = modeMetadata[activeMode];
    const cache = modeCache[activeMode];

    document.getElementById('lblActiveModeName').textContent = meta.name;

    // Перестройка субрежимов
    subModesContainer.innerHTML = '';
    meta.sub.forEach((subName, idx) => {
        const btn = document.createElement('button');
        btn.className = `btn-mode ${idx === cache.subMode ? 'active' : ''}`;
        btn.id = `subBtn${idx}`;
        btn.textContent = subName;
        btn.onclick = () => changeSubMode(idx);
        subModesContainer.appendChild(btn);
    });

    // Обновление ползунков
    document.getElementById('rangeSensitivity').value = cache.sensitivity;
    document.getElementById('valSensitivity').textContent = cache.sensitivity;
    document.getElementById('rangeSpeed').value = cache.speed;
    document.getElementById('valSpeed').textContent = cache.speed;

    // Настройка видимости опции
    const groupOpt = document.getElementById('groupOption');
    if (meta.opt) {
        groupOpt.style.display = "flex";
        document.getElementById('lblOption').firstChild.textContent = meta.opt + " ";
        document.getElementById('rangeOption').value = cache.option;
        document.getElementById('valOption').textContent = cache.option;
    } else {
        groupOpt.style.display = "none";
    }

    renderPalette();
    await sendString(`M:${activeMode}`);
}

function changeSubMode(subIdx) {
    const currentActive = subModesContainer.querySelector('.active');
    if(currentActive) currentActive.classList.remove('active');
    document.getElementById(`subBtn${subIdx}`).classList.add('active');
    
    modeCache[activeMode].subMode = subIdx;
    sendString(`S:${subIdx}`);
}

// Отрисовка ячеек палитры цвета
function renderPalette() {
    paletteContainer.innerHTML = '';
    const colors = modeCache[activeMode].colors;
    
    colors.forEach((color, idx) => {
        const slot = document.createElement('div');
        slot.className = 'color-slot';
        slot.innerHTML = `<input type="color" value="${color}">`;
        slot.querySelector('input').onchange = (e) => {
            modeCache[activeMode].colors[idx] = e.target.value;
        };
        paletteContainer.appendChild(slot);
    });
}

// Слушатели событий ползунков
async function sendString(str) {
    if (!bleCharacteristic) return;
    try {
        await bleCharacteristic.writeValue(new TextEncoder().encode(str));
    } catch (e) { console.error("Ошибка BLE передачи:", e); }
}

document.getElementById('rangeGlobalBrightness').oninput = (e) => {
    document.getElementById('valGlobalBrightness').textContent = e.target.value;
    sendString(`B:${e.target.value}`);
};
document.getElementById('rangeSensitivity').oninput = (e) => {
    modeCache[activeMode].sensitivity = e.target.value;
    document.getElementById('valSensitivity').textContent = e.target.value;
    sendString(`T:${e.target.value}`); // T - Sensitivity в прошивке
};
document.getElementById('rangeSpeed').oninput = (e) => {
    modeCache[activeMode].speed = e.target.value;
    document.getElementById('valSpeed').textContent = e.target.value;
    sendString(`V:${e.target.value}`);
};
document.getElementById('rangeOption').oninput = (e) => {
    modeCache[activeMode].option = e.target.value;
    document.getElementById('valOption').textContent = e.target.value;
    sendString(`O:${e.target.value}`);
};

// Палитра: Добавление, Сброс, Синхронизация
document.getElementById('btnColorAdd').onclick = () => {
    if(modeCache[activeMode].colors.length < 4) {
        modeCache[activeMode].colors.push("#ffffff");
        renderPalette();
    }
};
document.getElementById('btnColorReset').onclick = () => {
    modeCache[activeMode].colors = ["#ff0000"];
    renderPalette();
};
document.getElementById('btnColorSync').onclick = async () => {
    const hexColors = modeCache[activeMode].colors.map(c => "0x" + c.substring(1)).join(",");
    await sendString(`C:${hexColors}`);
};

// BLE логика подключения
btnConnect.onclick = async () => {
    if (bleDevice && bleDevice.gatt.connected) {
        bleDevice.gatt.disconnect();
        return;
    }
    try {
        statusText.innerHTML = "Статус: <span>Поиск...</span>";
        bleDevice = await navigator.bluetooth.requestDevice({
            filters: [{ name: 'ESP32-S3 Visualizer' }],
            optionalServices: [SERVICE_UUID]
        });
        statusText.innerHTML = "Статус: <span>Соединение...</span>";
        const server = await bleDevice.gatt.connect();
        const service = await server.getPrimaryService(SERVICE_UUID);
        bleCharacteristic = await service.getCharacteristic(CHARACTERISTIC_UUID);
        
        bleDevice.addEventListener('gattserverdisconnected', onDisconnected);
        statusText.className = "status connected";
        statusText.innerHTML = "Статус: <span>Подключено (Память активна)</span>";
        btnConnect.textContent = "Отключить связь";
        btnConnect.classList.add('btn-disconnect');
        mainControls.style.display = "flex";
        switchMode(1);
    } catch (e) {
        statusText.className = "status disconnected";
        statusText.innerHTML = "Статус: <span>Ошибка</span>";
    }
};

function onDisconnected() {
    statusText.className = "status disconnected";
    statusText.innerHTML = "Статус: <span>Связь разорвана</span>";
    btnConnect.textContent = "Подключиться к ESP32";
    btnConnect.classList.remove('btn-disconnect');
    mainControls.style.display = "none";
    bleCharacteristic = null;
}

// Запуск инициализации структуры интерфейса
initModesGrid();
