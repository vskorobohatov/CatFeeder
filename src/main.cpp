
#include <Arduino.h>
#include <WiFi.h>

#include "wifi_manager.h"
#include "time_manager.h"
#include "display_manager.h"
#include "motor_manager.h"
#include "feeder_manager.h"

// Wi-Fi credentials live in include/wifi_config.h, which is kept out of the
// repository (see .gitignore). include/wifi_config.example.h documents it.
#if defined(__has_include)
#  if __has_include("wifi_config.h")
#    include "wifi_config.h"
#  else
#    error "Missing include/wifi_config.h: copy include/wifi_config.example.h to include/wifi_config.h and define WIFI_SSID and WIFI_PASSWORD."
#  endif
#else
#  include "wifi_config.h"
#endif

#ifndef WIFI_SSID
#  error "WIFI_SSID is not defined in include/wifi_config.h"
#endif
#ifndef WIFI_PASSWORD
#  error "WIFI_PASSWORD is not defined in include/wifi_config.h"
#endif

WiFiManager wifiManager;
TimeManager timeManager;
DisplayManager displayManager;
MotorManager motor;
FeederManager feeder;

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32-C3 CatFeeder: startup");

    displayManager.begin();
    wifiManager.begin(WIFI_SSID, WIFI_PASSWORD);
    timeManager.begin();
    motor.begin();
    feeder.begin(motor);

    Serial.println("ESP32-C3 CatFeeder: entering main loop");
}

void loop() {
    wifiManager.update();
    timeManager.update(wifiManager.connected());
    feeder.update(timeManager.hasValidTime(), timeManager.localTime());
    motor.update();

    switch (wifiManager.state()) {
    case WiFiConnectionState::CONNECTING:
        displayManager.showStatus("WiFi...");
        break;

    case WiFiConnectionState::LOST:
        displayManager.showStatus("No WiFi");
        break;

    case WiFiConnectionState::CONNECTED:
        if (timeManager.hasValidTime()) {
            if (feeder.feeding()) {
                displayManager.showStatus("Feeding");
            } else {
                displayManager.showClock(timeManager.localTime());
            }
        } else {
            displayManager.showStatus("Sync time...");
        }
        break;
    }
}

