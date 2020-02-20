#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdint.h>

#include "config.h"
#include "icons.h"
#include "protocol.h"

#define THROTTLE_MIN 0x2C
#define THROTTLE_MAX 0xC5
#define BRAKE_MIN 0x2C
#define BRAKE_MAX 0xB5

Adafruit_SSD1306 display(128, 32, &Wire, -1);
uint16_t lastThrottle = 0;
uint16_t lastBrake = 0;

#define BUTTON_UP     0b00000001
#define BUTTON_RIGHT  0b00000010
#define BUTTON_DOWN   0b00000100
#define BUTTON_LEFT   0b00001000
#define BUTTON_CANCEL 0b00010000
#define BUTTON_POWER  0b00100000
uint8_t pressedButtons = 0;

bool isLocked = true;
uint8_t password[] = {LOCK_PASSWORD};

void detectButtonPress(bool brakeLever)
{
	static uint8_t heldThrottleButtons = 0;
	static uint8_t heldBrakeButtons = 0;
	static bool hadBrakeLever = false;

	if(lastThrottle > THROTTLE_READ_MAX - 100)
		heldThrottleButtons |= BUTTON_RIGHT;
	else if(lastThrottle > THROTTLE_READ_MIN + 100)
		heldThrottleButtons |= BUTTON_DOWN;

	if(heldThrottleButtons != 0 && lastThrottle <= THROTTLE_READ_MIN + 100)
	{
		if(heldThrottleButtons & BUTTON_RIGHT)
			heldThrottleButtons = heldThrottleButtons & ~BUTTON_DOWN;

		pressedButtons |= heldThrottleButtons;
		heldThrottleButtons = 0;
	}


	if(lastBrake > THROTTLE_READ_MAX - 100)
		heldBrakeButtons |= BUTTON_LEFT;
	else if(lastBrake > BRAKE_READ_MIN + 100)
		heldBrakeButtons |= BUTTON_UP;

	if(heldBrakeButtons != 0 && lastBrake <= THROTTLE_READ_MIN + 100)
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
	if(status.speed > 500 && lastToggle + 500 < millis())
	{
		setLight(!status.lights);
		lastToggle = millis();
	}
	else if(status.lights && lastToggle != 0)
	{
		setLight(false);
		lastToggle = 0;
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

	uint8_t wantedButton = password[index];
	if((buttons & ~wantedButton) != 0)
		failed = true;

	index++;
	if(index >= sizeof(password) / sizeof(uint8_t))
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

void routinelyRequestDetailedInfo()
{
	static unsigned lastRequest = 0;
	if(lastRequest + 200 > millis())
	{
		requestDetailedInfo1();
	}
	else if(lastRequest + 400 > millis())
	{
		requestDetailedInfo2();
		lastRequest = millis();
	}
}

void printCommaValue(uint32_t val, uint32_t unit)
{
	// when unit = 100 print 314cm as 3.1m or 3736cV as 37.3V etc.
	// when unit = 1000 print 7813m as 7.8km etc.
	display.print(val / unit);
	display.print(".");
	display.println((val / (unit / 10)) % 10);
}

void showInfoScreen(docgreen_status_t& status)
{
	routinelyRequestDetailedInfo();

	display.println("SOC");
	display.println(status.soc);
	display.setCursor(0, display.getCursorY() + 5);

	display.println("MODE");
	display.print(status.ecoMode ? "E" : "S");
	display.println(status.lights ? " N" : " D");
	display.setCursor(0, display.getCursorY() + 5);

	display.println("TIME");
	display.println(status.timeSinceBoot);
	display.setCursor(0, display.getCursorY() + 5);

	display.println("ODO");
	printCommaValue(status.odometer, 1000);
}

void showStatsScreen(docgreen_status_t& status)
{
	routinelyRequestDetailedInfo();

	display.println("VOLT");
	printCommaValue(status.voltage, 100);
	display.setCursor(0, display.getCursorY() + 2);

	display.println("AMPS");
	printCommaValue(status.current, 100);
	display.setCursor(0, display.getCursorY() + 2);

	display.println("ODO");
	printCommaValue(status.odometer, 1000);
	display.setCursor(0, display.getCursorY() + 2);

	display.println("TTIME");
	display.print(status.totalOperationTime / (60 * 60)); // hours
	display.print(":");
	display.print((status.totalOperationTime / 60) % 60); // minutes
	display.print(":")
	display.println(status.totalOperationTime % 60); // seconds
	display.setCursor(0, display.getCursorY() + 2);

	display.println("VERS");
	display.println(status.mainboardVersion, 16);
}

void showTuningMenu(uint8_t button)
{
	static uint32_t configuredSpeed = 20;
	static uint32_t speed = 20;

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

	if(speed > 30)
		speed = 30;

	if(button & BUTTON_RIGHT)
	{
		setMaxSpeed(speed);
		configuredSpeed = speed;
	}
}

void genericSelectionMenu(const char *title, uint8_t button,
	const char *options, int count, int *selection, bool *inMenu)
{
	if(!*inMenu && button & BUTTON_RIGHT)
		*inMenu = true;
	if(*inMenu && button & BUTTON_LEFT)
		*inMenu = false;

	if(*inMenu)
		return;

	int curr = *selection;
	if(button & BUTTON_DOWN)
	{
		curr++;
		if(curr >= count)
			curr = 0;
	}
	if(button & BUTTON_UP)
	{
		curr--;
		if(curr < 0)
			curr = count - 1;
	}
	*selection = curr;

	display.setTextSize(1);
	display.println(title);
	display.setCursor(0, display.getCursorY() + 5);

	for(int i = 0; i < count; i++)
	{
		if(i == curr)
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

void showConfigMenu(docgreen_status_t& status, uint8_t button)
{
	static int menu = 0;
	static bool inMenu = true;

	static const char *menus[] = {
		"tune",
		"light",
		"eco",
		"lock",
	};
	genericSelectionMenu("CONF", button, menus,
		sizeof(menus) / sizeof(const char *), &menu, &inMenu);

	if(inMenu)
	{
		switch(menu)
		{
			case 0:
				showTuningMenu();
				break;
			case 1:
				setLight(!status.lights);
				inMenu = false;
				break;
			case 2:
				setEcoMode(!status.ecoMode);
				inMenu = false;
				break;
			case 3:
				;
				// TODO should we use this lock functionality
				//  for our lock instead of breaking?
				// what does it even do?
				static bool isEcuLocked = false;
				isEcuLocked = !isEcuLocked;
				setLock(isEcuLocked);
				inMenu = false;
				break;
		}
	}
}

void showMainMenu(docgreen_status_t& status)
{
	static int menu = 0;
	static bool inMenu = true;
	uint8_t button = getAndResetButtons();

	static const char *menus[] = {
		"info",
		"stats",
		"config",
		"lock",
		"intro",
	};
	genericSelectionMenu("MENU", button, menus,
		sizeof(menus) / sizeof(const char *), &menu, &inMenu);

	if(inMenu)
	{
		uint8_t buton = getAndResetButtons();
		if(button & BUTTON_LEFT)
			inMenu = false;

		switch(menu)
		{
			case 0:
				showInfoScreen(status);
				break;
			case 1:
				showStatsScreen(status);
				break;
			case 2:
				showConfigMenu(status, button);
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

void setup()
{
	ScooterSerial.begin(115200);

	pinMode(MECHANICAL_BRAKE_PIN, INPUT);

	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

	display.setRotation(1);
	display.setTextSize(1);
	display.setTextColor(SSD1306_WHITE);

	showIntro();

#if DEFAULT_MAX_SPEED != 20
	// send default max speed 3 times, to make sure the packet isn't lost
	setMaxSpeed(DEFAULT_MAX_SPEED);
	delay(50);
	setMaxSpeed(DEFAULT_MAX_SPEED);
	delay(50);
	setMaxSpeed(DEFAULT_MAX_SPEED);
#endif
}

void loop()
{
	static uint32_t lastTransmit = 0;
	uint32_t now = millis();
	if(now - lastTransmit > TRANSMIT_INTERVAL)
	{
		uint16_t throttle = analogRead(THROTTLE_PIN);
		uint16_t brake = analogRead(BRAKE_PIN);
		bool brakeLever = digitalRead(MECHANICAL_BRAKE_PIN) == HIGH;

		if(throttle < THROTTLE_READ_MIN)
			throttle = THROTTLE_READ_MIN;
		if(throttle > THROTTLE_READ_MAX)
			throttle = THROTTLE_READ_MAX;
		if(brake < BRAKE_READ_MIN)
			brake = BRAKE_READ_MIN;
		if(brake > BRAKE_READ_MAX)
			brake = BRAKE_READ_MAX;

		lastThrottle = throttle;
		lastBrake = brake;
		detectButtonPress(brakeLever);

		if(brakeLever)
		{
			throttle = THROTTLE_READ_MIN;

			// XXX: in the orginal configuration pulling the mechanical brake
			// lever makes the scooter brake with the maximum power on the
			// electrical brake, however that feels very harsh and dangerous
			brake = map(40, 0, 100, THROTTLE_READ_MIN, THROTTLE_READ_MAX);
		}
		if(isLocked)
		{
			throttle = THROTTLE_READ_MIN;

			// if we set brake to max from boot up the ESC thinks the brake
			// sensor is broken
			if(millis() > 5000)
				brake = BRAKE_READ_MAX;
			else
				brake = BRAKE_READ_MIN;
		}

		throttle = map(throttle, THROTTLE_READ_MIN, THROTTLE_READ_MAX, THROTTLE_MIN, THROTTLE_MAX);
		brake = map(brake, BRAKE_READ_MIN, BRAKE_READ_MAX, BRAKE_MIN, BRAKE_MAX);

		transmitInputInfo(throttle, brake);
		lastTransmit = now;
	}

	static docgreen_status_t status;
	if(ScooterSerial.available() && receivePacket(&status))
	{
		static bool hadButton = false;
		if(status.buttonPress)
		{
			hadButton = true;
		}
		else if(hadButton)
		{
			hadButton = false;
			pressedButtons |= BUTTON_POWER;
		}

		display.clearDisplay();
		display.setCursor(0, 0);

		static bool inDrive = false;
		if(!inDrive && status.speed > 5000)
		{
			inDrive = true;
		}
		else if(inDrive && status.speed < 1000)
		{
			inDrive = false;

			//reset buttons as they might have been pressed during drive
			getAndResetButtons();
		}

#ifdef REENABLE_LIGHTS_AFTER_ERROR
		static bool hadError = false;
		static bool lightShouldBeOn = false;
		if(status.errorCode != 0)
		{
			hadError = true;
		}
		else if(hadError)
		{
			// if the lights were on before turn on the lights after an error
			if(lightShouldBeOn && !status.lights)
				setLight(true);

			hadError = false;
		}
		else if(status.lights != lightShouldBeOn)
		{
			lightShouldBeOn = status.lights;
		}
#endif

		//if(true) showDebugMenu(status); else
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
}
