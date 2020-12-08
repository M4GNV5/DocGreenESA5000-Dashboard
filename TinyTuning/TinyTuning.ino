#include <EEPROM.h>
#include "./protocol.h"

#define PACKETS_UNTIL_ACTION 50

docgreen_tiny_status_t scooterStatus;
static uint8_t throttlePressDuration = 0;
static uint8_t brakePressDuration = 0;

void setup()
{
	ScooterSerial.begin(115200);

	scooterStatus.isTuned = EEPROM.read(0) == 0xAA;
	if(scooterStatus.isTuned)
		SEND_REPEAT(setMaxSpeed(35));

	scooterStatus.isLocked = false;
	scooterStatus.inDrive = false;
}

void loop()
{
	if(receivePacket(scooterStatus))
	{
		if(!scooterStatus.inDrive && scooterStatus.speed > 2000)
		{
			scooterStatus.inDrive = true;
		}
		else if(scooterStatus.inDrive && scooterStatus.speed < 200)
		{
			throttlePressDuration = 0;
			brakePressDuration = 0;
			scooterStatus.inDrive = false;
		}

		if(scooterStatus.inDrive)
			return;

		if(scooterStatus.throttle > 0x70)
			throttlePressDuration++;
		else
			throttlePressDuration = 0;

		if(scooterStatus.brake > 0x70)
			brakePressDuration++;
		else
			brakePressDuration = 0;

		if(throttlePressDuration > PACKETS_UNTIL_ACTION && brakePressDuration < 5)
		{
			// hold throttle to tune / detune
			scooterStatus.isTuned = !scooterStatus.isTuned;
			throttlePressDuration = 0;

			SEND_REPEAT(setMaxSpeed(scooterStatus.isTuned ? 35 : 20));
			SEND_REPEAT(setEcoMode(!scooterStatus.ecoMode));
			delay(1000);
			SEND_REPEAT(setEcoMode(scooterStatus.ecoMode));

			EEPROM.write(0, scooterStatus.isTuned ? 0xAA : 0);
		}
		else if(throttlePressDuration < 5 && brakePressDuration > PACKETS_UNTIL_ACTION
			&& scooterStatus.buttonPress)
		{
			// hold brake and press button to lock / unlock
			scooterStatus.isLocked = !scooterStatus.isLocked;
			brakePressDuration = 0;

			SEND_REPEAT(setLock(scooterStatus.isLocked));
		}
	}
}
