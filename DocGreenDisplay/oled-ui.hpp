#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdint.h>

#include "state.hpp"
#include "icons.h"

uint8_t pressedButtons = 0;

uint32_t configuredSpeed = 20;

bool isLocked = false;
String scooterPin;

#ifdef ARDUINO_ARCH_ESP32
TwoWire I2CInstance = TwoWire(0);
Adafruit_SSD1306 display(128, 32, &I2CInstance, -1);
#else
Adafruit_SSD1306 display(128, 32, &Wire, -1);
#endif

//
// utility functions
//

void printCommaValue(int32_t val, int32_t unit)
{
	// when unit = 100 print 314cm as 3.1m or 3736cV as 37.3V etc.
	// when unit = 1000 print 7813m as 7.8km etc.
	int32_t afterComma = (val / (unit / 10)) % 10;
	if(afterComma < 0)
		afterComma = -afterComma;

	display.print(val / unit);
	display.print(".");
	display.println(afterComma);
}

void detectButtonPress(uint16_t throttle, uint16_t brake, bool brakeLever)
{
	static uint8_t heldThrottleButtons = 0;
	static uint8_t heldBrakeButtons = 0;
	static bool hadBrakeLever = false;

	if(throttle > THROTTLE_READ_MAX - 100)
		heldThrottleButtons |= BUTTON_RIGHT;
	else if(throttle > THROTTLE_READ_MIN + 100)
		heldThrottleButtons |= BUTTON_DOWN;

	if(heldThrottleButtons != 0 && throttle <= THROTTLE_READ_MIN + 100)
	{
		if(heldThrottleButtons & BUTTON_RIGHT)
			heldThrottleButtons = heldThrottleButtons & ~BUTTON_DOWN;

		pressedButtons |= heldThrottleButtons;
		heldThrottleButtons = 0;
	}


	if(brake > THROTTLE_READ_MAX - 100)
		heldBrakeButtons |= BUTTON_LEFT;
	else if(brake > BRAKE_READ_MIN + 100)
		heldBrakeButtons |= BUTTON_UP;

	if(heldBrakeButtons != 0 && brake <= THROTTLE_READ_MIN + 100)
	{
		if(heldBrakeButtons & BUTTON_LEFT)
			heldBrakeButtons = heldBrakeButtons & ~BUTTON_UP;

		pressedButtons |= heldBrakeButtons;
		heldBrakeButtons = 0;
	}


	if(brakeLever)
	{
		hadBrakeLever = true;
	}
	else if(hadBrakeLever)
	{
		hadBrakeLever = false;
		pressedButtons |= BUTTON_CANCEL;
	}
}

uint8_t getAndResetButtons()
{
	uint8_t val = pressedButtons;
	pressedButtons = 0;
	return val;
}

void genericSelectionMenu(const char *title, const char **options, int count,
	uint8_t &button, int &selection, bool &inMenu)
{
	if(!inMenu && button & BUTTON_RIGHT)
	{
		inMenu = true;
		button &= ~BUTTON_RIGHT;
	}

	if(inMenu)
		return;

	if(button & BUTTON_DOWN)
	{
		selection++;
		if(selection >= count)
			selection = 0;
	}
	if(button & BUTTON_UP)
	{
		selection--;
		if(selection < 0)
			selection = count - 1;
	}

	display.setTextSize(1);
	display.println(title);
	display.setCursor(0, display.getCursorY() + 5);

	for(int i = 0; i < count; i++)
	{
		if(i == selection)
		{
			display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
			display.println(options[i]);
			display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
		}
		else
		{
			display.println(options[i]);
		}
	}
}

//
// menu functions
//

void showDriveScreen(docgreen_status_t& status)
{
	display.setTextSize(1);
	display.println("SPEED");
	uint8_t speed = status.speed / 1000;
	if(status.speed % 1000 >= 500)
		speed++;
	display.setTextSize(4);
	if(speed < 10)
		display.println("0");
	display.println(speed);

	if(status.ecoMode)
	{
		display.setCursor(0, display.getCursorY() + 10);
		display.setTextSize(1);
		display.println("ECO");
	}

	if(status.lights)
	{
		display.drawBitmap(
			0, display.height() - BEAM_HEIGHT,
			beam_bitmap, BEAM_WIDTH, BEAM_HEIGHT, 1
		);
	}
}

void showErrorScreen(docgreen_status_t& status)
{
	display.setTextSize(1);
	display.println("ERROR");
	display.setTextSize(4);
	display.println(status.errorCode);
}

void showByeScreen(docgreen_status_t& status)
{
	display.setTextSize(4);
	display.print("BYE");
}

void showLockMenu(docgreen_status_t& status)
{
	static int index = 0;
	static bool failed = false;

	static unsigned lastToggle = 0;
	if(lastToggle == 0)
	{
		setLock(true);
		delay(50);
		setLock(true);
		delay(50);
		setLock(true);

		lastToggle = 1;
	}

	if(status.speed > 100 && lastToggle + 400 < millis())
	{
		setLight(!status.lights);
		lastToggle = millis();
	}

	display.setTextSize(1);
	display.println("LOCK");

	display.setCursor(0, display.getCursorY() + 10);
	display.println("ENTER");
	display.println("PIN");

	display.setCursor(0, display.getCursorY() + 10);
	for(int i = 0; i < index; i++)
		display.print("*");



	uint8_t buttons = getAndResetButtons();
	if(buttons == 0)
		return;

	uint8_t wantedButton = 1 << (scooterPin[index] - '0');
	if((buttons & ~wantedButton) != 0)
		failed = true;

	index++;
	if(index >= scooterPin.length())
	{
		if(!failed)
			isLocked = false;

		index = 0;
		failed = false;
	}

	if(!isLocked)
	{
		display.setCursor(0, display.getCursorY() + 10);
		display.println("HELLO");

		lastToggle = 0;
		setLock(false);
		delay(50);
		setLock(false);
		delay(50);
		setLock(false);
	}
	else
	{
		display.print("*");
	}
}

void showIntro()
{
	int width = display.width();
	for(int i = width; i >= -width; i--)
	{
		display.clearDisplay();
		display.drawBitmap(
			i, (display.height() - SCOOTER_HEIGHT) / 2,
			scooter_bitmap, SCOOTER_WIDTH, SCOOTER_HEIGHT, 1
		);
		display.display();

		delay(1);
	}
}

void showInfoScreen(docgreen_status_t& status)
{
	display.setTextSize(1);

	display.println("SOC");
	display.setTextSize(2);
	if(status.soc == 100)
	{
		display.println("FU");
	}
	else if(status.soc < 10)
	{
		display.print("0");
		display.println(status.soc);
	}
	else
	{
		display.println(status.soc);
	}
	display.setTextSize(1);
	display.setCursor(0, display.getCursorY() + 5);

	display.println("MODE");
	display.setTextSize(2);
	display.print(status.ecoMode ? "E" : "S");
	display.println(status.lights ? " N" : " D");
	display.setTextSize(1);
	display.setCursor(0, display.getCursorY() + 5);

	display.println("TIME");
	uint32_t now = millis();
	display.print(now / 60000);
	display.println("m");
	display.print((now / 1000) % 60);
	display.println("s");
	display.setCursor(0, display.getCursorY() + 5);

	display.println("ODO");
	printCommaValue(status.odometer, 1000);
}

void showStatsScreen(docgreen_status_t& status)
{
	display.setTextSize(1);

	display.println("VOLT");
	printCommaValue(status.voltage, 100);
	display.setCursor(0, display.getCursorY() + 5);

	display.println("AMPS");
	printCommaValue(status.current, 100);
	display.setCursor(0, display.getCursorY() + 5);

	display.println("ODO");
	printCommaValue(status.odometer, 1000);
	display.setCursor(0, display.getCursorY() + 5);

	display.println("TTIME");
	display.print(status.totalOperationTime / (60 * 60)); // hours
	display.println("h");
	display.print((status.totalOperationTime / 60) % 60); // minutes
	display.println("m");
	display.setCursor(0, display.getCursorY() + 5);

	display.println("VERS");
	display.println(status.mainboardVersion, 16);
}

void showDebugScreen(docgreen_status_t& status)
{
	display.setTextSize(1);

	display.println("thro");
	display.println(analogRead(THROTTLE_PIN));
	display.println(status.throttle, 16);

	display.println("brak");
	display.println(analogRead(BRAKE_PIN));
	display.println(status.brake, 16);

	display.println("hand");
	display.println(digitalRead(MECHANICAL_BRAKE_PIN) == HIGH ? "HIGH" : "LOW");

	display.println("time");
	display.println(millis() / 1000);
}

void showTuningMenu(uint8_t button)
{
	static uint32_t speed = 0;
	if(speed == 0)
		speed = configuredSpeed;

	display.setTextSize(1);
	display.println("max");
	display.println("speed");
	display.setTextSize(2);
	display.println(speed);
	display.setTextSize(1);
	display.println("rpm");
	display.println((speed * 2518) / 100);

	display.println();
	if(configuredSpeed == speed)
		display.println("set");

	if(button & BUTTON_DOWN)
		speed++;
	if(button & BUTTON_UP)
		speed--;

	if(speed > 35)
		speed = 35;

	if(button & BUTTON_RIGHT)
	{
		setMaxSpeed(speed);
		configuredSpeed = speed;
	}
}

void showWifiMenu(uint8_t button)
{
	display.setTextSize(1);
	display.println("WiFi");

	display.setCursor(0, display.getCursorY() + 5);

	display.println("AP:");
	display.println(wifiApEnabled ? "ON" : " OFF");

	display.setCursor(0, display.getCursorY() + 5);

	static bool onAfterReboot = false;
	if(wifiApEnabled)
	{
		display.println("pass:");
		display.println(wifiApPassword);
	}
	else if(onAfterReboot)
	{
		display.println("on");
		display.println("after");
		display.println("boot");
	}
	else
	{

		display.println("turn");
		display.println("on?");
		if(!onAfterReboot && button & BUTTON_RIGHT)
			preferences.putUChar(PREFERENCE_AP_ENABLE, 1);
	}
}

bool showConfigMenu(docgreen_status_t& status, uint8_t button)
{
	static int menu = 0;
	static bool inMenu = false;
	static const char *menus[] = {
		"tune",
		"wifi",
		"light",
		"eco",
		"debug",
	};

	bool exitConfigMenu = !inMenu && button & BUTTON_LEFT;

	genericSelectionMenu("CONF", menus, sizeof(menus) / sizeof(const char *),
		button, menu, inMenu);

	if(inMenu)
	{
		switch(menu)
		{
			case 0:
				showTuningMenu(button);
				inMenu = (button & BUTTON_LEFT) == 0;
				break;
			case 1:
				showWifiMenu(button);
				inMenu = (button & BUTTON_LEFT) == 0;
				break;
			case 2:
				delay(50);
				setLight(!status.lights);
				inMenu = false;
				break;
			case 3:
				delay(50);
				setEcoMode(!status.ecoMode);
				inMenu = false;
				break;
			case 4:
				showDebugScreen(status);
				inMenu = (button & BUTTON_LEFT) == 0;
				break;
		}
	}

	return !exitConfigMenu;
}

void showMainMenu(docgreen_status_t& status)
{
	static int menu = 0;
	static bool inMenu = true;
	uint8_t button = getAndResetButtons();

	static const char *menus[] = {
		"info",
		"stats",
		"conf",
		"lock",
		"intro",
	};
	genericSelectionMenu("MENU", menus, sizeof(menus) / sizeof(const char *),
		button, menu, inMenu);

	if(inMenu)
	{
		switch(menu)
		{
			case 0:
				showInfoScreen(status);
				inMenu = (button & BUTTON_LEFT) == 0;
				break;
			case 1:
				showStatsScreen(status);
				inMenu = (button & BUTTON_LEFT) == 0;
				break;
			case 2:
				inMenu = showConfigMenu(status, button);
				break;
			case 3:
				isLocked = true;
				inMenu = false;
				break;
			case 4:
				showIntro();
				inMenu = false;
				break;
		}
	}
}

//
// public functions
//

void initializeOledUi()
{
#ifdef ARDUINO_ARCH_ESP32
	I2CInstance.begin(17, 16, 400000);
#endif

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

	display.setRotation(1);
	display.setTextSize(1);
	display.setTextColor(SSD1306_WHITE);

	if(preferences.getUChar(PREFERENCE_SHOW_INTRO, 1))
		showIntro();
	if(preferences.getUChar(PREFERENCE_LOCK_ON_BOOT, 0))
		isLocked = true;

	scooterPin = preferences.getString(PREFERENCE_LOCK_PIN, "012345");

	display.clearDisplay();
	display.setCursor(0, 0);
	display.setTextSize(1);
	display.println("wait\nfor\ndata\n...");
	display.display();
}

void updateOledUi(docgreen_status_t &status)
{
    static bool inDrive = false;
    if(!inDrive && status.speed > 5000)
    {
        inDrive = true;
        status.enableStatsRequests = false;
    }
    else if(inDrive && status.speed < 1000)
    {
        inDrive = false;
        status.enableStatsRequests = true;

        //reset buttons as they might have been pressed during drive
        getAndResetButtons();
    }

    display.clearDisplay();
	display.setCursor(0, 0);

    //if(true) showDebugScreen(status); else
    if(status.errorCode != 0)
        showErrorScreen(status);
    else if(status.shuttingDown)
        showByeScreen(status);
    else if(isLocked)
        showLockMenu(status);
    else if(inDrive)
        showDriveScreen(status);
    else
        showMainMenu(status);

    display.display();
}