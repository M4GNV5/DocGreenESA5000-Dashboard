#pragma once

#define LOCK_PASSWORD \
	BUTTON_POWER, \
	BUTTON_UP, \
	BUTTON_DOWN, \
	BUTTON_CANCEL, \
	BUTTON_RIGHT,

#define ScooterSerial Serial
#define RX_DISABLE UCSR0B &= ~_BV(RXEN0);
#define RX_ENABLE  UCSR0B |=  _BV(RXEN0);

#define TRANSMIT_INTERVAL 50

#define THROTTLE_PIN A0
#define BRAKE_PIN A2
#define MECHANICAL_BRAKE_PIN 4

// with this defined the scooter will ask for the password after turning on.
#define LOCK_ON_BOOT

// with this defined the OLED will show a short animation on boot.
#define SHOW_INTRO_ON_BOOT

// set the max speed to this value after boot
// a value of 20 is the factory (and legal) default
#define DEFAULT_MAX_SPEED 20

// after an error occured the light often turns off as the ECU uses the light
// to visualize the error code. With this option defined light automatically turns
// back on after an error
#define REENABLE_LIGHTS_AFTER_ERROR

// the hall sensors have values between 0.8V and 4.3V resulting in 179-877 for a 10bit 5V ADC
// for some reason on the arduino nano the reading goes from 225 to 1024.
#define THROTTLE_READ_MIN 225
#define THROTTLE_READ_MAX 1024
#define BRAKE_READ_MIN 225
#define BRAKE_READ_MAX 1024
