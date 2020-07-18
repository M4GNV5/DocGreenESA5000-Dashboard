#pragma once

#include <WiFi.h>
#include <esp_system.h>
#include "state.hpp"

bool wifiApEnabled;
bool wifiStaEnabled;
String wifiApSsid;
String wifiApPassword;
String wifiStaSsid;
String wifiStaPassword;

void wifiSetup()
{
	wifiApEnabled = preferences.getUChar(PREFERENCE_AP_ENABLE, 1);
	wifiStaEnabled = preferences.getUChar(PREFERENCE_STA_ENABLE, 0);

	wifiApSsid = preferences.getString(PREFERENCE_AP_SSID, "");
	wifiApPassword = preferences.getString(PREFERENCE_AP_PASSWORD, "");
	wifiStaSsid = preferences.getString(PREFERENCE_STA_SSID, "");
	wifiStaPassword = preferences.getString(PREFERENCE_STA_PASSWORD, "");

	char buff[32];
	if(wifiApSsid == "")
	{
		// no wifi ssid defined, we generate a default one using our MAC

		uint32_t mac = ESP.getEfuseMac() >> 24;
		sprintf(buff, "Scooter %06X", mac);
		wifiApSsid = String(buff);

		preferences.putString(PREFERENCE_AP_SSID, wifiApSsid);
	}
	if(wifiApPassword == "")
	{
		// no wifi password, we enable a dummy station in order to have access
		//  to the RF based esp_random(), which we then use to generate a
		//  random password

		WiFi.mode(WIFI_STA);
		WiFi.disconnect();

		delay(1000);
		uint32_t rnd1 = esp_random();
		delay(100);
		uint32_t rnd2 = esp_random();

		sprintf(buff, "%08X%08X", rnd1, rnd2);

		wifiApPassword = String(buff);

		preferences.putString(PREFERENCE_AP_PASSWORD, wifiApPassword);
	}

	if(wifiStaSsid == "")
		wifiStaEnabled = false;

	if(wifiApEnabled && wifiStaEnabled)
		WiFi.mode(WIFI_AP_STA);
	else if(wifiStaEnabled)
		WiFi.mode(WIFI_STA);
	else if(wifiApEnabled)
		WiFi.mode(WIFI_AP);
	else
		WiFi.mode(WIFI_OFF);

	if(wifiStaEnabled)
	{
		const char *password;
		if(wifiStaPassword == "")
			password = NULL;
		else
			password = wifiStaPassword.c_str();

		WiFi.setHostname("ScooterDashboard");
		WiFi.begin(wifiStaSsid.c_str(), password);
	}
	if(wifiApEnabled)
	{
		WiFi.softAP(wifiApSsid.c_str(), wifiApPassword.c_str());
	}
}
