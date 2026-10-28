#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include "config.h"

enum MicMode : uint8_t {
  MIC_OFF = 0,
  MIC_ENVELOPE = 1,
  MIC_BEAT = 2,
  MIC_SPECTRUM = 3
};

struct AppState {
  uint8_t mode = 12;
  uint8_t on = 1;
  uint8_t brightness = 120;
  uint8_t speed = 80;
  uint8_t hue = 0;
  uint8_t sat = 220;
  uint8_t val = 255;
  uint8_t micMode = MIC_ENVELOPE;
  uint8_t micSens = 150;
  uint8_t paletteId = 0;
  uint8_t hueShift = 0;

  uint8_t c1r = 255, c1g = 40, c1b = 40;
  uint8_t c2r = 40, c2g = 255, c2b = 80;
  uint8_t c3r = 40, c3g = 120, c3b = 255;
};

extern AppState state;
extern CRGB leds[NUM_LEDS];

void stateInit();
void stateMarkDirty();
void stateSaveIfDirty();
void stateResetDefaults();

inline bool stateIsOn() { return state.on != 0; }
