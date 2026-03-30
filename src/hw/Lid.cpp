#include "Lid.h"

Lid::Lid(PinName motorPWM, PinName encA, PinName encB,
         float gearRatio, float kn, float voltageMax,
         PinName sensorClose,
         float speed, float openRotations)
    : m_motor(motorPWM, encA, encB, gearRatio, kn, voltageMax)
    , m_sensorClose(sensorClose)
    , m_speed(speed)
    , m_openRotations(openRotations)
{
    m_sensorClose.mode(PullUp);
    m_motor.setMaxVelocity(m_speed);
    m_motor.setVelocity(0.0f);
}

void Lid::openLid()
{
    // DCMotor-Rotations-Modus: fahre openRotations in positive Richtung
    m_motor.setRotationRelative(m_openRotations);
}

void Lid::closeLid()
{
    // Velocity-Modus in negative Richtung; State Machine stoppt beim Endschalter
    m_motor.setVelocity(-m_speed);
}

void Lid::stopLid()
{
    m_motor.setVelocity(0.0f);
}

// TCST2103: active LOW
bool Lid::isClosed() { return m_sensorClose.read() == 0; }

bool Lid::isOpen()
{
    // Schätzung über Motorposition (kein zweiter Endschalter)
    return m_motor.getRotation() >= m_openRotations * 0.9f;
}
