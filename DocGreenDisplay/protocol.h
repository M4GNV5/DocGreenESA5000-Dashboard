#pragma once

#include <stdbool.h>
#include <stdint.h>

#if defined(__AVR__)
#ifndef ScooterSerial
#define ScooterSerial Serial
#endif
#ifndef RX_DISABLE
#define RX_DISABLE UCSR0B &= ~_BV(RXEN0);
#endif
#ifndef RX_ENABLE
#define RX_ENABLE  UCSR0B |=  _BV(RXEN0);
#endif
#elif defined(ARDUINO_ARCH_ESP32)
#define ScooterSerial Serial2
#define RX_ENABLE (0)
#define RX_DISABLE (0)
#else
#error Unknown Microcontroller
#endif

typedef struct
{
	// controlling what is sent to the controller
	bool enableStatsRequests;
	uint8_t throttle;
	uint8_t brake;

	// data received from the controller
	bool ecoMode;
	bool shuttingDown;
	bool lights;
	bool buttonPress;
	uint8_t errorCode;
	uint8_t soc; //state of charge (battery %)
	uint16_t speed; // meters per hour

	uint32_t totalOperationTime; // in seconds
	uint32_t timeSinceBoot; // in seconds
	uint16_t voltage; // in 1/10 V
	int16_t current; // in 1/10 A

	uint32_t mainboardVersion; // should be 0x0003027d
	uint32_t odometer; // in meter
} docgreen_status_t;

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
    *(uint16_t *)&data[6] = (speed * 252) / 10;
    *(uint16_t *)&data[8] = calculateChecksum(data + 2);

    RX_DISABLE;
    ScooterSerial.write(data, sizeof(data) / sizeof(uint8_t));
    RX_ENABLE;
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

    RX_DISABLE;
    ScooterSerial.write(data, sizeof(data) / sizeof(uint8_t));
    RX_ENABLE;
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

const uint8_t regularInputPacket[] = {
	0x07, 0x25, 0x60, 0x05, //header
	0x04,
	0x00, // throttle
	0x00, // brake
	0x00, 0x00,
};
const uint8_t regularInputPacket2[] = {
	0x09, 0x27, 0x63, 0x07, //header
	0x06,
	0x00, // throttle
	0x00, // brake
	0x00, 0x00,
	0x00, 0x04,
};
const uint8_t detailRequestInputPacket[] = {
	0x07, 0x25, 0x64, // header
	0x37, 0x32, //request id (can be 37 32, 1F 32, 1F 1C or BE 06)
	0x03,
	0x28, // throttle
	0x29, // brake
	0x00,
};
void transmitInputInfo(docgreen_status_t& status)
{
	static int counter = 0;
	uint8_t data[sizeof(regularInputPacket2) + 2];

	if(counter > 5)
	{
		memcpy(data, detailRequestInputPacket, sizeof(detailRequestInputPacket));
		data[6] = status.throttle;
		data[7] = status.brake;

		// XXX: we only request packets with known contents
		static int detailedReqCounter = 0;
		if(detailedReqCounter == 0)
		{
			data[3] = 0x37; // request detailed info 1
			data[4] = 0x32;
			detailedReqCounter++;
		}
		else // if(detailedReqCounter == 1)
		{
			data[3] = 0x1F; // request detailed info 2
			data[4] = 0x32;
			detailedReqCounter = 0;
		}
	}
	else if(counter == 4)
	{
		memcpy(data, regularInputPacket2, sizeof(regularInputPacket2));
		data[5] = status.throttle;
		data[6] = status.brake;
	}
	else
	{
		memcpy(data, regularInputPacket, sizeof(regularInputPacket));
		data[5] = status.throttle;
		data[6] = status.brake;
	}

	if(counter > 5 || (counter > 4 && !status.enableStatsRequests))
		counter = 0;
	else
		counter++;

	uint8_t len = data[0] + 2;
	*(uint16_t *)&data[len] = calculateChecksum(data);

	RX_DISABLE;
	ScooterSerial.write(0x55);
	ScooterSerial.write(0xAA);
	ScooterSerial.write(data, len + 2);
	RX_ENABLE;
}

void sendBleConnected()
{
	uint8_t data[] = {
		0x55, 0xAA, // start
		0x06, 0xF4, 0x06, 0x30,
		0x1C, 0x81, 0x18, 0xB5
    };

    RX_DISABLE;
    ScooterSerial.write(data, sizeof(data) / sizeof(uint8_t));
    RX_ENABLE;
}

void parseDetailedInfo(docgreen_status_t *status, uint8_t *buff)
{
	if(buff[0] != 0x34) // expect length 52
		return;

	// XXX we assume our architecture uses LE order here
	if(buff[3] == 0x00)
	{
		status->totalOperationTime = *(uint32_t *)&buff[4];
		status->timeSinceBoot = *(uint32_t *)&buff[20];
		status->voltage = *(uint16_t *)&buff[46];
		status->current = *(int16_t *)&buff[48];
	}
	else if(buff[3] == 0x28)
	{
		status->mainboardVersion = *(uint32_t *)&buff[10];
		status->odometer = *(uint32_t *)&buff[34];
	}
}

void parseMotorInfoPacket(docgreen_status_t *status, uint8_t *buff)
{
	if(buff[0] != 0x0b) // expect length 11
		return;

	status->ecoMode = buff[4] == 0x02;
	status->shuttingDown = buff[5] == 0x08;
	status->lights = buff[6] == 0x01;
	status->buttonPress = buff[10] == 0x01;
	status->errorCode = buff[11];
	status->soc = buff[12];
	status->speed = *(uint16_t *)&buff[8]; // XXX we assume our architecture uses LE order here
}

uint8_t readBlocking()
{
	while(!ScooterSerial.available())
		delay(1);

	return ScooterSerial.read();
}
bool receivePacket(docgreen_status_t *status)
{
	if(readBlocking() != 0x55)
		return false;
	if(readBlocking() != 0xAA)
		return false;

	uint8_t buff[256];

	uint8_t len = readBlocking();
	buff[0] = len;
	if(len >= sizeof(buff) - 4)
		return false;

	uint8_t addr = readBlocking();
	buff[1] = addr;

	for(int i = 0; i < len; i++)
	{
		buff[i + 2] = readBlocking();
	}

	uint16_t actualChecksum = (uint16_t)readBlocking() | ((uint16_t)readBlocking() << 8);
	uint16_t expectedChecksum = calculateChecksum(buff);
	if(actualChecksum != expectedChecksum)
		return false;

	switch(addr)
	{
		case 0x11:
			parseDetailedInfo(status, buff);
			break;
		case 0x28:
			parseMotorInfoPacket(status, buff);
			break;
	}
	return true;
}
