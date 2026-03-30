#include "LiftMotor.h"

LiftMotor::LiftMotor(PinName stepPin, PinName dirPin, PinName enPin,
                     PinName sensorTop, PinName sensorBottom,
                     PinName magnetPin)
    : m_stepper(stepPin, dirPin, enPin, 200 * 16)
    , m_sensorTop(sensorTop)
    , m_sensorBottom(sensorBottom)
    , m_magnet(magnetPin, 0)
{
    m_sensorTop.mode(PullUp);
    m_sensorBottom.mode(PullUp);
    m_stepper.enable();
}

void LiftMotor::moveUp()
{
    m_stepper.setVelocity(SPEED);   // positive = CW = up (adjust sign if needed)
}

void LiftMotor::moveDown()
{
    m_stepper.setVelocity(-SPEED);
}

void LiftMotor::stop()
{
    m_stepper.setVelocity(0.0f);
}

void LiftMotor::grab()    { m_magnet = 1; }
void LiftMotor::release() { m_magnet = 0; }
bool LiftMotor::isGrabbing() const { return m_magnet.read() == 1; }

// TCST2103 – Phototransistor active LOW when beam is interrupted
bool LiftMotor::isAtTop()    const { return m_sensorTop.read()    == 0; }
bool LiftMotor::isAtBottom() const { return m_sensorBottom.read() == 0; }

void LiftMotor::home()
{
    // Drive up until top sensor triggers, then reset step counter
    moveUp();
    while (!isAtTop()) { thread_sleep_for(5); }
    stop();
    // Step position zero = top
}

void LiftMotor::moveTo(int32_t steps, float velocity)
{
    m_stepper.setSteps(steps, velocity);
}

int32_t LiftMotor::getSteps() const
{
    return m_stepper.getSteps();
}
