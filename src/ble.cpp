#include "ble.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include "audio.h"
#include "config.h"
#include "state.h"

static const char* SERVICE_UUID   = "a1b2c3d4-e5f6-4444-8888-123456789abc";
static const char* CHAR_MODE_UUID = "a1b2c3d4-e5f6-4444-8888-123456789abd";
static const char* CHAR_STATE_UUID= "a1b2c3d4-e5f6-4444-8888-123456789abe";
static const char* CHAR_COLOR_UUID= "a1b2c3d4-e5f6-4444-8888-123456789abf";
static const char* CHAR_SPEED_UUID= "a1b2c3d4-e5f6-4444-8888-123456789ac0";
static const char* CHAR_MIC_UUID  = "a1b2c3d4-e5f6-4444-8888-123456789ac1";
static const char* CHAR_NOTIFY_UUID = "a1b2c3d4-e5f6-4444-8888-123456789ac2";
static const char* CHAR_BRIGHT_UUID = "a1b2c3d4-e5f6-4444-8888-123456789ac3";
static const char* CHAR_PALETTE_UUID= "a1b2c3d4-e5f6-4444-8888-123456789ac4";
static const char* CHAR_LIVE_UUID   = "a1b2c3d4-e5f6-4444-8888-123456789ac5";
static const char* CHAR_INFO_UUID   = "a1b2c3d4-e5f6-4444-8888-123456789ac6";

static volatile bool connected = false;
static bool needAdvertising = false;
static uint32_t disconnectMs = 0;

static BLEServer* pServer = nullptr;
static BLECharacteristic* pMode = nullptr;
static BLECharacteristic* pState = nullptr;
static BLECharacteristic* pColor = nullptr;
static BLECharacteristic* pSpeed = nullptr;
static BLECharacteristic* pMic = nullptr;
static BLECharacteristic* pNotify = nullptr;
static BLECharacteristic* pBright = nullptr;
static BLECharacteristic* pPalette = nullptr;
static BLECharacteristic* pLive = nullptr;

static void setU8(BLECharacteristic* ch, uint8_t value) {
  ch->setValue(&value, 1);
}

static void updateCharValues() {
  setU8(pMode, state.mode);
  setU8(pState, state.on);
  setU8(pSpeed, state.speed);
  setU8(pBright, state.brightness);

  uint8_t col[3] = {state.hue, state.sat, state.val};
  pColor->setValue(col, sizeof(col));

  uint8_t mic[2] = {state.micMode, state.micSens};
  pMic->setValue(mic, sizeof(mic));

  uint8_t pal[11] = {
      state.paletteId, state.hueShift,
      state.c1r, state.c1g, state.c1b,
      state.c2r, state.c2g, state.c2b,
      state.c3r, state.c3g, state.c3b
  };
  pPalette->setValue(pal, sizeof(pal));
}

void bleNotifyState() {
  if (!pNotify) return;

  uint8_t snapshot[12] = {
      1,
      state.mode,
      state.on,
      state.brightness,
      state.speed,
      state.hue,
      state.sat,
      state.val,
      state.micMode,
      state.micSens,
      state.paletteId,
      state.hueShift
  };
  pNotify->setValue(snapshot, sizeof(snapshot));
  if (connected) pNotify->notify();
}

bool bleIsConnected() {
  return connected;
}

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) override {
    (void)server;
    connected = true;
    needAdvertising = false;
    Serial.println("BLE: connected");
  }

  void onDisconnect(BLEServer* server) override {
    (void)server;
    connected = false;
    needAdvertising = true;
    disconnectMs = millis();
    Serial.println("BLE: disconnected");
  }
};

class WriteCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* ch) override {
    String value = ch->getValue();
    if (value.length() == 0) return;

    if (ch == pMode) {
      uint8_t mode = (uint8_t)value[0];
      if (mode < MODE_COUNT) {
        state.mode = mode;
        state.on = mode ? 1 : 0;
      }
    } else if (ch == pState) {
      state.on = value[0] ? 1 : 0;
      if (state.on && state.mode == 0) state.mode = 1;
    } else if (ch == pColor) {
      if (value.length() >= 3) {
        state.hue = (uint8_t)value[0];
        state.sat = (uint8_t)value[1];
        state.val = (uint8_t)value[2];
      }
    } else if (ch == pSpeed) {
      state.speed = (uint8_t)value[0];
      if (state.speed == 0) state.speed = 1;
    } else if (ch == pBright) {
      state.brightness = (uint8_t)value[0];
    } else if (ch == pMic) {
      if (value.length() >= 2) {
        state.micMode = (uint8_t)value[0];
        if (state.micMode > MIC_SPECTRUM) state.micMode = MIC_ENVELOPE;
        state.micSens = (uint8_t)value[1];
      }
    } else if (ch == pPalette) {
      if (value.length() >= 11) {
        uint8_t id = (uint8_t)value[0];
        if (id < PALETTE_COUNT) state.paletteId = id;
        state.hueShift = (uint8_t)value[1];
        state.c1r = (uint8_t)value[2];
        state.c1g = (uint8_t)value[3];
        state.c1b = (uint8_t)value[4];
        state.c2r = (uint8_t)value[5];
        state.c2g = (uint8_t)value[6];
        state.c2b = (uint8_t)value[7];
        state.c3r = (uint8_t)value[8];
        state.c3g = (uint8_t)value[9];
        state.c3b = (uint8_t)value[10];
      }
    }

    updateCharValues();
    stateMarkDirty();
    bleNotifyState();
  }
};

void bleInit() {
  BLEDevice::init(BLE_DEVICE_NAME);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* service = pServer->createService(SERVICE_UUID);

  const uint32_t rw = BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_WRITE_NR |
                      BLECharacteristic::PROPERTY_NOTIFY;

  pMode = service->createCharacteristic(CHAR_MODE_UUID, rw);
  pState = service->createCharacteristic(CHAR_STATE_UUID, rw);
  pColor = service->createCharacteristic(CHAR_COLOR_UUID, rw);
  pSpeed = service->createCharacteristic(CHAR_SPEED_UUID, rw);
  pMic = service->createCharacteristic(CHAR_MIC_UUID, rw);
  pBright = service->createCharacteristic(CHAR_BRIGHT_UUID, rw);
  pPalette = service->createCharacteristic(CHAR_PALETTE_UUID, rw);

  pMode->setCallbacks(new WriteCallbacks());
  pState->setCallbacks(new WriteCallbacks());
  pColor->setCallbacks(new WriteCallbacks());
  pSpeed->setCallbacks(new WriteCallbacks());
  pMic->setCallbacks(new WriteCallbacks());
  pBright->setCallbacks(new WriteCallbacks());
  pPalette->setCallbacks(new WriteCallbacks());

  pNotify = service->createCharacteristic(
      CHAR_NOTIFY_UUID, BLECharacteristic::PROPERTY_READ |
                        BLECharacteristic::PROPERTY_NOTIFY);

  pLive = service->createCharacteristic(
      CHAR_LIVE_UUID, BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY);

  BLECharacteristic* pInfo = service->createCharacteristic(
      CHAR_INFO_UUID, BLECharacteristic::PROPERTY_READ);
  String info = String("CeilingLight ") + FW_VERSION + ";" +
                String(MODE_COUNT) + " modes;" +
                String(NUM_LEDS) + " leds;" +
                String(PALETTE_COUNT) + " palettes";
  pInfo->setValue((uint8_t*)info.c_str(), info.length());

  updateCharValues();

  service->start();

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  adv->setMaxPreferred(0x12);
  adv->start();

  Serial.printf("BLE: advertising as %s\n", BLE_DEVICE_NAME);
}

void bleLoop() {
  if (needAdvertising && millis() - disconnectMs > 600) {
    needAdvertising = false;
    BLEDevice::getAdvertising()->start();
    Serial.println("BLE: advertising again");
  }

  if (!connected) return;

  static uint32_t lastLive = 0;
  if (millis() - lastLive < 100) return;
  lastLive = millis();

  uint8_t live[4] = {
      audioLevel(),
      audioBand(0),
      audioBand(3),
      audioBand(6)
  };
  pLive->setValue(live, sizeof(live));
  pLive->notify();
}
