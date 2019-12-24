#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdint.h>
#include "config.h"

#define THROTTLE_MIN 0x2C
#define THROTTLE_MAX 0xC5
#define BRAKE_MIN 0x2C
#define BRAKE_MAX 0xB5

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

Adafruit_SSD1306 display(128, 32, &Wire, -1);

uint16_t calculateChecksum(uint8_t *data)
{
	uint8_t len = data[0] + 2;
	uint16_t sum = 0;
	for(int i = 0; i < len; i++)
		sum += data[i];

	sum ^= 0xFFFF;
	return sum;
}

void transmitPacket(uint8_t throttle, uint8_t brake)
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

	uint16_t sum = len + addr;
	for(int i = 0; i < len; i++)
	{
		uint8_t curr = readBlocking();
		buff[i + 2] = curr;
		sum += curr;
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

void setup()
{
	ScooterSerial.begin(115200);

	pinMode(MECHANICAL_BRAKE_PIN, INPUT);

	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
	display.display();
	delay(1000);

	display.setRotation(1);
	display.setTextSize(1);
	display.setTextColor(SSD1306_WHITE);

	display.clearDisplay();
	display.setCursor(0, 0);
	display.println("NO\nDATA");
	display.display();
}

void loop()
{
	static uint16_t lastThrottle = 0;
	static uint16_t lastBrake = 0;

	static uint32_t lastTransmit = 0;
	uint32_t now = millis();
	if(now - lastTransmit > TRANSMIT_INTERVAL)
	{
		uint16_t throttle = analogRead(THROTTLE_PIN);
		uint16_t brake = analogRead(BRAKE_PIN);

		if(digitalRead(MECHANICAL_BRAKE_PIN) == HIGH)
		{
			throttle = THROTTLE_READ_MIN;

			// XXX: in the orginal configuration pulling the mechanical brake
			// lever makes the scooter brake with the maximum power on the
			// electrical brake, however that feels very harsh and dangerous
			brake = map(40, 0, 100, THROTTLE_READ_MIN, THROTTLE_READ_MAX);
		}

		if(throttle < THROTTLE_READ_MIN)
			throttle = THROTTLE_READ_MIN;
		if(throttle > THROTTLE_READ_MAX)
			throttle = THROTTLE_READ_MAX;
		if(brake < BRAKE_READ_MIN)
			brake = BRAKE_READ_MIN;
		if(brake > BRAKE_READ_MAX)
			brake = BRAKE_READ_MAX;

		lastThrottle = throttle;
		lastBrake = brake;

		throttle = map(throttle, THROTTLE_READ_MIN, THROTTLE_READ_MAX, THROTTLE_MIN, THROTTLE_MAX);
		brake = map(brake, BRAKE_READ_MIN, BRAKE_READ_MAX, BRAKE_MIN, BRAKE_MAX);

		transmitPacket(throttle, brake);

		lastTransmit = now;
	}

	docgreen_status_t status;
	if(ScooterSerial.available() && receivePacket(&status))
	{
		display.clearDisplay();
		display.setCursor(0, 0);

		display.setTextSize(1);
		display.println("SPEED");
		display.setTextSize(2);
		uint8_t speed = status.speed / 1000;
		if(status.speed % 1000 >= 500)
			speed++;
		display.println(speed);

		display.setCursor(0, display.getCursorY() + 10);
		display.setTextSize(1);
		display.println("SOC");
		display.setTextSize(2);
		display.println(status.soc);

		display.setTextSize(1);

		if(status.errorCode != 0)
		{
			display.println("ERROR");
			display.setTextSize(2);
			display.println(status.errorCode);	
			display.display();
			return;
		}
		else if(status.shuttingDown)
		{
			display.println("BYE");	
			display.display();
			return;
		}

		display.setCursor(0, display.getCursorY() + 10);
		display.println(status.ecoMode ? "ECO" : "SPORT");

		display.setCursor(0, display.getCursorY() + 3);
		display.println(status.lights ? "NIGHT" : "DAY");

		uint16_t lastX = display.width() - 1;

		display.setCursor(0, display.getCursorY() + 3);
		display.println("gas");
		uint16_t currY = display.getCursorY();
		uint16_t endX = map(lastThrottle, THROTTLE_READ_MIN, THROTTLE_READ_MAX, 0, lastX);
		display.drawLine(0, currY, endX, currY, SSD1306_WHITE);

		display.setCursor(0, display.getCursorY() + 4);
		display.println("brake");
		currY = display.getCursorY();
		endX = map(lastBrake, BRAKE_READ_MIN, BRAKE_READ_MAX, 0, lastX);
		display.drawLine(0, currY, endX, currY, SSD1306_WHITE);

		static int loadingPos = 0;
		uint8_t startY = display.getCursorY() + 10;
		uint8_t endY = display.height() - 1;
		display.drawLine(loadingPos, startY, loadingPos, endY, SSD1306_WHITE);

		loadingPos++;
		if(loadingPos >= display.width())
			loadingPos = 0;

		display.display();
	}
}
