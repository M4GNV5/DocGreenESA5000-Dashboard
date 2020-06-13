#include <Arduino.h>
#include <stdint.h>

#include "config.h"
#include "protocol.h"
#include "oled-ui.hpp"

#define THROTTLE_MIN 0x2C
#define THROTTLE_MAX 0xC5
#define BRAKE_MIN 0x2C
#define BRAKE_MAX 0xB5

void setup()
{
	ScooterSerial.begin(115200);

	pinMode(MECHANICAL_BRAKE_PIN, INPUT);

	initializeOledUi();

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
	static docgreen_status_t status = {
		.enableStatsRequests = true,
	};

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

		detectButtonPress(throttle, brake, brakeLever);

		if(brakeLever)
		{
			throttle = THROTTLE_READ_MIN;

			// XXX: in the orginal configuration pulling the mechanical brake
			// lever makes the scooter brake with the maximum power on the
			// electrical brake, however that feels very harsh and dangerous
			uint16_t minBrake = map(40, 0, 100, THROTTLE_READ_MIN, THROTTLE_READ_MAX);

			// allow the user to manually brake >40%
			if(minBrake > brake)
				brake = minBrake;
		}

		throttle = map(throttle, THROTTLE_READ_MIN, THROTTLE_READ_MAX, THROTTLE_MIN, THROTTLE_MAX);
		brake = map(brake, BRAKE_READ_MIN, BRAKE_READ_MAX, BRAKE_MIN, BRAKE_MAX);

		status.throttle = throttle;
		status.brake = brake;
		transmitInputInfo(status);
		lastTransmit = now;
	}

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

		// TODO do this more often?
		// not only after a packet from the motor controller was received?
		updateOledUi(status);
	}
}
