#include <Arduino.h>
#include <stdint.h>

#include "config.h"
#include "state.hpp"
#include "protocol.h"

#include "wifi.hpp"
#include "oled-ui.hpp"
#include "webserver.hpp"

Preferences preferences;

docgreen_status_t scooterStatus = {
	.enableStatsRequests = true,
};

bool reenableLightsAfterError = false;

void setup()
{
#ifdef ARDUINO_ARCH_ESP32
	ScooterSerial.begin(115200, SERIAL_8N1, 27, 26);

	analogReadResolution(10);
	analogSetAttenuation(ADC_11db);
#else
	ScooterSerial.begin(115200);
#endif

	pinMode(MECHANICAL_BRAKE_PIN, INPUT);
	preferences.begin("scooter", false);

	wifiSetup();
	initializeOledUi();
	webServerSetup();

	if(wifiApEnabled || wifiStaEnabled)
	{
		MDNS.begin(MDNS_DOMAIN_NAME); // TODO allow the user to configure this?
		MDNS.addService("http", "tcp", 80);
	}

	configuredSpeed = preferences.getUChar(PREFERENCE_MAX_SPEED, 20);
	if(configuredSpeed != 20)
	{
		// send default max speed 3 times, to make sure the packet isn't lost
		setMaxSpeed(configuredSpeed);
		delay(50);
		setMaxSpeed(configuredSpeed);
		delay(50);
		setMaxSpeed(configuredSpeed);
	}

	if(preferences.getUChar(PREFERENCE_REENABLE_LIGHT, 0))
		reenableLightsAfterError = true;
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

		scooterStatus.throttle = throttle;
		scooterStatus.brake = brake;
		transmitInputInfo(scooterStatus);
		lastTransmit = now;
	}

	if(ScooterSerial.available() && receivePacket(&scooterStatus))
	{
		static bool hadButton = false;
		if(scooterStatus.buttonPress)
		{
			hadButton = true;
		}
		else if(hadButton)
		{
			hadButton = false;
			pressedButtons |= BUTTON_POWER;
		}

		if(reenableLightsAfterError)
		{
			static bool hadError = false;
			static bool lightShouldBeOn = false;
			if(scooterStatus.errorCode != 0)
			{
				hadError = true;
			}
			else if(hadError)
			{
				// if the lights were on before turn on the lights after an error
				if(lightShouldBeOn && !scooterStatus.lights)
					setLight(true);

				hadError = false;
			}
			else if(scooterStatus.lights != lightShouldBeOn)
			{
				lightShouldBeOn = scooterStatus.lights;
			}
		}

		// TODO do this more often?
		// not only after a packet from the motor controller was received?
		updateOledUi(scooterStatus);
	}

	webServerLoop();
	delay(1);
}
