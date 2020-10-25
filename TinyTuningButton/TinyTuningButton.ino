#include <stdint.h>
#include <EEPROM.h>

#define UART_RX_PIN 1
#include "./protocol.h"

static bool isTuned = false;

void toggleTuning()
{
	isTuned = !isTuned;
	EEPROM.write(0, isTuned ? (uint8_t)0xAA : (uint8_t)0x00);

	if(isTuned)
		SEND_REPEAT(setMaxSpeed(35));
	else
		SEND_REPEAT(setMaxSpeed(20));

	SEND_REPEAT(setEcoMode(false));
	delay(500);
	SEND_REPEAT(setEcoMode(true));
	delay(1000);
	SEND_REPEAT(setEcoMode(false));
}

uint8_t buttonCount = 0;
void buttonInterruptHandler()
{
	static unsigned long lastChange = 0;
	unsigned long now = millis();
	unsigned long diff = now - lastChange;

	if(diff < 100) // 100ms debounce
		return;
	if(diff > 1500)
		buttonCount = 0;

	lastChange = now;
	buttonCount++;
}

void setup()
{
    ScooterSerial.begin(115200);
	ScooterSerial.stopListening();

	delay(3000);

	if(EEPROM.read(0) == 0xAA)
	{
		SEND_REPEAT(setMaxSpeed(35));
		isTuned = true;

		SEND_REPEAT(setEcoMode(true));
		delay(1000);
		SEND_REPEAT(setEcoMode(false));
	}

	pinMode(2, INPUT_PULLUP);
	attachInterrupt(2, buttonInterruptHandler, FALLING);
}

void loop()
{
	if(isTuned && buttonCount >= 3)
	{
		buttonCount = 0;
		toggleTuning();
	}
	else if(!isTuned && buttonCount >= 5)
	{
		buttonCount = 0;
		toggleTuning();
	}

	delay(300);
}