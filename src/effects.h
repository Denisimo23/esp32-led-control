#pragma once

#include <Arduino.h>

#include "config.h"

extern const char* const MODE_NAMES[MODE_COUNT];
extern const char* const PALETTE_NAMES[PALETTE_COUNT];

void renderFrame();
