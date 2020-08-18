#pragma once

#include <Preferences.h>
#include "protocol.h"


// DocGreenDisplay.ino
extern docgreen_status_t scooterStatus;
extern Preferences preferences;
extern bool reenableLightsAfterError;
#define PREFERENCE_MAX_SPEED "max-speed"
#define PREFERENCE_SHOW_INTRO "show-intro"
#define PREFERENCE_REENABLE_LIGHT "reenable-light"
#define PREFERENCE_LOCK_ON_BOOT "lock-on-boot"
#define PREFERENCE_LOCK_PIN "lock-pin"
#define PREFERENCE_AP_ENABLE "ap-enable"
#define PREFERENCE_AP_SSID "ap-ssid"
#define PREFERENCE_AP_PASSWORD "ap-pw"
#define PREFERENCE_STA_ENABLE "sta-enable"
#define PREFERENCE_STA_SSID "sta-ssid"
#define PREFERENCE_STA_PASSWORD "sta-pw"
#define PREFERENCE_UPDATE_URL "update-url"


// oled-ui.hpp
#define BUTTON_UP     0b00000001
#define BUTTON_RIGHT  0b00000010
#define BUTTON_DOWN   0b00000100
#define BUTTON_LEFT   0b00001000
#define BUTTON_CANCEL 0b00010000
#define BUTTON_POWER  0b00100000
extern uint8_t pressedButtons;
uint8_t getAndResetButtons();

extern uint32_t configuredSpeed;

extern String scooterPin;
extern bool isLocked;


// wifi.hpp
extern bool wifiApEnabled;
extern bool wifiStaEnabled;
extern String wifiApSsid;
extern String wifiApPassword;
extern String wifiStaSsid;
extern String wifiStaPassword;


// update.hpp
extern String firmwareUpdateUrl;
extern String firmwareUpdateStatus;
extern void checkForFirmwareUpdates();