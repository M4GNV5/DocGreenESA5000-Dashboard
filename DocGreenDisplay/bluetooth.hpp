#include <stdlib.h>
#include <stdint.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include "state.hpp"
#include "protocol.h"

// Instead of implementing a custom protocol we emulate the orginal
// M365 Bluetooth Protocol, hopefully this allows users to use one of the
// many M365 apps available with this dashboard.
// For an overview of the protocol see:
//      https://github.com/etransport/ninebot-docs/wiki/protocol
//      https://github.com/etransport/ninebot-docs/wiki/M365ESC
//      https://github.com/etransport/py9b

#define M365_REG_MAGIC 0x00
#define M365_REG_PIN 0x17
#define M365_REG_ERROR 0x1B
#define M365_REG_SOC 0x22
#define M365_REG_RANGE_CONSERVATIVE 0x24
#define M365_REG_RANGE 0x25
#define M365_REG_SPEED1 0x26
#define M365_REG_ODOMETER 0x29
#define M365_REG_TRIP_ODOMETER 0x2F
#define M365_REG_OPERATION_TIME 0x32
#define M365_REG_OPERATION_TIME2 0x34
#define M365_REG_TRIP_TIME 0x3A
#define M365_REG_TRIP_TIME2 0x3B
#define M365_REG_BATTERY_VOLTAGE 0x48
#define M365_REG_AVERAGE_SPEED 0x65
#define M365_REG_LOCK_COMMAND 0x70
#define M365_REG_UNLOCK_COMMAND 0x71
#define M365_REG_ECO_MODE 0x75
#define M365_REG_LIGHTS 0x7D
#define M365_REG_ERROR2 0xB0
#define M365_REG_SOC2 0xB4
#define M365_REG_SPEED2 0xB5
#define M365_REG_AVERAGE_SPEED2 0xB6
#define M365_REG_ODOMETER2 0xB7
#define M365_REG_TRIP_ODOMETER2 0xB9

bool bluetoothEnabled = false;
bool bluetoothControlEnabled = false;
uint16_t m365Registers[0xB9 + 2];
uint32_t tripStartOdometer = 0;
uint32_t averageSpeedCount = 0;
int32_t averageSpeed = 0;

#define BLE_M365_SERVICE_UUID "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define BLE_M365_RX_UUID "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define BLE_M365_TX_UUID "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

BLEServer *pServer = NULL;
BLECharacteristic *rxCharacteristic = NULL;
BLECharacteristic *txCharacteristic = NULL;

void setReg2(uint8_t offset, uint16_t val)
{
	m365Registers[offset] = val;
}
void setReg4(uint8_t offset, uint32_t val)
{
	*(uint32_t *)&m365Registers[offset] = val;
}

class M365BleCallbacks: public BLECharacteristicCallbacks
{
	void onWrite(BLECharacteristic *characteristic)
	{
		std::string value = characteristic->getValue();
		uint8_t *buff = (uint8_t *)value.c_str();
		uint8_t len = value.length();

		if(len < 6 || len != buff[2] + 6)
			return;

		if(buff[0] != 0x55 || buff[1] != 0xaa)
			return;

		//uint16_t actualChecksum = *(uint16_t *)&buff[len + 4];
		//if(calculateChecksum(buff + 2) != actualChecksum)
		//	return;

#if 0
		for(int i = 0; i < len; i++)
		{
			if(buff[i] < 16)
				Serial.print('0');

			Serial.print(buff[i], 16);
			Serial.print(' ');
		}
		Serial.println();
#endif

		uint8_t cmd = buff[4];
		uint8_t offset = buff[5];

		uint8_t responseLen = cmd == 0x01 ? buff[6] : 1;
		uint8_t response[8 + responseLen];
		response[0] = 0x55;
		response[1] = 0xAA;
		response[2] = responseLen + 2; // packet length
		response[3] = buff[3] + 3; // address
		response[4] = cmd; // cmd
		response[5] = offset; // arg

		if(cmd == 0x01) // read register
		{
			if(offset + responseLen >= sizeof(m365Registers))
				return;  // TODO send an error back?

			memcpy(response + 6, &m365Registers[offset], responseLen);
		}
		else if(cmd == 0x02 || cmd == 0x03) // write register
		{
			response[6] = 1; // write success

			if(!bluetoothControlEnabled)
			{
				response[6] = 0; // write disabled
			}
			else if(offset == M365_REG_LOCK_COMMAND)
			{
				isLocked = true;
				setLock(true);
				setLock(true);
				setLock(true);
			}
			else if(offset == M365_REG_UNLOCK_COMMAND)
			{
				isLocked = false;
				setLock(false);
				setLock(false);
				setLock(false);
			}
			else if(offset == M365_REG_ECO_MODE)
			{
				bool enabled = (buff[6] == 0 && buff[7] == 0) ? false : true;
				setEcoMode(enabled);
				setEcoMode(enabled);
				setEcoMode(enabled);
			}
			else if(offset == M365_REG_LIGHTS)
			{
				bool enabled = (buff[6] == 0 && buff[7] == 0) ? false : true;
				internalSetLight(enabled);
			}
			else
			{
				response[6] = 0; // write failed
			}

			if(cmd == 0x03) // write without response
				return;
		}

		*(uint16_t *)(response + responseLen + 6) = calculateChecksum(response + 2);

		txCharacteristic->setValue(response, 8 + responseLen);
		txCharacteristic->notify();
	}
};

void bluetoothSetup()
{
	bluetoothEnabled = preferences.getUChar(PREFERENCE_BLE_ENABLE, 0);
	bluetoothControlEnabled = preferences.getUChar(PREFERENCE_BLE_CONTROL_ENABLE, 0);
	if(!bluetoothEnabled)
		return;

#if 0
	Serial.begin(115200);
#endif

	// TODO: should we generate our own name?
	BLEDevice::init(wifiApSsid.c_str());

	pServer = BLEDevice::createServer();
	BLEService *pService = pServer->createService(BLE_M365_SERVICE_UUID);
	rxCharacteristic = pService->createCharacteristic(BLE_M365_RX_UUID,
		BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
	txCharacteristic = pService->createCharacteristic(BLE_M365_TX_UUID,
		BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

	txCharacteristic->addDescriptor(new BLE2902());
	rxCharacteristic->setCallbacks(new M365BleCallbacks());

	pService->start();

	BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(BLE_M365_SERVICE_UUID);
	pAdvertising->setScanResponse(true);
	pAdvertising->setMinPreferred(0x06);
	BLEDevice::startAdvertising();

	setReg2(M365_REG_MAGIC, 0x515C);
	memset(&m365Registers[M365_REG_PIN], '0', 6);
}

void bluetoothLoop(docgreen_status_t& status)
{
	if(!bluetoothEnabled)
		return;

	if(tripStartOdometer == 0)
		tripStartOdometer = status.odometer;
	setReg2(M365_REG_TRIP_ODOMETER, status.odometer - tripStartOdometer);
	setReg2(M365_REG_TRIP_ODOMETER2, status.odometer - tripStartOdometer);

	if(status.speed > 100)
	{
		averageSpeedCount++;
		int32_t diff = (int32_t)status.speed * 1000 - averageSpeed;
		diff /= averageSpeedCount;
		averageSpeed += diff;

		setReg2(M365_REG_AVERAGE_SPEED, averageSpeed);
		setReg2(M365_REG_AVERAGE_SPEED2, averageSpeed);
	}

	setReg2(M365_REG_RANGE, 20 * (uint16_t)status.soc); // 20km = 20000m at 100% => 200m/%
	setReg2(M365_REG_RANGE_CONSERVATIVE, 16 * (uint16_t)status.soc); // 16km = 16000m at 100% => 160m/%

	setReg2(M365_REG_ERROR, status.errorCode);
	setReg2(M365_REG_ERROR2, status.errorCode);
	setReg2(M365_REG_SOC, status.soc);
	setReg2(M365_REG_SOC2, status.soc);
	setReg2(M365_REG_SPEED1, status.speed);
	setReg2(M365_REG_SPEED2, status.speed);
	setReg4(M365_REG_ODOMETER, status.odometer);
	setReg4(M365_REG_ODOMETER2, status.odometer);
	setReg4(M365_REG_OPERATION_TIME, status.totalOperationTime);
	setReg4(M365_REG_OPERATION_TIME2, status.totalOperationTime);
	setReg2(M365_REG_TRIP_TIME, millis() / 1000);
	setReg2(M365_REG_TRIP_TIME2, millis() / 1000);
	setReg2(M365_REG_BATTERY_VOLTAGE, status.voltage);
	setReg2(M365_REG_ECO_MODE, status.ecoMode);
	setReg2(M365_REG_LIGHTS, status.lights);
}
