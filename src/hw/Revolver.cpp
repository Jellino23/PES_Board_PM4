#include "Revolver.h"

Revolver::Revolver(PinName stepPin, PinName dirPin, PinName enPin,
                   PinName sensorPin, float speed)
    : m_stepper(stepPin, dirPin, enPin, 200 * 16)
    , m_sensor(sensorPin)
    , m_wasBlocked(false)
    , m_triggerCount(0)
    , m_speed(speed)
{
    m_sensor.mode(PullUp);
    m_wasBlocked = (m_sensor.read() == 1);  // Startposition merken, kein Falschtrigger
    m_stepper.enable();
}

void Revolver::update()
{
    bool blocked = m_sensor.read() == 1;
    if (blocked && !m_wasBlocked)
        m_triggerCount++;
    m_wasBlocked = blocked;
}

void Revolver::turnCW()  { m_stepper.setVelocity(-m_speed); }
void Revolver::turnCCW() { m_stepper.setVelocity(m_speed); }
void Revolver::stop()    { m_stepper.setVelocity(0.0f); }

void Revolver::moveSteps(int32_t steps) { m_stepper.setStepsRelative(steps, m_speed); }
bool Revolver::isMoving() const         { return m_stepper.isMoving(); }

// TCST2103 (+ = Collector an PC_5, D = Emitter an GND): active HIGH wenn Strahl unterbrochen
bool Revolver::isAtVial() { return m_sensor.read() == 1; }
bool Revolver::isAtHole() { return m_sensor.read() == 1; }

void    Revolver::resetTriggerCount()    { m_triggerCount = 0; }
int     Revolver::getTriggerCount() const { return m_triggerCount; }

int32_t Revolver::getSteps() { return m_stepper.getSteps(); }
