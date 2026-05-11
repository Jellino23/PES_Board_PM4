#include "Lid.h"

Lid::Lid(PinName motorPWM, PinName encA, PinName encB,
         float gearRatio, float kn, float voltageMax,
         float speed, float openRotations)
    : m_motor(motorPWM, encA, encB, gearRatio, kn, voltageMax)
    , m_speed(speed)
    , m_openRotations(openRotations)
{
    m_motor.setMaxVelocity(m_speed);
    m_motor.setVelocity(0.0f);
}

void Lid::openLid()
{
    // DCMotor-Rotations-Modus: fahre openRotations in positive Richtung
    m_motor.setRotation(-m_openRotations);
}

void Lid::closeLid()
{
    m_motor.setRotation(m_openRotations);
}

void Lid::stopLid()
{
    m_motor.setVelocity(0.0f);
}

bool  Lid::isClosed()              { return m_motor.getRotation() >= m_openRotations * 0.9f; }
bool  Lid::isOpen()                { return m_motor.getRotation() <= -m_openRotations * 0.9f; }
void  Lid::runVelocity(float rps)  { m_motor.setVelocity(rps); }
float Lid::getRotation() const     { return m_motor.getRotation(); }
float Lid::getVelocity() const     { return m_motor.getVelocity(); }
float Lid::getVoltage()  const     { return m_motor.getVoltage(); }
float Lid::getPWM()      const     { return m_motor.getPWM(); }
long  Lid::getEncoderCount() const { return m_motor.getEncoderCount(); }
