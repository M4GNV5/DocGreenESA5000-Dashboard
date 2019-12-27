#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdint.h>
#include "config.h"
#include "icons.h"

#define THROTTLE_MIN 0x2C
#define THROTTLE_MAX 0xC5
#define BRAKE_MIN 0x2C
#define BRAKE_MAX 0xB5

typedef struct
{
	bool ecoMode;
	bool shuttingDown;
	bool lights;
	bool buttonPress;
	uint8_t errorCode;
	uint8_t soc; //state of charge (battery %)
	uint16_t speed; // meters per hour
} docgreen_status_t;

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
uint8_t password[] = {
	BUTTON_POWER,
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_CANCEL,
	BUTTON_RIGHT,
};

uint16_t calculateChecksum(uint8_t *data)
{
	uint8_t len = data[0] + 2;
	uint16_t sum = 0;
	for(int i = 0; i < len; i++)
		sum += data[i];

	sum ^= 0xFFFF;
	return sum;
}

void transmitPacket(uint8_t throttle, uint8_t brake)
{
	static int counter = 0;

	uint8_t fifthHeader[] = {
		0x09, 0x27, 0x63, 0x07, 0x06,
	};
	uint8_t regularHeader[] = {
		0x07, 0x25, 0x60, 0x05, 0x04,
	};

	uint8_t data[] = {
		0, 0, 0, 0, 0, // header, will be either regularHeader or fifthHeader
		throttle, // range 0x2C - 0xC5 
		brake, // range 0x2C - 0xB5
		0x00, 0x00, // ?
		0x00, 0x04, // ?, only present on every fith package
		0x00, 0x00, // checksum
	};

	uint8_t len;
	if(counter == 4)
	{
		len = 13;
		memcpy(data, fifthHeader, 5);

		counter = 0;
	}
	else
	{
		len = 11;
		memcpy(data, regularHeader, 5);

		counter++;
	}

	// XXX we assume our architecture uses LE order here
	*(uint16_t *)&data[len - 2] = calculateChecksum(data);

	RX_DISABLE;
	ScooterSerial.write(0x55);
	ScooterSerial.write(0xAA);
	ScooterSerial.write(data, len);
	RX_ENABLE;
}

uint8_t readBlocking()
{
	while(!ScooterSerial.available())
		delay(1);

	return ScooterSerial.read();
}
bool receivePacket(docgreen_status_t *status)
{
	if(readBlocking() != 0x55)
		return false;
	if(readBlocking() != 0xAA)
		return false;

	uint8_t buff[256];

	uint8_t len = readBlocking();
	buff[0] = len;
	if(len >= 256 - 4)
		return false;

	uint8_t addr = readBlocking();
	buff[1] = addr;

	if(addr != 0x28)
		return false; // the packet is not for us

	uint16_t sum = len + addr;
	for(int i = 0; i < len; i++)
	{
		uint8_t curr = readBlocking();
		buff[i + 2] = curr;
		sum += curr;
	}

	uint16_t actualChecksum = (uint16_t)readBlocking() | ((uint16_t)readBlocking() << 8);
	uint16_t expectedChecksum = calculateChecksum(buff);
	if(actualChecksum != expectedChecksum)
		return false;

	status->ecoMode = buff[4] == 0x02;
	status->shuttingDown = buff[5] == 0x08;
	status->lights = buff[6] == 0x01;
	status->buttonPress = buff[10] == 0x01;
	status->errorCode = buff[11];
	status->soc = buff[12];
	status->speed = *(uint16_t *)&buff[8]; // XXX we assume our architecture uses LE order here

	return true;
}

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
	for(int i = 127; i >= 0; i--)
	{
		display.clearDisplay();
		display.drawBitmap(
			0, (display.height() - SCOOTER_HEIGHT) / 2,
			scooter_bitmap, SCOOTER_WIDTH, SCOOTER_HEIGHT, 1
		);
		display.drawLine(0, i, display.width(), i, SSD1306_WHITE);
		display.display();

		delay(1);
	}
}

void showInfoScreen(docgreen_status_t& status)
{
	display.setTextSize(1);
	display.println("SPEED");
	uint8_t speed = status.speed / 1000;
	if(status.speed % 1000 >= 500)
		speed++;
	display.setTextSize(2);
	display.println(speed);

	display.setCursor(0, display.getCursorY() + 10);

	display.setTextSize(1);
	display.println("SOC");
	display.setTextSize(2);
	if(status.soc == 100)
		display.println("FU");
	else
		display.println(status.soc);

	display.setTextSize(1);

	display.setCursor(0, display.getCursorY() + 10);
	display.println(status.ecoMode ? "ECO" : "SPORT");

	display.setCursor(0, display.getCursorY() + 3);
	display.println(status.lights ? "NIGHT" : "DAY");

	// TODO show milage etc.
}

void showDebugScreen(docgreen_status_t& status)
{
	display.setTextSize(1);
	display.println("DEBUG");
	display.println();
	display.println("soc");
	display.println(status.soc);
	display.println("gas");
	display.println(lastThrottle);
	display.println("brake");
	display.println(lastBrake);
	display.println("press");
	display.print("0x");
	display.println(pressedButtons, 16);
}

void showLockOptionMenu(uint8_t button)
{
	if(button & BUTTON_CANCEL)
		isLocked = true;

	display.setTextSize(1);
	display.println("press\nbrake\nto\nlock");
}

void showIntroMenu(uint8_t button)
{
	if(button & BUTTON_CANCEL)
		showIntro();

	display.setTextSize(1);
	display.println("press\nbrake\nto\nshow\nintro");
}

void showMainMenu(docgreen_status_t& status)
{
	uint8_t button = getAndResetButtons();

	static int menu = 0;
	if(button & BUTTON_RIGHT)
	{
		menu++;
		if(menu > 3)
			menu = 0;
	}
	if(button & BUTTON_LEFT)
	{
		menu--;
		if(menu < 0)
			menu = 3;
	}

	switch(menu)
	{
		case 0:
			showInfoScreen(status);
			break;
		case 1:
			showLockOptionMenu(button);
			break;
		case 2:
			showIntroMenu(button);
			break;
		case 3:
			showDebugScreen(status);
			break;
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

		transmitPacket(throttle, brake);
		lastTransmit = now;
	}

	docgreen_status_t status;
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
		if(status.speed > 5000)
			inDrive = true;
		else if(status.speed < 1000)
			inDrive = false;

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
