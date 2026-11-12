#include <Arduino.h>
#include <WiFi.h>

#include "wifi_manager.h"

static constexpr uint32_t CONNECT_TIMEOUT_MS = 30000;
static constexpr uint32_t RECONNECT_DELAY_MS = 10000;

void WiFiManager::begin(const char *ssid, const char *password) {
    m_ssid = ssid;
    m_password = password;
    m_connected = false;
    m_state = WiFiConnectionState::CONNECTING;
    startAttempt();
}

void WiFiManager::startAttempt() {
    m_attempt += 1;
    m_attemptStartMs = millis();

    if (m_ssid == nullptr || m_ssid[0] == '\0') {
        Serial.println("WiFi: WIFI_SSID is empty, edit include/wifi_config.h");
    } else {
        Serial.printf("WiFi: connection attempt %lu\n", m_attempt);
    }
    WiFi.begin(m_ssid, m_password);
}

void WiFiManager::update() {
    const bool nowConnected = WiFi.isConnected();

    switch (m_state) {
    case WiFiConnectionState::CONNECTING: {
        if (nowConnected) {
            m_state = WiFiConnectionState::CONNECTED;
            m_connected = true;
            Serial.print("WiFi: connected, assigned IP: ");
            Serial.println(WiFi.localIP());
            return;
        }

        const wl_status_t status = WiFi.status();
        if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL) {
            Serial.printf("WiFi: attempt %lu failed (status %d)\n", m_attempt, status);
            m_state = WiFiConnectionState::LOST;
            m_connected = false;
            m_lastAttemptMs = millis();
            return;
        }

        if (millis() - m_attemptStartMs >= CONNECT_TIMEOUT_MS) {
            Serial.printf("WiFi: attempt %lu timed out after %lu ms\n", m_attempt, CONNECT_TIMEOUT_MS);
            m_state = WiFiConnectionState::LOST;
            m_connected = false;
            m_lastAttemptMs = millis();
        }
        break;
    }

    case WiFiConnectionState::CONNECTED:
        if (!nowConnected) {
            Serial.println("WiFi: connection lost, reconnecting later");
            m_state = WiFiConnectionState::LOST;
            m_connected = false;
            m_lastAttemptMs = millis();
        }
        break;

    case WiFiConnectionState::LOST:
        if (millis() - m_lastAttemptMs >= RECONNECT_DELAY_MS) {
            Serial.println("WiFi: reconnect attempt scheduled");
            m_state = WiFiConnectionState::CONNECTING;
            startAttempt();
        }
        break;
    }
}
