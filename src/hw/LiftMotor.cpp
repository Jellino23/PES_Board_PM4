#include "LiftMotor.h"

LiftMotor::LiftMotor(PinName stepPin, PinName dirPin, PinName enPin,
                     PinName sensorLift,
                     PinName magnetPin, float speed, int32_t stepsDown)
    : m_stepper(stepPin, dirPin, enPin, 200 * 16)
    , m_sensorLift(sensorLift)
    , m_magnet(magnetPin, 0)
    , m_speed(speed)
    , m_stepsDown(stepsDown)
    , m_homed(false)
{
    m_sensorLift.mode(PullUp);
    // Mit PullUp: Ende erreicht (Strahl frei) → Phototransistor leitet → LOW (0)
    //             Während Fahrt (Strahl unterbrochen) → Phototransistor sperrt → HIGH (1)
    m_wasBlocked   = (m_sensorLift.read() == 0);
    m_triggerCount = 0;
    m_stepper.enable();
}

void LiftMotor::update()
{
    bool blocked = (m_sensorLift.read() == 0);  // LOW = Strahl frei = Endposition
    if (blocked && !m_wasBlocked)
        m_triggerCount++;
    m_wasBlocked = blocked;
}

void LiftMotor::moveUp()   { m_stepper.setVelocity(m_speed); }
void LiftMotor::moveDown() { m_stepper.setVelocity(-m_speed); }
void LiftMotor::stop()     { m_stepper.setVelocity(0.0f); }

void LiftMotor::grab()    { m_magnet = 1; }
void LiftMotor::release() { m_magnet = 0; }
bool LiftMotor::isGrabbing() { return m_magnet.read() == 1; }

// Lichtschranke oben: LOW wenn Strahl frei (Endposition erreicht)
bool LiftMotor::isAtTop() { return m_sensorLift.read() == 0; }

// Schrittbasiert: erst gültig nach setHome(); true wenn stepsDown Schritte unter Home (= 0)
bool LiftMotor::isAtBot()
{
    if (!m_homed) return false;
    return m_stepper.getSteps() <= -m_stepsDown;
}

int LiftMotor::sensorRaw() { return m_sensorLift.read(); }

// Nach erfolgreichem Homing aufrufen: Schritte auf 0 setzen, isAtBot() wird gültig
void LiftMotor::setHome()
{
    m_stepper.resetSteps();
    m_homed = true;
    printf("[LIFT] Home=0, Bot bei steps<=%ld  (LIFT_STEPS_DOWN kalibrieren!)\n",
           (long)(-m_stepsDown));
}

void    LiftMotor::resetTriggerCount()     { m_triggerCount = 0; }
int     LiftMotor::getTriggerCount() const { return m_triggerCount; }
int32_t LiftMotor::getSteps()              { return m_stepper.getSteps(); }
