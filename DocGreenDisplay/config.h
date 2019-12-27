#pragma once

#define ScooterSerial Serial
#define RX_DISABLE UCSR0B &= ~_BV(RXEN0);
#define RX_ENABLE  UCSR0B |=  _BV(RXEN0);

#define TRANSMIT_INTERVAL 50

#define THROTTLE_PIN A0
#define BRAKE_PIN A2
#define MECHANICAL_BRAKE_PIN 4

// the hall sensors have values between 0.8V and 4.3V resulting in 179-877 for a 10bit 5V ADC
// for some reason on the arduino nano the reading goes from 225 to 1024.
#define THROTTLE_READ_MIN 225
#define THROTTLE_READ_MAX 1024
#define BRAKE_READ_MIN 225
#define BRAKE_READ_MAX 1024
