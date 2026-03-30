#include "Lid.h"

static constexpr float GEAR_RATIO_DEFAULT = 100.0f;

Lid::Lid(PinName motorPWM, PinName encA, PinName encB,
         float gearRatio, float kn,
         PinName sensorClose, PinName sensorOpen,
         float openRotations)
    : m_motor(motorPWM, encA, encB, gearRatio, kn, VOLTAGE_MAX)
    , m_sensorClose(sensorClose)
    , m_sensorOpen(sensorOpen != NC ? sensorOpen : sensorClose) // fallback
    , m_openRotations(openRotations)
    , m_hasOpenSensor(sensorOpen != NC)
{
    m_sensorClose.mode(PullUp);
    if (m_hasOpenSensor)
        m_sensorOpen.mode(PullUp);

    m_motor.setMaxVelocity(SPEED);
    m_motor.setVelocity(0.0f);
}

void Lid::openLid()
{
    // Fahre um openRotations in positive Richtung
    m_motor.setRotationRelative(m_openRotations);
}

void Lid::closeLid()
{
    // Fahre in negative Richtung – State Machine stoppt beim Endschalter
    m_motor.setVelocity(-SPEED);
}

void Lid::stopLid()
{
    m_motor.setVelocity(0.0f);
}

// TCST2103 – active LOW
bool Lid::isClosed() const { return m_sensorClose.read() == 0; }

bool Lid::isOpen() const
{
    if (m_hasOpenSensor)
        return m_sensorOpen.read() == 0;
    // Schätzung über Motorposition wenn kein Sensor vorhanden
    return (m_motor.getRotation() >= m_openRotations * 0.9f);
}
