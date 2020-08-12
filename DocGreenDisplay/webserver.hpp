#pragma once

#include <WebServer.h>
#include <ESPmDNS.h>

#include "state.hpp"
#include "protocol.h"

#include "webinterface/bundle.hpp"

static WebServer server;

static void handleIndex()
{
	server.sendHeader("Content-Encoding", "gzip");
	server.send_P(200, PSTR("text/html"), (PGM_P)webpageData, sizeof(webpageData) / sizeof(*webpageData));
}

static void handleData()
{
	String data = String("{") +
		"\"throttle\": " + scooterStatus.throttle +
		", \"brake\": " + scooterStatus.brake +
		", \"ecoMode\": " + scooterStatus.ecoMode +
		", \"shuttingDown\": " + scooterStatus.shuttingDown +
		", \"lights\": " + scooterStatus.lights +
		", \"buttonPress\": " + scooterStatus.buttonPress +
		", \"errorCode\": " + scooterStatus.errorCode +
		", \"soc\": " + scooterStatus.soc +
		", \"speed\": " + scooterStatus.speed +
		", \"totalOperationTime\": " + scooterStatus.totalOperationTime +
		", \"timeSinceBoot\": " + scooterStatus.timeSinceBoot +
		", \"voltage\": " + scooterStatus.voltage +
		", \"current\": " + scooterStatus.current +
		", \"mainboardVersion\": " + scooterStatus.mainboardVersion +
		", \"odometer\": " + scooterStatus.odometer +
		", \"isLocked\": " + isLocked +
		", \"updateStatus\": \"" + firmwareUpdateStatus + "\"" +
	"}";

	server.send(200, "application/json", data);
}

static void handleConfig()
{
	String data = String("{") +
		"\"" PREFERENCE_MAX_SPEED "\": " + configuredSpeed +
		", \"" PREFERENCE_SHOW_INTRO "\": " + preferences.getUChar(PREFERENCE_SHOW_INTRO, 1) +
		", \"" PREFERENCE_REENABLE_LIGHT "\": " + reenableLightsAfterError +
		", \"" PREFERENCE_LOCK_ON_BOOT "\": " + preferences.getUChar(PREFERENCE_LOCK_ON_BOOT, 0) +
		", \"" PREFERENCE_LOCK_PIN "\": \"" + scooterPin + "\"" +
		", \"" PREFERENCE_AP_ENABLE "\": " + preferences.getUChar(PREFERENCE_AP_ENABLE, 1) +
		", \"" PREFERENCE_AP_SSID "\": \"" + wifiApSsid + "\"" +
		", \"" PREFERENCE_AP_PASSWORD "\": \"" + wifiApPassword + "\"" +
		", \"" PREFERENCE_STA_ENABLE "\": " + preferences.getUChar(PREFERENCE_STA_ENABLE, 0) +
		", \"" PREFERENCE_STA_SSID "\": \"" + wifiStaSsid + "\"" +
		", \"" PREFERENCE_STA_PASSWORD "\": \"" + wifiStaPassword + "\"" +
		", \"" PREFERENCE_UPDATE_AUTO "\": \"" + preferences.getUChar(PREFERENCE_UPDATE_AUTO, 1) + "\"" +
		", \"" PREFERENCE_UPDATE_URL "\": \"" + firmwareUpdateUrl + "\"" +
	"}";

	server.send(200, "application/json", data);
}

static bool updateBoolPreference(const char *name, bool oldVal)
{
	if(!server.hasArg(name))
		return oldVal;

	bool val = server.arg(name) == "true";
	if(val != oldVal)
		preferences.putUChar(name, val);

	return val;
}
static void updateStringPreference(const char *name, String *valPtr)
{
	if(!server.hasArg(name))
		return;

	String val = server.arg(name);
	if(val != *valPtr)
	{
		preferences.putString(name, val);
		*valPtr = val;
	}
}
static void handleUpdateConfig()
{
	String maxSpeedStr = server.arg(PREFERENCE_MAX_SPEED);
	uint32_t maxSpeed = atoi(maxSpeedStr.c_str());
	if(maxSpeed >= 5 && maxSpeed <= 35 && maxSpeed != configuredSpeed)
	{
		setMaxSpeed(maxSpeed);

		configuredSpeed = maxSpeed;
		preferences.putUChar(PREFERENCE_MAX_SPEED, maxSpeed);
	}
	else if(maxSpeed != configuredSpeed)
	{
		server.send(400, "text/plain", "invalid speed");
		return;
	}

	updateBoolPreference(PREFERENCE_SHOW_INTRO, preferences.getUChar(PREFERENCE_SHOW_INTRO, 1));
	updateBoolPreference(PREFERENCE_LOCK_ON_BOOT, preferences.getUChar(PREFERENCE_LOCK_ON_BOOT, 0));
	updateBoolPreference(PREFERENCE_AP_ENABLE, preferences.getUChar(PREFERENCE_AP_ENABLE, 1));
	updateBoolPreference(PREFERENCE_STA_ENABLE, preferences.getUChar(PREFERENCE_STA_ENABLE, 0));
	updateBoolPreference(PREFERENCE_UPDATE_AUTO, preferences.getUChar(PREFERENCE_UPDATE_AUTO, 1));
	reenableLightsAfterError = updateBoolPreference(PREFERENCE_REENABLE_LIGHT, reenableLightsAfterError);

	updateStringPreference(PREFERENCE_LOCK_PIN, &scooterPin);
	updateStringPreference(PREFERENCE_AP_SSID, &wifiApSsid);
	updateStringPreference(PREFERENCE_AP_PASSWORD, &wifiApPassword);
	updateStringPreference(PREFERENCE_STA_SSID, &wifiStaSsid);
	updateStringPreference(PREFERENCE_STA_PASSWORD, &wifiStaPassword);
	updateStringPreference(PREFERENCE_UPDATE_URL, &firmwareUpdateUrl);

	server.send(200, "text/plain", "ok");
}

static void handleAction()
{
	String action = server.pathArg(0);
	bool enabled = server.pathArg(1) == "true";

	if(action == "setEcoMode")
	{
		setEcoMode(enabled);
	}
	else if(action == "setLock")
	{
		isLocked = enabled;
		setLock(enabled);
		delay(50);
		setLock(enabled);
		delay(50);
		setLock(enabled);
	}
	else if(action == "setLight")
	{
		setLight(enabled);
	}

	server.send(200, "text/plain", "ok");
}

void webServerSetup()
{
	server.on("/", handleIndex);
	server.on("/data", handleData);
	server.on("/config", handleConfig);
	server.on("/updateConfig", handleUpdateConfig);
	server.on("/action/{}/{}", handleAction);

	if(wifiApEnabled || wifiStaEnabled)
		server.begin();
}

void webServerLoop()
{
	if(wifiApEnabled || wifiStaEnabled)
		server.handleClient();
}