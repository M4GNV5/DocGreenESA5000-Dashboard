#pragma once

#include <WebServer.h>
#include "state.hpp"
#include "protocol.h"

#include "webinterface/bundle.hpp"

static WebServer server;

static void handleIndex()
{
	server.send(200, "text/html", webpageData);
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
		", \"temperature\": " + scooterStatus.temperature +
		", \"isLocked\": " + isLocked +
	"}";

	server.send(200, "application/json", data);
}

static void handleConfig()
{
	String data = String("{") +
		"\"" PREFERENCE_MAX_SPEED "\": " + configuredSpeed +
		", \"" PREFERENCE_SHOW_INTRO "\": " + preferences.getUChar(PREFERENCE_SHOW_INTRO, 1) +
		", \"" PREFERENCE_REENABLE_LIGHT "\": " + reenableLightsAfterError +
		", \"" PREFERENCE_LOCK_ON_BOOT "\": " + preferences.getUChar(PREFERENCE_LOCK_ON_BOOT, 1) +
		", \"" PREFERENCE_LOCK_PIN "\": \"" + scooterPin + "\"" +
		", \"" PREFERENCE_AP_SSID "\": \"" + preferences.getString(PREFERENCE_AP_SSID, "Scooter Dashboard") + "\"" +
		", \"" PREFERENCE_AP_PASSWORD "\": \"" + preferences.getString(PREFERENCE_AP_PASSWORD, "FossScootersAreCool") + "\"" +
		", \"" PREFERENCE_STA_SSID "\": \"" + preferences.getString(PREFERENCE_STA_SSID) + "\"" +
		", \"" PREFERENCE_STA_PASSWORD "\": \"" + preferences.getString(PREFERENCE_STA_PASSWORD) + "\"" +
	"}";

	server.send(200, "application/json", data);
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
	}
	else if(action == "setLight")
	{
		setLight(enabled);
	}

	server.send(200, "text/plain", "ok");
}

void setupWebServer()
{
	String ssid = preferences.getString(PREFERENCE_AP_SSID, "Scooter Dashboard");
	String pw = preferences.getString(PREFERENCE_AP_PASSWORD, "FossScootersAreCool");
	WiFi.softAP(ssid.c_str(), pw.c_str());

	server.on("/", handleIndex);
	server.on("/data", handleData);
	server.on("/config", handleConfig);
	server.on("/action/{}/{}", handleAction);

	server.begin();
}

void handleWebserver()
{
	server.handleClient();
}