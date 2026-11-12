/*
 * MotorManager - the single seam to the step-motor hardware.
 *
 * Today this is a mock: startRotation() and stopRotation() do nothing but
 * log, and no pin is configured. When the motor and its controller are known,
 * only this file changes - configurePins() gets the pin numbers and
 * startRotation()/stopRotation() get the real actuation. FeederManager talks
 * to the motor through this interface and never sees pins or libraries, so
 * the feeding schedule stays untouched across a hardware swap.
 *
 * update() is a per-loop tick hook that the mock leaves empty; stepper-style
 * motor libraries need one, and having it here now keeps main.cpp unchanged
 * when the real motor lands.
 */
#pragma once

enum class MotorState : uint8_t {
    NOT_CONFIGURED,
    READY,
};

class MotorManager {
public:
    void begin();
    void update();
    void startRotation();
    void stopRotation();

    MotorState state() const { return m_state; }

private:
    void configurePins();

    MotorState m_state = MotorState::NOT_CONFIGURED;
};
