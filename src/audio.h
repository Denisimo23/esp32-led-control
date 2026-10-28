#pragma once

#include <Arduino.h>

void audioInit();

uint8_t audioLevel();
uint8_t audioBand(uint8_t index);
uint8_t audioBeatPulse();
uint32_t audioBeatCount();
