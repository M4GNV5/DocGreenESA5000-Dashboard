#pragma once

#include "state.hpp"
#include "protocol.h"

bool reenableLightsAfterError = false;
static uint8_t reenableLightTimeout = 0;
static bool lightShouldBeOn = false;

void internalSetLight(bool shouldBeOn)
{
    reenableLightTimeout = 20;
    lightShouldBeOn = shouldBeOn;

    setLight(shouldBeOn);
    setLight(shouldBeOn);
    setLight(shouldBeOn);
}

void reenableLightLoop(docgreen_status_t& scooterStatus)
{
    if(!reenableLightsAfterError)
        return;	

    if(reenableLightTimeout > 0)
        reenableLightTimeout--;
    if(scooterStatus.buttonPress)
        reenableLightTimeout = 20;

    if(lightShouldBeOn && !scooterStatus.lights && reenableLightTimeout == 0)
    {
        setLight(true);
    }
    else if(scooterStatus.lights != lightShouldBeOn)
    {
        lightShouldBeOn = scooterStatus.lights;
    }
}