#pragma once

#include "protocol.h"

// DocGreenDisplay.ino
extern bool isLocked;
extern docgreen_status_t scooterStatus;

// oled-ui.hpp
#define BUTTON_UP     0b00000001
#define BUTTON_RIGHT  0b00000010
#define BUTTON_DOWN   0b00000100
#define BUTTON_LEFT   0b00001000
#define BUTTON_CANCEL 0b00010000
#define BUTTON_POWER  0b00100000
extern uint8_t pressedButtons = 0;
extern uint8_t password[];
uint8_t getAndResetButtons();
