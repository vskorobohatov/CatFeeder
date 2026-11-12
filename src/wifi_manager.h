/*
 * WiFiManager - Wi-Fi connection management and automatic reconnection.
 *
 * All operations are non-blocking: begin() only starts the connection
 * handshake, update() polls the connection state once per main-loop
 * iteration and re-arms the attempt when it times out or when the link
 * drops.
 */
#pragma once

#include <WiFi.h>

enum class WiFiConnectionState : uint8_t {
    CONNECTING,
    CONNECTED,
    LOST,
};

class WiFiManager {
public:
    WiFiConnectionState state() const { return m_state; }
    bool connected() const { return m_connected; }

    void begin(const char *ssid, const char *password);
    void update();

private:
    void startAttempt();

    WiFiConnectionState m_state = WiFiConnectionState::CONNECTING;
    bool m_connected = false;
    const char *m_ssid = nullptr;
    const char *m_password = nullptr;
    unsigned long m_attemptStartMs = 0;
    unsigned long m_lastAttemptMs = 0;
    unsigned long m_attempt = 0;
};
