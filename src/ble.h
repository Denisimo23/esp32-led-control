#pragma once

#include <Arduino.h>

void bleInit();
void bleLoop();
void bleNotifyState();
bool bleIsConnected();
