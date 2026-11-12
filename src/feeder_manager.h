/*
 * FeederManager - time-of-day feeding schedule and rotation state machine.
 *
 * Pure policy: it decides when a meal is due and how long the motor should
 * run, then drives the motor through MotorManager. It owns no hardware, so
 * the schedule logic does not move when the motor is swapped.
 *
 * update() never blocks: a due slot starts the rotation and the duration is
 * measured with millis() across later loop passes.
 */
#pragma once

#include "motor_manager.h"
#include <time.h>

enum class FeedingState : uint8_t {
    IDLE,
    ROTATING,
};

// Upper bound for the SCHEDULE table in feeder_manager.cpp; keeps the fixed
// per-slot bookkeeping arrays free of heap and in step with the table.
static constexpr uint8_t MAX_FEED_SLOTS = 3;

class FeederManager {
public:
    void begin(MotorManager &motor);
    void update(bool timeValid, const struct tm &clock);

    FeedingState state() const { return m_state; }
    bool feeding() const { return m_state == FeedingState::ROTATING; }

private:
    void primeSchedule(int minuteOfDay);
    void resetFiredFlags();
    void fireSlot(uint8_t slot);

    MotorManager *m_motor = nullptr;
    FeedingState m_state = FeedingState::IDLE;
    bool m_fired[MAX_FEED_SLOTS] = {};
    bool m_primed = false;
    int m_fireYear = -1;
    int m_fireDay = -1;
    unsigned long m_rotationStartMs = 0;
};
