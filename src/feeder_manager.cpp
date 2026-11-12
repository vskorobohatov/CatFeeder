#include <Arduino.h>
#include <time.h>

#include "feeder_manager.h"

// Meal times in local time. Local time is deliberate: TimeManager already
// applies the Europe/Kyiv DST rules, so a schedule expressed in minutes
// since midnight keeps working through both DST switches.
struct FeedSlot {
    uint8_t hour;
    uint8_t minute;
};

static constexpr const FeedSlot SCHEDULE[] = {{6, 30}, {15, 0}, {22, 0}};
static constexpr uint8_t SLOT_COUNT = sizeof(SCHEDULE) / sizeof(SCHEDULE[0]);
static constexpr uint32_t ROTATION_MS = 5000;

static_assert(SLOT_COUNT <= MAX_FEED_SLOTS, "SCHEDULE is longer than MAX_FEED_SLOTS");

static int slotMinute(const FeedSlot &slot) {
    return slot.hour * 60 + slot.minute;
}

void FeederManager::begin(MotorManager &motor) {
    m_motor = &motor;
    m_state = FeedingState::IDLE;
    m_primed = false;
    m_fireYear = -1;
    m_fireDay = -1;
    resetFiredFlags();
    Serial.printf("Feeder: %u slots, rotation %lu ms\n", SLOT_COUNT, ROTATION_MS);
}

void FeederManager::resetFiredFlags() {
    for (uint8_t slot = 0; slot < MAX_FEED_SLOTS; ++slot) {
        m_fired[slot] = false;
    }
}

// Slots already in the past on the very first valid clock are marked done
// without feeding: booting at 15:45 must not dump food for the 06:30 slot.
void FeederManager::primeSchedule(int minuteOfDay) {
    for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
        if (minuteOfDay >= slotMinute(SCHEDULE[slot])) {
            m_fired[slot] = true;
        }
    }
}

void FeederManager::fireSlot(uint8_t slot) {
    m_fired[slot] = true;
    m_motor->startRotation();
    m_rotationStartMs = millis();
    m_state = FeedingState::ROTATING;
}

void FeederManager::update(bool timeValid, const struct tm &clock) {
    if (!timeValid || m_motor == nullptr) {
        return;
    }

    if (clock.tm_year != m_fireYear || clock.tm_yday != m_fireDay) {
        m_fireYear = clock.tm_year;
        m_fireDay = clock.tm_yday;
        resetFiredFlags();
        m_primed = false;
    }

    const int minuteOfDay = clock.tm_hour * 60 + clock.tm_min;

    if (!m_primed) {
        m_primed = true;
        primeSchedule(minuteOfDay);
        return;
    }

    switch (m_state) {
    case FeedingState::IDLE: {
        for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
            const int due = slotMinute(SCHEDULE[slot]);
            if (minuteOfDay < due || m_fired[slot]) {
                continue;
            }

            // A clock that jumped forward (DST, or an NTP correction) still
            // gets the meal, late but logged - a missed meal is worse for the
            // cat than a late one.
            if (minuteOfDay > due) {
                Serial.printf("Feeder: slot %u fired %d min late\n", slot, minuteOfDay - due);
            } else {
                Serial.printf("Feeder: slot %u due at %02u:%02u\n", slot,
                              SCHEDULE[slot].hour, SCHEDULE[slot].minute);
            }
            fireSlot(slot);
            return;
        }
        break;
    }

    case FeedingState::ROTATING:
        if (millis() - m_rotationStartMs >= ROTATION_MS) {
            m_motor->stopRotation();
            m_state = FeedingState::IDLE;
        }
        break;
    }
}
