#pragma once

/*
  Все настройки железа собраны здесь.
  Значения можно переопределить build_flags в platformio.ini.
*/

// ======================= АДРЕСНАЯ ЛЕНТА =======================
#ifndef LED_PIN
#define LED_PIN 5
#endif

#ifndef NUM_LEDS
#define NUM_LEDS 144
#endif

#ifndef LED_TYPE
#define LED_TYPE WS2812B
#endif

#ifndef COLOR_ORDER
#define COLOR_ORDER GRB
#endif

// Ограничение по току. 4000 мА = 4 А (безопасно для БП 5V/4-5A).
// Если у тебя мощный блок питания — поставь 8000.
#ifndef MAX_MILLIAMPS
#define MAX_MILLIAMPS 4000
#endif

// ======================= МИКРОФОН =============================
// 0 = аналоговый (MAX9814 / MAX4466 / KY-038) -> MIC_PIN
// 1 = цифровой I2S (INMP441) -> I2S_SCK / I2S_WS / I2S_SD
#ifndef MIC_I2S
#define MIC_I2S 0
#endif

#ifndef MIC_PIN
#define MIC_PIN 4
#endif

#ifndef I2S_SCK
#define I2S_SCK 15
#endif

#ifndef I2S_WS
#define I2S_WS 16
#endif

#ifndef I2S_SD
#define I2S_SD 17
#endif

#define AUDIO_SAMPLE_RATE 8000
#define AUDIO_BLOCK 256
#define AUDIO_BANDS 8

// ======================= ПРОЧЕЕ ===============================
#ifndef BLE_DEVICE_NAME
#define BLE_DEVICE_NAME "CeilingLight"
#endif

#define FW_VERSION "1.0.0"
#define MODE_COUNT 48
#define PALETTE_COUNT 13
