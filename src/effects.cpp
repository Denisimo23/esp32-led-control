#include "effects.h"

#include <FastLED.h>

#include "audio.h"
#include "state.h"

const char* const MODE_NAMES[MODE_COUNT] = {
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
};

const char* const PALETTE_NAMES[PALETTE_COUNT] = {
    "Радуга",
    "Огонь",
    "Океан",
    "Лес",
    "Лава",
    "Облака",
    "Вечеринка",
    "Радужные полосы",
    "Осень",
    "Новый год",
    "Валентин",
    "Неон",
    "Своя палитра"
};

static_assert(sizeof(MODE_NAMES) / sizeof(MODE_NAMES[0]) == MODE_COUNT,
              "MODE_NAMES size mismatch");
static_assert(sizeof(PALETTE_NAMES) / sizeof(PALETTE_NAMES[0]) == PALETTE_COUNT,
              "PALETTE_NAMES size mismatch");

/* ======================= ПАЛИТРЫ ======================= */

DEFINE_GRADIENT_PALETTE(autumn_gp) {
  0,   110,  20,   0,
  80,  210,  60,   0,
  160, 255, 170,  20,
  255, 140,  30,   0
};

DEFINE_GRADIENT_PALETTE(xmas_gp) {
  0,   200,   0,   0,
  85,   0, 180,  40,
  170, 255, 200,  40,
  255, 200,   0,   0
};

DEFINE_GRADIENT_PALETTE(valentine_gp) {
  0,   120,   0,  20,
  85,  255,  20, 110,
  170, 255, 190, 220,
  255,  255,  20, 110
};

DEFINE_GRADIENT_PALETTE(neon_gp) {
  0,     0, 255, 255,
  85,  255,   0, 255,
  170, 255, 255,   0,
  255,    0, 255, 255
};

static CRGBPalette16 getPalette() {
  switch (state.paletteId) {
    case 0:  return RainbowColors_p;
    case 1:  return HeatColors_p;
    case 2:  return OceanColors_p;
    case 3:  return ForestColors_p;
    case 4:  return LavaColors_p;
    case 5:  return CloudColors_p;
    case 6:  return PartyColors_p;
    case 7:  return RainbowStripeColors_p;
    case 8:  return CRGBPalette16(autumn_gp);
    case 9:  return CRGBPalette16(xmas_gp);
    case 10: return CRGBPalette16(valentine_gp);
    case 11: return CRGBPalette16(neon_gp);
    default: {
      CRGB c1(state.c1r, state.c1g, state.c1b);
      CRGB c2(state.c2r, state.c2g, state.c2b);
      CRGB c3(state.c3r, state.c3g, state.c3b);
      return CRGBPalette16(c1, c2, c3, c1);
    }
  }
}

static CRGB modifyColor(CRGB c, bool shiftHue) {
  if (state.sat == 255 && state.val == 255 && (!shiftHue || state.hueShift == 0)) {
    return c;
  }

  CHSV hsv = rgb2hsv_approximate(c);
  if (shiftHue) hsv.h += state.hueShift;
  hsv.s = scale8(hsv.s, state.sat);
  hsv.v = scale8(hsv.v, state.val);
  return CRGB(hsv);
}

static CRGBPalette16 framePalette;

static CRGB paletteColor(uint8_t index, uint8_t brightness = 255) {
  CRGB c = ColorFromPalette(framePalette, index, brightness, LINEARBLEND);
  return modifyColor(c, true);
}

/* ======================= ХЕЛПЕРЫ ======================= */

static uint16_t periodMs(uint16_t fastMs, uint16_t slowMs) {
  uint16_t p = (uint16_t)map(state.speed, 1, 255, (long)slowMs, (long)fastMs);
  if (p < 8) p = 8;
  return p;
}

static uint8_t bpm() {
  return (uint8_t)map(state.speed, 1, 255, 6, 180);
}

static uint8_t phase8(uint16_t period) {
  if (period < 8) period = 8;
  return (uint8_t)(((millis() % period) * 256UL) / period);
}

static uint8_t rainbowDelta() {
  uint16_t d = (255 + NUM_LEDS - 1) / NUM_LEDS;
  if (d < 1) d = 1;
  if (d > 16) d = 16;
  return (uint8_t)d;
}

static void fillAll(CRGB c) {
  fill_solid(leds, NUM_LEDS, c);
}

static CRGB solidColor(uint8_t mode) {
  CRGB c = CRGB::White;
  switch (mode) {
    case 1: c = CRGB::White; break;
    case 2: c = CRGB(255, 190, 120); break;
    case 3: c = CRGB::Red; break;
    case 4: c = CRGB::Green; break;
    case 5: c = CRGB::Blue; break;
    case 6: c = CRGB::Yellow; break;
    case 7: c = CRGB::Purple; break;
    case 8: c = CRGB::Cyan; break;
    case 9: c = CHSV(state.hue, state.sat, state.val); break;
  }
  if (mode != 9) c.nscale8(state.val);
  return c;
}

static void renderAmbient() {
  uint8_t t = phase8(periodMs(2200, 9000));
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t idx = (uint8_t)(state.hue + (uint16_t)i * 256 / NUM_LEDS + t);
    leds[i] = paletteColor(idx, 150 + (i % 3) * 30);
  }
}

/* ======================= СТАТИКА ======================= */

static void renderSolid(uint8_t mode) {
  fillAll(solidColor(mode));
}

/* ======================= ДИНАМИКА ======================= */

static void renderDynamic(uint8_t mode) {
  switch (mode) {
    case 10: {
      fillAll(paletteColor(state.hue));
      break;
    }

    case 11: {
      for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t idx = (uint8_t)(state.hue + (uint16_t)i * 255 / NUM_LEDS);
        leds[i] = paletteColor(idx);
      }
      break;
    }

    case 12: {
      fill_rainbow(leds, NUM_LEDS, state.hue + state.hueShift, rainbowDelta());
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = modifyColor(leds[i], false);
      }
      break;
    }

    case 13: {
      uint8_t t = phase8(periodMs(400, 4000));
      fill_rainbow(leds, NUM_LEDS, state.hue + state.hueShift + t, rainbowDelta());
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = modifyColor(leds[i], false);
      }
      break;
    }

    case 14: {
      uint16_t period = periodMs(20, 220);
      int segLen = NUM_LEDS / 3;
      if (segLen < 1) segLen = 1;
      int head = (int)((millis() / period) % (uint32_t)(NUM_LEDS + segLen));

      fillAll(CRGB::Black);
      for (int j = 0; j < segLen; j++) {
        int idx = head - j;
        if (idx < 0) idx += NUM_LEDS + segLen;
        if (idx >= 0 && idx < NUM_LEDS) {
          leds[idx] = paletteColor((uint8_t)(state.hue + j * 255 / segLen));
        }
      }
      break;
    }

    case 15: {
      uint8_t b = beatsin8(12 + bpm() / 2, 14, 255);
      CRGB c = paletteColor(state.hue);
      c.nscale8(b);
      fillAll(c);
      break;
    }

    case 16: {
      static uint8_t flick[NUM_LEDS];
      static uint32_t last = 0;
      if (millis() - last > 35) {
        last = millis();
        for (int i = 0; i < NUM_LEDS; i++) {
          flick[i] = random8(145, 255);
        }
      }
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = paletteColor((uint8_t)(state.hue + (i * 7) % 24), flick[i]);
      }
      break;
    }

    case 17: {
      static uint8_t heat[NUM_LEDS];
      uint8_t cooling = (uint8_t)map(state.speed, 1, 255, 85, 12);
      uint8_t sparking = (uint8_t)map(state.speed, 1, 255, 25, 140);

      for (int i = 0; i < NUM_LEDS; i++) {
        heat[i] = qsub8(heat[i], random8(0, cooling / 8 + 2));
      }

      for (int i = NUM_LEDS - 1; i >= 2; i--) {
        heat[i] = (uint8_t)(((uint16_t)heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3);
      }

      for (int i = 0; i < 3; i++) {
        if (random8() < sparking) {
          uint16_t p = random16(NUM_LEDS);
          heat[p] = qadd8(heat[p], random8(170, 255));
        }
      }

      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = paletteColor(heat[i]);
      }
      break;
    }

    case 18: {
      uint8_t t = phase8(periodMs(2500, 12000));
      for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t w1 = sin8((uint8_t)(i * 4 + t));
        uint8_t w2 = sin8((uint8_t)(i * 9 - t * 2));
        uint8_t v = (uint8_t)(((uint16_t)w1 + w2) / 2);
        leds[i] = paletteColor((uint8_t)(state.hue + v / 2), 90 + v / 2);
      }
      break;
    }

    case 19: {
      uint16_t period = periodMs(25, 170);
      int tail = NUM_LEDS / 5;
      if (tail < 3) tail = 3;

      fadeToBlackBy(leds, NUM_LEDS, 36);

      for (int m = 0; m < 2; m++) {
        int head = (int)((millis() / period + m * (NUM_LEDS / 2)) % (uint32_t)(NUM_LEDS + tail));
        for (int j = 0; j < tail; j++) {
          int idx = head - j;
          if (idx >= 0 && idx < NUM_LEDS) {
            uint8_t b = (uint8_t)(255 - j * 255 / tail);
            leds[idx] = paletteColor((uint8_t)(state.hue + m * 80 + j * 4), b);
          }
        }
      }
      break;
    }

    case 20: {
      fadeToBlackBy(leds, NUM_LEDS, 26);
      uint8_t chance = (uint8_t)map(state.speed, 1, 255, 6, 70);
      if (random8() < chance) {
        leds[random16(NUM_LEDS)] = paletteColor(random8(), 255);
      }
      if (random8() < chance / 2) {
        leds[random16(NUM_LEDS)] = CRGB::White;
      }
      break;
    }

    case 21: {
      uint32_t t1 = millis() / 17;
      uint32_t t2 = millis() / 29;
      uint32_t t3 = millis() / 43;
      for (int i = 0; i < NUM_LEDS; i++) {
        uint16_t v = sin8((uint8_t)(i * 8 + t1)) +
                     sin8((uint8_t)(i * 3 - t2)) +
                     sin8((uint8_t)(i * 13 + t3));
        leds[i] = paletteColor((uint8_t)(v / 3));
      }
      break;
    }
  }
}

/* ======================= ЗВУК ======================= */

static void drawMeteor(uint8_t hue, uint16_t head, uint8_t tail, uint8_t bright) {
  for (int j = 0; j < tail; j++) {
    int idx = (int)head - j;
    if (idx < 0) idx += NUM_LEDS;
    if (idx >= NUM_LEDS) idx -= NUM_LEDS;
    uint8_t b = (uint8_t)((uint16_t)bright * (tail - j) / tail);
    leds[idx] = paletteColor((uint8_t)(hue + j * 5), b);
  }
}

static void renderAudio(uint8_t mode) {
  if (state.micMode == MIC_OFF) {
    renderAmbient();
    return;
  }

  uint8_t lvl = audioLevel();
  uint8_t bass = audioBand(0);
  uint8_t mid = audioBand(3);
  uint8_t treb = audioBand(6);
  uint8_t beat = audioBeatPulse();

  switch (mode) {
    case 22: {
      int n = (int)lvl * NUM_LEDS / 255;
      fillAll(CRGB::Black);
      for (int i = 0; i < n; i++) {
        leds[i] = paletteColor((uint8_t)(state.hue + (uint16_t)i * 255 / NUM_LEDS));
      }

      static int peak = 0;
      if (n > peak) peak = n;
      else if (peak > 0) peak -= 1;
      if (peak >= NUM_LEDS) peak = NUM_LEDS - 1;
      if (peak >= 0 && peak < NUM_LEDS) leds[peak] = CRGB::White;
      break;
    }

    case 23: {
      uint8_t b = beat;
      CRGB c = paletteColor((uint8_t)(state.hue + b / 3));
      c.nscale8(b);
      fillAll(c);
      break;
    }

    case 24: {
      fillAll(CRGB::Black);
      int seg = (NUM_LEDS + AUDIO_BANDS - 1) / AUDIO_BANDS;
      if (seg < 1) seg = 1;
      for (int i = 0; i < NUM_LEDS; i++) {
        int band = i / seg;
        if (band >= AUDIO_BANDS) band = AUDIO_BANDS - 1;
        int local = i % seg;
        int lit = (int)audioBand((uint8_t)band) * seg / 255;
        if (local < lit) {
          leds[i] = paletteColor((uint8_t)(state.hue + band * 30));
        }
      }
      break;
    }

    case 25: {
      uint8_t t = phase8(periodMs(200, 1800));
      uint8_t start = (uint8_t)(state.hue + state.hueShift + bass / 2 + t);
      fill_rainbow(leds, NUM_LEDS, start, rainbowDelta());
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = modifyColor(leds[i], false);
        leds[i].nscale8(qadd8(32, lvl));
      }
      break;
    }

    case 26: {
      if (beat > 150) fillAll(CRGB::White);
      else fillAll(CRGB::Black);
      break;
    }

    case 27: {
      static uint32_t lastBeat = 0;
      static uint16_t head = 0;
      uint32_t bc = audioBeatCount();
      if (bc != lastBeat) {
        lastBeat = bc;
        head = (uint16_t)((head + random8(1, 5)) % NUM_LEDS);
      }
      fadeToBlackBy(leds, NUM_LEDS, 70);
      drawMeteor((uint8_t)(state.hue + beat / 2), head, 12, 255);
      break;
    }

    case 28: {
      uint8_t amp = lvl;
      for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t w = sin8((uint8_t)(i * 6 + millis() / 14));
        uint8_t b = qadd8(scale8(w, amp), amp / 3);
        leds[i] = paletteColor((uint8_t)(state.hue + i * 2), b);
      }
      break;
    }

    case 29: {
      static uint8_t offset = 0;
      offset = (uint8_t)(offset + 1 + lvl / 40 + state.speed / 32);
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = paletteColor((uint8_t)(state.hue + offset + i * 3), 60 + lvl / 2);
      }
      break;
    }

    case 30: {
      static uint8_t heat[NUM_LEDS];
      uint8_t cooling = (uint8_t)map(state.speed, 1, 255, 80, 20);
      for (int i = 0; i < NUM_LEDS; i++) {
        heat[i] = qsub8(heat[i], random8(0, cooling / 8 + 2));
      }
      for (int i = NUM_LEDS - 1; i >= 2; i--) {
        heat[i] = (uint8_t)(((uint16_t)heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3);
      }
      if (lvl > 10) {
        for (int i = 0; i < 1 + lvl / 64; i++) {
          if (random8() < lvl) {
            uint16_t p = random16(NUM_LEDS);
            heat[p] = qadd8(heat[p], qadd8(lvl, 80));
          }
        }
      }
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = paletteColor(heat[i]);
      }
      break;
    }

    case 31: {
      uint8_t b = qadd8(lvl, beat / 3);
      uint8_t w = beatsin8(20 + bpm() / 2, 40, 255);
      CRGB c = paletteColor((uint8_t)(state.hue + b / 4));
      c.nscale8(scale8(b, w));
      fillAll(c);
      break;
    }

    case 32: {
      int center = NUM_LEDS / 2;
      uint8_t t = (uint8_t)(millis() / 12);
      for (int i = 0; i < NUM_LEDS; i++) {
        int dist = i > center ? i - center : center - i;
        uint8_t w = sin8((uint8_t)(dist * 8 - t));
        uint8_t b = scale8(w, lvl);
        leds[i] = paletteColor((uint8_t)(state.hue + dist * 3), b);
      }
      break;
    }

    case 33: {
      int half = NUM_LEDS / 2;
      int n = (int)lvl * half / 255;
      fillAll(CRGB::Black);
      for (int i = 0; i < half && i < n; i++) {
        CRGB c = paletteColor((uint8_t)(state.hue + (uint16_t)i * 255 / half));
        leds[i] = c;
        leds[NUM_LEDS - 1 - i] = c;
      }
      if (beat > 180) {
        leds[half] = CRGB::White;
        if (half > 0) leds[half - 1] = CRGB::White;
      }
      break;
    }

    case 34: {
      uint8_t dominant = 0;
      uint8_t maxB = 0;
      for (uint8_t b = 0; b < AUDIO_BANDS; b++) {
        if (audioBand(b) > maxB) {
          maxB = audioBand(b);
          dominant = b;
        }
      }
      uint8_t hue = (uint8_t)(state.hue + dominant * 31);
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = paletteColor((uint8_t)(hue + (uint16_t)i * 128 / NUM_LEDS), qadd8(40, lvl));
      }
      break;
    }

    case 35: {
      uint8_t t = (uint8_t)(phase8(periodMs(300, 2500)) + audioBeatCount() * 11);
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = paletteColor((uint8_t)(state.hue + t + i * 2), qadd8(60, lvl));
      }
      if (beat > 170) {
        for (int i = 0; i < NUM_LEDS; i += 4) leds[i] = CRGB::White;
      }
      break;
    }
  }
}

/* ======================= ОСОБЫЕ ======================= */

static void renderSpecial(uint8_t mode) {
  switch (mode) {
    case 36: { // Новый год
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB(0, 0, 22);
      }
      uint8_t chance = (uint8_t)map(state.speed, 1, 255, 3, 30);
      for (int k = 0; k < 3; k++) {
        if (random8() < chance) {
          leds[random16(NUM_LEDS)] = (random8() < 128) ? CRGB::White : CRGB(255, 200, 40);
        }
      }
      break;
    }

    case 37: { // Хэллоуин
      uint8_t t = phase8(periodMs(700, 4000));
      for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t v = (uint8_t)((uint16_t)i * 255 / NUM_LEDS + t);
        CRGB c = (v < 128) ? CRGB(255, 90, 0) : CRGB(90, 0, 160);
        c.nscale8(random8(140, 255));
        leds[i] = c;
      }
      if (random8() < 3) fillAll(CRGB::Black);
      break;
    }

    case 38: { // Романтика
      uint8_t b = beatsin8(8 + bpm() / 4, 50, 255);
      for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t idx = (uint8_t)(state.hue + 60 + (uint16_t)i * 128 / NUM_LEDS);
        leds[i] = paletteColor(idx, b);
      }
      break;
    }

    case 39: { // Дискотека
      static uint32_t last = 0;
      static uint8_t seed = 0;
      uint16_t p = periodMs(70, 500);
      if (millis() - last > p) {
        last = millis();
        seed = random8();
      }
      uint8_t beatBoost = 30;
      if (state.micMode != MIC_OFF) beatBoost = audioBeatPulse() / 4;
      for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t block = (uint8_t)((i * 4 / NUM_LEDS + seed) & 3);
        leds[i] = paletteColor((uint8_t)(seed * 13 + block * 64), 180 + beatBoost);
      }
      break;
    }

    case 40: { // Океан
      uint8_t t = phase8(periodMs(2500, 12000));
      for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t w = sin8((uint8_t)(i * 5 + t));
        uint8_t b = 80 + w / 2;
        leds[i] = paletteColor((uint8_t)(state.hue + 80 + w / 2), b);
      }
      if (random8() < 3) leds[random16(NUM_LEDS)] = CRGB::White;
      break;
    }

    case 41: { // Лес
      uint8_t b = beatsin8(6 + bpm() / 3, 40, 220);
      for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t idx = (uint8_t)(state.hue + 40 + (uint16_t)i * 100 / NUM_LEDS);
        leds[i] = paletteColor(idx, b);
      }
      if (random8() < 6) leds[random16(NUM_LEDS)] = CRGB(180, 255, 120);
      break;
    }

    case 42: { // Космос
      fadeToBlackBy(leds, NUM_LEDS, 30);
      uint8_t t = phase8(periodMs(3000, 15000));
      for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t idx = (uint8_t)(state.hue + 160 + (uint16_t)i * 60 / NUM_LEDS + t);
        CRGB c = paletteColor(idx, 50);
        leds[i] = blend(leds[i], c, 90);
      }
      if (random8() < 5) leds[random16(NUM_LEDS)] = CRGB::White;
      break;
    }

    case 43: { // Полиция
      uint16_t p = periodMs(120, 900);
      bool flip = (millis() / p) & 1;
      int half = NUM_LEDS / 2;
      CRGB a = flip ? CRGB::Red : CRGB::Blue;
      CRGB b = flip ? CRGB::Blue : CRGB::Red;
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = (i < half) ? a : b;
      }
      if ((millis() % (p * 2)) < 40) fillAll(CRGB::White);
      break;
    }

    case 44: { // Молния
      static uint32_t nextFlash = 0;
      static uint32_t flashEnd = 0;
      if (millis() > nextFlash) {
        flashEnd = millis() + random8(25, 110);
        nextFlash = millis() + random16(1200, 6000);
      }
      if (millis() < flashEnd) {
        fillAll(CRGB::White);
      } else {
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = CRGB(0, 0, 20 + random8(0, 18));
        }
      }
      break;
    }

    case 45: { // Рассвет
      uint8_t t = phase8(periodMs(6000, 24000));
      for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t pos = (uint8_t)((uint16_t)i * 255 / NUM_LEDS + t);
        CRGB low(110, 0, 30);
        CRGB high(255, 170, 30);
        CRGB c = blend(low, high, pos);
        uint8_t b = 80 + pos / 2;
        c.nscale8(b);
        leds[i] = c;
      }
      break;
    }

    case 46: { // Северное сияние
      uint8_t t = phase8(periodMs(3500, 14000));
      for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t w1 = sin8((uint8_t)(i * 4 + t));
        uint8_t w2 = sin8((uint8_t)(i * 7 - t * 2));
        uint8_t v = (uint8_t)(((uint16_t)w1 + w2) / 2);
        uint8_t hue = (uint8_t)(100 + v / 3);
        leds[i] = paletteColor((uint8_t)(state.hue + hue), 60 + v / 2);
      }
      break;
    }

    case 47: { // Матрица
      fadeToBlackBy(leds, NUM_LEDS, 42);
      uint16_t p = periodMs(30, 200);
      for (int k = 0; k < 4; k++) {
        int head = (int)((millis() / p + k * (NUM_LEDS / 4)) % NUM_LEDS);
        leds[head] = CRGB(80, 255, 80);
        int prev = head - 1;
        if (prev < 0) prev = NUM_LEDS - 1;
        leds[prev] = CRGB(0, 160, 20);
        if (random8() < 10) leds[random16(NUM_LEDS)] = CRGB::White;
      }
      break;
    }
  }
}

/* ======================= ГЛАВНЫЙ РЕНДЕР ======================= */

void renderFrame() {
  FastLED.setBrightness(stateIsOn() ? state.brightness : 0);

  if (!stateIsOn() || state.mode == 0) {
    fillAll(CRGB::Black);
    FastLED.show();
    return;
  }

  uint8_t mode = state.mode;
  framePalette = getPalette();

  if (mode <= 9) {
    renderSolid(mode);
  } else if (mode <= 21) {
    renderDynamic(mode);
  } else if (mode <= 35) {
    renderAudio(mode);
  } else {
    renderSpecial(mode);
  }

  FastLED.show();
}
