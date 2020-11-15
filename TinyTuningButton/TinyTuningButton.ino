#include <stdint.h>
#include <EEPROM.h>

#define UART_RX_PIN 1
#include "./protocol.h"

static bool isTuned = false;
static bool isLocked = false;

void toggleTuning(bool save)
{
	isTuned = !isTuned;
	if(save)
		EEPROM.write(0, isTuned ? (uint8_t)0xAA : (uint8_t)0x00);

	if(isTuned)
		SEND_REPEAT(setMaxSpeed(35));
	else
		SEND_REPEAT(setMaxSpeed(20));

	SEND_REPEAT(setEcoMode(true));
	delay(1000);
	SEND_REPEAT(setEcoMode(false));
}

void setup()
{
	pinMode(2, INPUT_PULLUP);

	ScooterSerial.begin(115200);
	ScooterSerial.stopListening();

	delay(1000);

	if(EEPROM.read(0) == 0xAA)
		toggleTuning(false);
}

void loop()
{
	static uint8_t pressCount = 0;
	static bool wasPressed = false;
	static unsigned long lastPress = 0;

	bool isPressed = digitalRead(2) == LOW;
	unsigned long diff = millis() - lastPress;

	if(wasPressed && !isPressed && diff > 50)
	{
		if(diff > 1500)
			pressCount = 0;

		pressCount++;
		lastPress = millis();
	}
	wasPressed = isPressed;

	if(diff > 500)
	{
		if(pressCount >= 8)
		{
			isLocked = !isLocked;
			SEND_REPEAT(setLock(isLocked));

			pressCount = 0;
		}
		else if((!isTuned && pressCount >= 5)
			|| (isTuned && pressCount >= 3))
		{
			toggleTuning(true);
			pressCount = 0;
		}
	}
}
