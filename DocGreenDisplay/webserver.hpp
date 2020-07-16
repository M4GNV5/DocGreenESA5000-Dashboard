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
	String data = "{"
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
	String data = "{"
		", \"" PREFERENCE_MAX_SPEED "\": " + configuredSpeed +
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
	String arg = server.pathArg(1);

	if(action == "setMaxSpeed")
	{
		int speed = atoi(arg);
		if(speed < 5 || speed > 35)
		{
			server.send(400, "text/plain", "invalid speed");
			return;
		}

		setMaxSpeed(speed);
	}
	else if(action == "setEcoMode")
	{
		bool enabled = arg == "true";
		setEcoMode(enabled);
	}
	else if(action == "setLock")
	{
		bool enabled = arg == "true";
		isLocked = enabled;
		setLock(enabled);
	}
	else if(action == "setLight")
	{
		bool enabled = arg == "true";
		setLight(enabled);
	}

	server.send(200, "text/plain", "ok");
}

void setupWebServer()
{
	String ssid = preferences.getString(PREFERENCE_AP_SSID, "Scooter Dashboard");
	String pw = preferences.getString(PREFERENCE_AP_PASSWORD, "FossScootersAreCool");
	WiFi.softAP(ssid, pw);

	server.on("/", handleIndex);
	server.on("/data", handleData);
	server.on("/data", handleConfig);
	server.on("/action/{}/{}", handleData);

	server.begin();
}

void handleWebserver()
{
	server.handleClient();
}