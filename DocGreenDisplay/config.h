#pragma once

// packet interval in ms
#define TRANSMIT_INTERVAL 50

#define THROTTLE_PIN 39
#define BRAKE_PIN 36
#define MECHANICAL_BRAKE_PIN 34

// the hall sensors have values between 0.8V and 5V resulting in 179-1024 for a 10bit 5V ADC
#define THROTTLE_READ_MIN 225
#define THROTTLE_READ_MAX 1024
#define BRAKE_READ_MIN 225
#define BRAKE_READ_MAX 1024

// min/max values on the bus
#define THROTTLE_MIN 0x2C
#define THROTTLE_MAX 0xC5
#define BRAKE_MIN 0x2C
#define BRAKE_MAX 0xB5

// mDNS name, the webinterface will be available at http://<name>.local
#define MDNS_DOMAIN_NAME "scooter"
