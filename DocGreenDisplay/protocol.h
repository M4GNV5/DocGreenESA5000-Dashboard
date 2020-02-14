#include <stdbool.h>
#include <stdint.h>

#ifndef ScooterSerial
#define ScooterSerial Serial
#endif
#ifndef RX_DISABLE
#define RX_DISABLE UCSR0B &= ~_BV(RXEN0);
#endif
#ifndef RX_ENABLE
#define RX_ENABLE  UCSR0B |=  _BV(RXEN0);
#endif

typedef struct
{
	bool ecoMode;
	bool shuttingDown;
	bool lights;
	bool buttonPress;
	uint8_t errorCode;
	uint8_t soc; //state of charge (battery %)
	uint16_t speed; // meters per hour
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

void transmitInputInfo(uint8_t throttle, uint8_t brake)
{
	static int counter = 0;

	uint8_t fifthHeader[] = {
		0x09, 0x27, 0x63, 0x07, 0x06,
	};
	uint8_t regularHeader[] = {
		0x07, 0x25, 0x60, 0x05, 0x04,
	};

	uint8_t data[] = {
		0, 0, 0, 0, 0, // header, will be either regularHeader or fifthHeader
		throttle, // range 0x2C - 0xC5 
		brake, // range 0x2C - 0xB5
		0x00, 0x00, // ?
		0x00, 0x04, // ?, only present on every fith package
		0x00, 0x00, // checksum
	};

	uint8_t len;
	if(counter == 4)
	{
		len = 13;
		memcpy(data, fifthHeader, 5);

		counter = 0;
	}
	else
	{
		len = 11;
		memcpy(data, regularHeader, 5);

		counter++;
	}

	// XXX we assume our architecture uses LE order here
	*(uint16_t *)&data[len - 2] = calculateChecksum(data);

	RX_DISABLE;
	ScooterSerial.write(0x55);
	ScooterSerial.write(0xAA);
	ScooterSerial.write(data, len);
	RX_ENABLE;
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
	if(len >= 256 - 4)
		return false;

	uint8_t addr = readBlocking();
	buff[1] = addr;

	if(addr != 0x28)
		return false; // the packet is not for us

	for(int i = 0; i < len; i++)
	{
		uint8_t curr = readBlocking();
		buff[i + 2] = curr;
	}

	uint16_t actualChecksum = (uint16_t)readBlocking() | ((uint16_t)readBlocking() << 8);
	uint16_t expectedChecksum = calculateChecksum(buff);
	if(actualChecksum != expectedChecksum)
		return false;

	status->ecoMode = buff[4] == 0x02;
	status->shuttingDown = buff[5] == 0x08;
	status->lights = buff[6] == 0x01;
	status->buttonPress = buff[10] == 0x01;
	status->errorCode = buff[11];
	status->soc = buff[12];
	status->speed = *(uint16_t *)&buff[8]; // XXX we assume our architecture uses LE order here

	return true;
}