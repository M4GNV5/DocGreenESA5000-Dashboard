#pragma once

#include <stdint.h>
#include <SoftwareSerial.h>

// we could use protocol.h from DocGreenDisplay, but it's docgreen_status_t
// structure alone needs 12% of the ATtiny's RAM (31 of 256 byte)
// thus below are parts rewritten without using buffers or large structs

#define UART_TX_PIN 3
#define UART_RX_PIN 4
SoftwareSerial ScooterSerial(UART_RX_PIN, UART_TX_PIN);

typedef struct
{
	uint8_t throttle;
	uint8_t brake;

	uint16_t speed; // meters per hour

	uint8_t ecoMode : 1;
	uint8_t lights : 1;
	uint8_t buttonPress : 1;

	// used and set in TinyTuning.ino, just defined here to save 2 bytes RAM
	uint8_t isTuned : 1;
	uint8_t isLocked : 1;
} docgreen_tiny_status_t;

uint16_t calculateChecksum(uint8_t *data)
{
	uint8_t len = data[0] + 2;
	uint16_t sum = 0;
	for(int i = 0; i < len; i++)
		sum += data[i];

	sum ^= 0xFFFF;
	return sum;
}

void setMaxSpeed(uint8_t speed)
{
    uint8_t data[] = {
        0x55, 0xAA, 0x04, 0x22, 0x01, 0xF2,
        0, 0, //rpm
        0, 0, //checksum
    };

    // XXX we assume our architecture uses LE order here
    *(uint16_t *)&data[6] = ((uint16_t)speed * 252) / 10;
    *(uint16_t *)&data[8] = calculateChecksum(data + 2);

    ScooterSerial.write(data, sizeof(data) / sizeof(uint8_t));
}

static void setOption(uint8_t id, bool enabled)
{
	uint8_t data[] = {
        0x55, 0xAA, 0x04, 0x22, 0x01, id,
        enabled ? (uint8_t)0x01 : (uint8_t)0x00,
		0x00,
        0, 0, //checksum
    };

    *(uint16_t *)&data[8] = calculateChecksum(data + 2);

    ScooterSerial.write(data, sizeof(data) / sizeof(uint8_t));
}
void setEcoMode(bool enabled)
{
	setOption(0x7C, enabled);
}
void setLock(bool enabled)
{
	setOption(0x7D, enabled);
}
void setLight(bool enabled)
{
	setOption(0xF0, enabled);
}

uint8_t readWithDefault()
{
	// give the scooter some time
	if(!ScooterSerial.available())
		delay(3);

	if(ScooterSerial.available())
		return ScooterSerial.read();
	else
		return 0x00;
}
bool receivePacket(docgreen_tiny_status_t& status)
{
	if(readWithDefault() != 0x55)
		return false;
	if(readWithDefault() != 0xAA)
		return false;

	uint8_t len = readWithDefault();
	uint8_t addr = readWithDefault();
	if(len == 0 || (addr != 0x25 && addr != 0x28))
		return false;

	uint16_t sum = 0;
	for(int i = 0; i < len; i++)
	{
		uint8_t val = readWithDefault();
		sum += val;

		if(addr == 0x25) // input info packet
		{
			if(i == 5)
				status.throttle = val;
			else if(i == 6)
				status.brake = val;
		}
		else if(addr == 0x28)
		{
			switch(i)
			{
				case 4:
					status.ecoMode = val == 0x02;
					break;
				case 6:
					status.lights = val == 0x01;
					break;
				case 8:
					status.speed = (uint16_t)val << 8;
					break;
				case 9:
					status.speed |= val;
					break;
				case 10:
					status.buttonPress = val == 0x01;
					break;
			}
		}
	}

	uint16_t actualChecksum = (uint16_t)readWithDefault() | ((uint16_t)readWithDefault() << 8);
	sum ^= 0xFFFF;
	if(actualChecksum != sum)
		return false;

	return true;
}
