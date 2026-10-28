#include <Arduino.h>
#include <FastLED.h>

#include "audio.h"
#include "ble.h"
#include "config.h"
#include "effects.h"
#include "state.h"

static void bootAnimation() {
  FastLED.setBrightness(50);
  for (int pass = 0; pass < 2; pass++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV((uint8_t)(i * 256 / NUM_LEDS + pass * 128), 255, 255);
      FastLED.show();
      delay(3);
    }
  }
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void setup() {
  Serial.begin(115200);
  uint32_t serialStart = millis();
  while (!Serial && millis() - serialStart < 1200) {
    delay(10);
  }

  Serial.println();
  Serial.println("=== Ceiling Light ESP32-S3 ===");
  Serial.printf("Firmware: %s | modes: %d | LEDs: %d\n", FW_VERSION, MODE_COUNT, NUM_LEDS);

  stateInit();

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
      .setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MILLIAMPS);
  FastLED.setBrightness(stateIsOn() ? state.brightness : 0);
  FastLED.clear(true);

  bootAnimation();

  audioInit();
  bleInit();

  Serial.println("Ready. Open the web app and connect to BLE.");
}

void loop() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();

  if (now - lastFrame >= 16) {
    lastFrame = now;
    renderFrame();
  }

  bleLoop();
  stateSaveIfDirty();
  delay(1);
}
