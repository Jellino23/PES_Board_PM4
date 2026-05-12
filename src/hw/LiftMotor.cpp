#include "LiftMotor.h"

LiftMotor::LiftMotor(PinName stepPin, PinName dirPin, PinName enPin,
                     PinName sensorLift,
                     PinName magnetPin, float speed)
    : m_stepper(stepPin, dirPin, enPin, 200 * 16)
    , m_sensorLift(sensorLift)
    , m_magnet(magnetPin, 0)
    , m_speed(speed)
    , m_wasBlocked(false)
    , m_triggerCount(0)
{
    m_sensorLift.mode(PullUp);
    m_wasBlocked = (m_sensorLift.read() == 0);
    m_stepper.enable();
    // Sensor alle 5 ms pollen – unabhaengig von Display-SPI-Blockierungen
    m_sensorTicker.attach(callback(this, &LiftMotor::pollSensor),
                          std::chrono::milliseconds{5});
}

void LiftMotor::moveUp()   { m_stepper.setVelocity( m_speed); }
void LiftMotor::moveDown() { m_stepper.setVelocity(-m_speed); }
void LiftMotor::stop()     { m_stepper.setVelocity(0.0f); }

void LiftMotor::grab()    { m_magnet = 1; }
void LiftMotor::release() { m_magnet = 0; }
bool LiftMotor::isGrabbing() { return m_magnet.read() == 1; }

// TCST2103: active LOW wenn Strahl unterbrochen
bool LiftMotor::isAtEnd() { return m_sensorLift.read() == 0; }
bool LiftMotor::isAtTop() { return m_sensorLift.read() == 0; }

void LiftMotor::update() {}  // Sensor-Polling laeuft im Ticker

void LiftMotor::pollSensor()
{
    bool blocked = (m_sensorLift.read() == 0);
    if (blocked && !m_wasBlocked)
        m_triggerCount++;
    m_wasBlocked = blocked;
}

void    LiftMotor::resetTriggerCount()    { m_triggerCount = 0; }
int     LiftMotor::getTriggerCount() const { return m_triggerCount; }

int32_t LiftMotor::getSteps() { return m_stepper.getSteps(); }
