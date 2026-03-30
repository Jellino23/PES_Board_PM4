#include "Revolver.h"

Revolver::Revolver(PinName stepPin, PinName dirPin, PinName enPin,
                   PinName sensorVial, PinName sensorHole, float speed)
    : m_stepper(stepPin, dirPin, enPin, 200 * 16)
    , m_sensorVial(sensorVial)
    , m_sensorHole(sensorHole)
    , m_speed(speed)
{
    m_sensorVial.mode(PullUp);
    m_sensorHole.mode(PullUp);
    m_stepper.enable();
}

void Revolver::turnCW()  { m_stepper.setVelocity( m_speed); }
void Revolver::turnCCW() { m_stepper.setVelocity(-m_speed); }
void Revolver::stop()    { m_stepper.setVelocity(0.0f); }

// TCST2103: active LOW wenn Strahl unterbrochen
bool Revolver::isAtVial() { return m_sensorVial.read() == 0; }
bool Revolver::isAtHole() { return m_sensorHole.read() == 0; }

int32_t Revolver::getSteps() { return m_stepper.getSteps(); }
