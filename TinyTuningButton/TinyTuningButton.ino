#include <stdint.h>
#include <EEPROM.h>

#define UART_RX_PIN 1
#include "./protocol.h"

static bool isTuned = false;
static bool isLocked = false;
static volatile uint8_t pressCount = 0;
static volatile uint32_t lastPress = 0;

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

void buttonInterrupt()
{
	uint32_t now = millis();
	if(lastPress + 50 > now)
		return;

	lastPress = now;
	pressCount++;
}

void setup()
{
	pinMode(2, INPUT_PULLUP);
	attachInterrupt(0, buttonInterrupt, CHANGE);

	ScooterSerial.begin(115200);
	ScooterSerial.stopListening();

	delay(2000);

	if(EEPROM.read(0) == 0xAA)
		toggleTuning(false);
}

void loop()
{
	if(lastPress + 500 < millis())
	{
		uint8_t oldCount = pressCount;
		pressCount = 0;

		if(oldCount >= 8)
			toggleTuning(true);
	}

	delay(100);
}
