/*
 * TimeManager - NTP time synchronisation and current local time.
 *
 * Synchronisation uses the ESP32 core helper configTzTime(), which arms
 * lwIP's SNTP client (UDP, non-blocking) and installs the Europe/Kyiv
 * timezone rules, including the automatic switch between EET and EEST.
 * update() never blocks: it only reads the system clock and re-arms the
 * client when an attempt timed out or when a periodic re-sync is due.
 */
#pragma once

#include <time.h>

enum class TimeSyncState : uint8_t {
    AWAITING_WIFI,
    PENDING,
    SYNCED,
};

class TimeManager {
public:
    void begin();
    void update(bool wifiConnected);

    bool hasValidTime() const { return m_clockValid; }
    const struct tm &localTime() const { return m_clock; }

private:
    void startSync();
    void refreshClock();

    TimeSyncState m_state = TimeSyncState::AWAITING_WIFI;
    struct tm m_clock = {};
    bool m_clockValid = false;
    unsigned long m_clockStampMs = 0;
    unsigned long m_syncStartMs = 0;
    unsigned long m_lastSyncMs = 0;
    unsigned long m_attempt = 0;
};
