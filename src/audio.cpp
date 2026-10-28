#include "audio.h"

#include <math.h>

#include "config.h"
#include "state.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#if MIC_I2S
#include <driver/i2s.h>
#endif

static volatile uint8_t  gLevel = 0;
static volatile uint8_t  gBands[AUDIO_BANDS] = {0};
static volatile uint8_t  gBeatPulse = 0;
static volatile uint32_t gBeatCount = 0;
static volatile float    gRms = 0.0f;

static const float BAND_FREQ[AUDIO_BANDS] = {
    60.0f, 120.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 3000.0f, 3800.0f
};

static float gCoeff[AUDIO_BANDS];

#if MIC_I2S
static bool gI2sReady = false;
#endif

static uint8_t levelFromBands() {
  uint8_t mx = 0;
  for (uint8_t i = 0; i < AUDIO_BANDS; i++) {
    if (gBands[i] > mx) mx = gBands[i];
  }
  return mx;
}

uint8_t audioLevel() {
  if (state.micMode == MIC_OFF) return 0;
  if (state.micMode == MIC_BEAT) return gBeatPulse;
  if (state.micMode == MIC_SPECTRUM) return levelFromBands();
  return gLevel;
}

uint8_t audioBand(uint8_t index) {
  if (index >= AUDIO_BANDS) return 0;
  return gBands[index];
}

uint8_t audioBeatPulse() {
  return gBeatPulse;
}

uint32_t audioBeatCount() {
  return gBeatCount;
}

static float goertzel(const int16_t* samples, uint16_t count, float coeff) {
  float q1 = 0.0f;
  float q2 = 0.0f;

  for (uint16_t i = 0; i < count; i++) {
    float q0 = coeff * q1 - q2 + (float)samples[i];
    q2 = q1;
    q1 = q0;
  }

  float power = q1 * q1 + q2 * q2 - coeff * q1 * q2;
  if (power < 0.0f) power = 0.0f;
  return sqrtf(power) / (float)count;
}

static void processBlock(int16_t* samples, uint16_t count) {
  float sum = 0.0f;
  for (uint16_t i = 0; i < count; i++) {
    sum += (float)samples[i] * (float)samples[i];
  }

  float rms = sqrtf(sum / (float)count);
  gRms = rms;

  static float peakRms = 120.0f;
  if (rms > peakRms) {
    peakRms = peakRms * 0.70f + rms * 0.30f;
  } else {
    peakRms *= 0.997f;
  }
  if (peakRms < 60.0f) peakRms = 60.0f;

  float gate = 0.012f + (float)(255 - state.micSens) * 0.0012f;
  if (gate > 0.55f) gate = 0.55f;

  float norm = rms / peakRms;
  float x = (norm - gate) / (1.0f - gate);
  if (x < 0.0f) x = 0.0f;
  if (x > 1.0f) x = 1.0f;

  uint8_t target = (uint8_t)(powf(x, 0.65f) * 255.0f);

  static float smoothed = 0.0f;
  float attack = (target > smoothed) ? 0.65f : 0.16f;
  smoothed += ((float)target - smoothed) * attack;
  gLevel = (uint8_t)smoothed;

  static float bandPeak[AUDIO_BANDS] = {
      0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f
  };
  static float bandSmooth[AUDIO_BANDS] = {0};

  float normBand[AUDIO_BANDS];
  for (uint8_t b = 0; b < AUDIO_BANDS; b++) {
    float mag = goertzel(samples, count, gCoeff[b]);

    if (mag > bandPeak[b]) {
      bandPeak[b] = bandPeak[b] * 0.70f + mag * 0.30f;
    } else {
      bandPeak[b] *= 0.995f;
    }
    if (bandPeak[b] < 0.01f) bandPeak[b] = 0.01f;

    float bn = mag / bandPeak[b];
    if (bn > 1.0f) bn = 1.0f;
    normBand[b] = bn;

    float bx = (bn - gate * 0.85f) / (1.0f - gate * 0.85f);
    if (bx < 0.0f) bx = 0.0f;
    if (bx > 1.0f) bx = 1.0f;

    uint8_t bt = (uint8_t)(powf(bx, 0.60f) * 255.0f);
    float ba = (bt > bandSmooth[b]) ? 0.70f : 0.22f;
    bandSmooth[b] += ((float)bt - bandSmooth[b]) * ba;
    gBands[b] = (uint8_t)bandSmooth[b];
  }

  // --- детектор бита по низам ---
  float bass = (normBand[0] + normBand[1] + normBand[2]) / 3.0f;
  static float bassAvg = 0.15f;
  bassAvg = bassAvg * 0.92f + bass * 0.08f;

  static uint32_t lastBeatMs = 0;
  uint32_t now = millis();

  if (bass > bassAvg * 1.35f + 0.06f &&
      bass > gate * 1.15f &&
      now - lastBeatMs > 170) {
    lastBeatMs = now;
    gBeatPulse = 255;
    gBeatCount++;
  }

  gBeatPulse = qsub8(gBeatPulse, 34);
}

#if MIC_I2S
static void i2sInit() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate = AUDIO_SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = 256;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = false;
  cfg.fixed_mclk = 0;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = I2S_SCK;
  pins.ws_io_num = I2S_WS;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = I2S_SD;

  if (i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr) == ESP_OK &&
      i2s_set_pin(I2S_NUM_0, &pins) == ESP_OK) {
    i2s_zero_dma_buffer(I2S_NUM_0);
    gI2sReady = true;
    Serial.println("MIC: I2S INMP441 ready");
  } else {
    Serial.println("MIC: I2S init FAILED, fallback to analog");
  }
}
#endif

static void audioTask(void* arg) {
  (void)arg;

  int16_t samples[AUDIO_BLOCK];
  float dc = 2048.0f;

#if MIC_I2S
  static int32_t i2sBuffer[AUDIO_BLOCK];
#endif

  for (;;) {
    bool haveBlock = false;

#if MIC_I2S
    if (gI2sReady) {
      size_t bytesRead = 0;
      esp_err_t err = i2s_read(I2S_NUM_0, i2sBuffer,
                               sizeof(i2sBuffer), &bytesRead, portMAX_DELAY);
      if (err == ESP_OK && bytesRead >= sizeof(int16_t)) {
        uint16_t n = bytesRead / sizeof(int32_t);
        if (n > AUDIO_BLOCK) n = AUDIO_BLOCK;
        for (uint16_t i = 0; i < n; i++) {
          samples[i] = (int16_t)(i2sBuffer[i] >> 16);
        }
        for (uint16_t i = n; i < AUDIO_BLOCK; i++) {
          samples[i] = 0;
        }
        haveBlock = true;
      }
    }
#endif

    if (!haveBlock) {
      dc = dc * 0.9995f + 2048.0f * 0.0005f;
      for (uint16_t i = 0; i < AUDIO_BLOCK; i++) {
        int raw = analogRead(MIC_PIN);
        dc = dc * 0.9995f + (float)raw * 0.0005f;
        int ac = raw - (int)dc;
        if (ac > 2047) ac = 2047;
        if (ac < -2048) ac = -2048;
        samples[i] = (int16_t)ac;
      }
    }

    processBlock(samples, AUDIO_BLOCK);

    // Уступаем управление FreeRTOS, иначе задача заново получит CPU 0
    // и сработает Task Watchdog (IDLE0 не получит время).
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void audioInit() {
  for (uint8_t i = 0; i < AUDIO_BANDS; i++) {
    float w = 2.0f * (float)PI * BAND_FREQ[i] / (float)AUDIO_SAMPLE_RATE;
    gCoeff[i] = 2.0f * cosf(w);
  }

  analogReadResolution(12);
  analogSetPinAttenuation(MIC_PIN, ADC_11db);

#if MIC_I2S
  i2sInit();
#else
  Serial.printf("MIC: analog on GPIO%d\n", MIC_PIN);
#endif
  xTaskCreatePinnedToCore(audioTask, "audio", 8192, nullptr, 1, nullptr, 0);
}
