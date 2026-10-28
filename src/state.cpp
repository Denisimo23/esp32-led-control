#include "state.h"

#include <Preferences.h>

AppState state;
CRGB leds[NUM_LEDS];

static Preferences prefs;
static volatile bool dirty = false;
static volatile uint32_t lastChangeMs = 0;

static void loadState() {
  prefs.begin("clight", true);

  state.mode       = prefs.getUChar("mode", state.mode);
  state.on         = prefs.getUChar("on", state.on);
  state.brightness = prefs.getUChar("bright", state.brightness);
  state.speed      = prefs.getUChar("speed", state.speed);
  state.hue        = prefs.getUChar("hue", state.hue);
  state.sat        = prefs.getUChar("sat", state.sat);
  state.val        = prefs.getUChar("val", state.val);
  state.micMode    = prefs.getUChar("micMode", state.micMode);
  state.micSens    = prefs.getUChar("micSens", state.micSens);
  state.paletteId  = prefs.getUChar("pal", state.paletteId);
  state.hueShift   = prefs.getUChar("hueShift", state.hueShift);

  state.c1r = prefs.getUChar("c1r", state.c1r);
  state.c1g = prefs.getUChar("c1g", state.c1g);
  state.c1b = prefs.getUChar("c1b", state.c1b);
  state.c2r = prefs.getUChar("c2r", state.c2r);
  state.c2g = prefs.getUChar("c2g", state.c2g);
  state.c2b = prefs.getUChar("c2b", state.c2b);
  state.c3r = prefs.getUChar("c3r", state.c3r);
  state.c3g = prefs.getUChar("c3g", state.c3g);
  state.c3b = prefs.getUChar("c3b", state.c3b);

  prefs.end();

  if (state.mode >= MODE_COUNT) state.mode = 12;
  if (state.speed == 0) state.speed = 1;
  if (state.micMode > MIC_SPECTRUM) state.micMode = MIC_ENVELOPE;
  if (state.paletteId >= PALETTE_COUNT) state.paletteId = 0;
  state.on = state.on ? 1 : 0;
}

static void saveState() {
  prefs.begin("clight", false);

  prefs.putUChar("mode", state.mode);
  prefs.putUChar("on", state.on);
  prefs.putUChar("bright", state.brightness);
  prefs.putUChar("speed", state.speed);
  prefs.putUChar("hue", state.hue);
  prefs.putUChar("sat", state.sat);
  prefs.putUChar("val", state.val);
  prefs.putUChar("micMode", state.micMode);
  prefs.putUChar("micSens", state.micSens);
  prefs.putUChar("pal", state.paletteId);
  prefs.putUChar("hueShift", state.hueShift);

  prefs.putUChar("c1r", state.c1r);
  prefs.putUChar("c1g", state.c1g);
  prefs.putUChar("c1b", state.c1b);
  prefs.putUChar("c2r", state.c2r);
  prefs.putUChar("c2g", state.c2g);
  prefs.putUChar("c2b", state.c2b);
  prefs.putUChar("c3r", state.c3r);
  prefs.putUChar("c3g", state.c3g);
  prefs.putUChar("c3b", state.c3b);

  prefs.end();
}

void stateInit() {
  loadState();
  dirty = false;
  lastChangeMs = millis();
}

void stateMarkDirty() {
  lastChangeMs = millis();
  dirty = true;
}

void stateSaveIfDirty() {
  if (!dirty) return;
  if (millis() - lastChangeMs < 1500) return;
  dirty = false;
  saveState();
}

void stateResetDefaults() {
  state = AppState();
  stateMarkDirty();
}
