#include <Arduino.h>

#include "motor_manager.h"

/*
 * Mock backend - no motor is attached to the board yet.
 *
 * Swap-in point when the hardware is known:
 *   configurePins()          pin numbers, pinType, attach() for a stepper
 *                            library, rotation direction and speed
 *   startRotation()/stopRotation()   the real actuation
 *   update()                 per-loop tick, needed by stepper libraries
 *   MotorState::NOT_CONFIGURED       keep it until pins are actually wired
 *
 * Nothing outside this file is expected to change.
 */

void MotorManager::begin() {
    configurePins();
    m_state = MotorState::READY;
    Serial.println("Motor: mock backend, no pins configured");
}

void MotorManager::configurePins() {
}

void MotorManager::update() {
    if (m_state != MotorState::READY) {
        Serial.println("Motor: update called before pins are configured");
    }
}

void MotorManager::startRotation() {
    Serial.println("Motor: startRotation (mock)");
}

void MotorManager::stopRotation() {
    Serial.println("Motor: stopRotation (mock)");
}
