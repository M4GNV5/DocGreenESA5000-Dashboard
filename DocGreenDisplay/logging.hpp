#pragma once

#include <stdint.h>

#include "state.hpp"

typedef struct
{
	uint32_t odometerDiff : 10; // m since last log entry (max = 1024 -> 1.024km)
	uint32_t speed : 9; // speed in 0.1 km/h (max = 512 -> 51.2km/h)
	uint32_t voltage : 9; // voltage in 0.1 V (max = 512 -> 51.2V)
	uint32_t lights : 1;
	uint32_t ecoMode : 1;
	uint32_t locked : 1;
} scooter_logentry_t;

static_assert(sizeof(scooter_logentry_t) == 4, "Logentry size does not fit.");

static scooter_logentry_t logEntries[1024];
static uint16_t logPos = 0;
static uint32_t odometerAtStart = 0;

void loggingLoop()
{
	static uint32_t lastOdometer = 0;
	static uint32_t lastEntry = 0;

	if(lastEntry + 30 * 1000 > millis())
		return;
	lastEntry = millis();

	scooter_logentry_t *entry = &logEntries[logPos];

	if(logPos == 0)
		entry->odometerDiff = 0;
	else
		entry->odometerDiff = scooterStatus.odometer - lastOdometer;
	lastOdometer = scooterStatus.odometer;

	entry->speed = scooterStatus.speed / 100;
	entry->voltage = scooterStatus.voltage;
	entry->lights = scooterStatus.lights;
	entry->ecoMode = scooterStatus.ecoMode;
	entry->locked = isLocked;

	logPos++;
}
