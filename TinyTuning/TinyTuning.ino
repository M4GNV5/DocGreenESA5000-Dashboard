#include <EEPROM.h>
#include "./protocol.h"

#define SEND_REPEAT(func) do { \
		func; \
		delay(7); \
		func; \
		delay(11); \
		func; \
	} while(0)

docgreen_tiny_status_t scooterStatus;
static uint8_t throttlePressDuration = 0;
static uint8_t brakePressDuration = 0;

void setup()
{
	ScooterSerial.begin(115200);

	scooterStatus.isTuned = EEPROM.read(0) == 0xAA;
	if(scooterStatus.isTuned)
		SEND_REPEAT(setMaxSpeed(35));
}

void loop()
{
	if(receivePacket(scooterStatus))
	{
		if(scooterStatus.speed > 5000)
		{
			throttlePressDuration = 0;
			brakePressDuration = 0;
			return;
		}

		if(scooterStatus.throttle > 0x70)
			throttlePressDuration++;
		else
			throttlePressDuration = 0;
		
		if(scooterStatus.brake > 0x70)
			brakePressDuration++;
		else
			brakePressDuration = 0;

		if(throttlePressDuration == 20 && brakePressDuration < 5)
		{
			// hold throttle to tune / detune
			scooterStatus.isTuned = !scooterStatus.isTuned;

			SEND_REPEAT(setEcoMode(!scooterStatus.ecoMode));
			SEND_REPEAT(setMaxSpeed(scooterStatus.isTuned ? 35 : 20));
			delay(200);
			SEND_REPEAT(setEcoMode(scooterStatus.ecoMode));

			EEPROM.write(0, scooterStatus.isTuned ? 0xAA : 0);

			throttlePressDuration = 0;
		}
		else if(throttlePressDuration < 5 && brakePressDuration > 20
			&& scooterStatus.buttonPress)
		{
			// hold brake and press button to lock / unlock
			scooterStatus.isLocked = !scooterStatus.isLocked;

			SEND_REPEAT(setLight(!scooterStatus.lights));
			SEND_REPEAT(setLock(scooterStatus.isLocked));
			delay(200);
			SEND_REPEAT(setLight(scooterStatus.lights));
		}
	}
}