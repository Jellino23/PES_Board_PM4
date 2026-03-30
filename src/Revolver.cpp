#include "Revolver.h"

Revolver::Revolver(PinName stepPin, PinName dirPin, PinName enPin,
                   PinName sensorVial, PinName sensorHole,
                   int32_t slotSteps)
    : m_stepper(stepPin, dirPin, enPin, 200 * 16)
    , m_sensorVial(sensorVial)
    , m_sensorHole(sensorHole)
    , m_slotSteps(slotSteps)
{
    m_sensorVial.mode(PullUp);
    m_sensorHole.mode(PullUp);
    m_stepper.enable();
}

void Revolver::turnCW()  { m_stepper.setVelocity( SPEED); }
void Revolver::turnCCW() { m_stepper.setVelocity(-SPEED); }
void Revolver::stop()    { m_stepper.setVelocity(0.0f); }

// TCST2103: Phototransistor active LOW when beam is interrupted
bool Revolver::isAtVial() const { return m_sensorVial.read() == 0; }
bool Revolver::isAtHole() const { return m_sensorHole.read() == 0; }

int32_t Revolver::getSteps() const { return m_stepper.getSteps(); }

// --------------------------------------------------------------------------
void Revolver::driveUntil(Callback<bool()> sensorFn, bool cw)
{
    if (cw) turnCW();
    else    turnCCW();
    while (!sensorFn()) { thread_sleep_for(2); }
    stop();
}

void Revolver::stepToNextVial()
{
    driveUntil(callback(this, &Revolver::isAtVial), true);
}

void Revolver::stepToNextHole()
{
    driveUntil(callback(this, &Revolver::isAtHole), true);
}

void Revolver::home()
{
    // Drehe CW bis erste Vial-Lichtschranke, dann Nullpunkt setzen
    turnCW();
    while (!isAtVial()) { thread_sleep_for(2); }
    stop();
}
