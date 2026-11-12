#include <Arduino.h>
#include <time.h>

#include "time_manager.h"

// POSIX timezone description for Europe/Kyiv: EET (UTC+2) in winter, EEST
// (UTC+3) in summer, switching on the last Sunday of March at 03:00 local
// standard time and back on the last Sunday of October at 04:00 local DST.
static constexpr const char *TIMEZONE = "EET-2EEST,M3.5.0/3,M10.5.0/4";
static constexpr const char *NTP_SERVER_1 = "pool.ntp.org";
static constexpr const char *NTP_SERVER_2 = "time.google.com";

static constexpr uint32_t SYNC_TIMEOUT_MS = 20000;
static constexpr uint32_t RESYNC_INTERVAL_MS = 15u * 60u * 1000u;
static constexpr uint32_t CLOCK_REFRESH_MS = 1000;
static constexpr int MIN_VALID_YEAR = 2020;

void TimeManager::begin() {
    m_state = TimeSyncState::AWAITING_WIFI;
    m_clockValid = false;
    m_clock = {};
    m_attempt = 0;
    Serial.println("Time: NTP sync deferred until WiFi is up");
}

void TimeManager::startSync() {
    m_attempt += 1;
    m_syncStartMs = millis();
    Serial.printf("Time: NTP sync attempt %lu (%s, %s)\n", m_attempt, NTP_SERVER_1, NTP_SERVER_2);
    configTzTime(TIMEZONE, NTP_SERVER_1, NTP_SERVER_2);
}

void TimeManager::refreshClock() {
    const unsigned long now = millis();
    if (m_clockStampMs != 0 && now - m_clockStampMs < CLOCK_REFRESH_MS) {
        return;
    }
    m_clockStampMs = now;

    const time_t raw = time(0);
    struct tm candidate;
    localtime_r(&raw, &candidate);

    if (candidate.tm_year >= MIN_VALID_YEAR - 1900) {
        m_clock = candidate;
        m_clockValid = true;
    } else {
        m_clockValid = false;
    }
}

void TimeManager::update(bool wifiConnected) {
    refreshClock();

    if (!wifiConnected) {
        return;
    }

    if (m_state == TimeSyncState::AWAITING_WIFI) {
        m_state = TimeSyncState::PENDING;
        startSync();
        return;
    }

    if (m_state == TimeSyncState::PENDING) {
        if (m_clockValid) {
            m_state = TimeSyncState::SYNCED;
            m_lastSyncMs = millis();

            char stamp[20];
            snprintf(stamp, sizeof(stamp), "%02u.%02u.%04u %02u:%02u:%02u",
                     static_cast<unsigned>(m_clock.tm_mday) % 100u,
                     static_cast<unsigned>(m_clock.tm_mon + 1) % 100u,
                     static_cast<unsigned>(m_clock.tm_year + 1900) % 10000u,
                     static_cast<unsigned>(m_clock.tm_hour) % 100u,
                     static_cast<unsigned>(m_clock.tm_min) % 100u,
                     static_cast<unsigned>(m_clock.tm_sec) % 100u);
            Serial.printf("Time: NTP sync OK, local time = %s (DST=%d)\n", stamp, m_clock.tm_isdst);
        } else if (millis() - m_syncStartMs >= SYNC_TIMEOUT_MS) {
            Serial.printf("Time: attempt %lu timed out, retrying\n", m_attempt);
            startSync();
        }
        return;
    }

    if (millis() - m_lastSyncMs >= RESYNC_INTERVAL_MS) {
        Serial.println("Time: periodic NTP re-sync");
        startSync();
        m_lastSyncMs = millis();
    }
}
