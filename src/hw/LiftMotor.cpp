#include "LiftMotor.h"

LiftMotor::LiftMotor(PinName stepPin, PinName dirPin, PinName enPin,
                     PinName sensorLift,
                     PinName magnetPin, float speed)
    : m_stepper(stepPin, dirPin, enPin, 200 * 16)
    , m_sensorLift(sensorLift)
    , m_magnet(magnetPin, 0)
    , m_speed(speed)
{
    m_sensorLift.mode(PullUp);
    m_stepper.enable();
}

void LiftMotor::moveUp()   { m_stepper.setVelocity( m_speed); }
void LiftMotor::moveDown() { m_stepper.setVelocity(-m_speed); }
void LiftMotor::stop()     { m_stepper.setVelocity(0.0f); }

void LiftMotor::grab()    { m_magnet = 1; }
void LiftMotor::release() { m_magnet = 0; }
bool LiftMotor::isGrabbing() { return m_magnet.read() == 1; }

// TCST2103: Phototransistor leitet wenn Strahl unterbrochen → active LOW
bool LiftMotor::isAtEnd()    { return m_sensorLift.read()    == 0; }


int32_t LiftMotor::getSteps() { return m_stepper.getSteps(); }
