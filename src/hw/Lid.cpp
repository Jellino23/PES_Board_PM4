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

void Lid::openLid()  { m_motor.setRotation(-m_openRotations); }
void Lid::closeLid() { m_motor.setRotation( m_openRotations); }
void Lid::stopLid()  { m_motor.setVelocity(0.0f); }

bool Lid::isClosed() { return m_motor.getRotation() >= m_openRotations * 0.9f; }
bool Lid::isOpen()   { return m_motor.getRotation() <= -m_openRotations * 0.9f; }
