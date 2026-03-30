#pragma once
#include "mbed.h"
#include "DCMotor.h"

/**
 * @brief Deckel-Achse.
 *
 * Schliessen: Motor läuft bis Endschalter „geschlossen" aktiv ist.
 * Öffnen:     Motor läuft für eine feste Anzahl Umdrehungen (keine
 *             Endschalter auf der Öffnungsseite nötig).
 *
 * Hardware:
 *  - DC-Motor mit Encoder (DCMotor-Klasse)
 *  - Endschalter „geschlossen" (active LOW, PullUp)
 *  - (optional) Endschalter „offen" (active LOW, PullUp)
 */
class Lid {
public:
    /**
     * @param motorPWM, encA, encB  DC-Motor-Pins
     * @param gearRatio             Getriebeübersetzung des Deckelmotors
     * @param kn                    Motorkonstante [rpm/V]
     * @param sensorClose           Endschalter geschlossen (active LOW)
     * @param sensorOpen            Endschalter offen (active LOW, NC = NC_PIN)
     * @param openRotations         Umdrehungen die der Motor zum Öffnen macht
     */
    Lid(PinName motorPWM, PinName encA, PinName encB,
        float gearRatio, float kn,
        PinName sensorClose,
        PinName sensorOpen    = NC,
        float   openRotations = 2.5f);

    void openLid();   // nicht-blockierend
    void closeLid();  // nicht-blockierend
    void stopLid();

    bool isOpen()  const;
    bool isClosed() const;

    static constexpr float VOLTAGE_MAX = 12.0f;
    static constexpr float SPEED       = 0.3f;  // [rot/s]

private:
    DCMotor   m_motor;
    DigitalIn m_sensorClose;
    DigitalIn m_sensorOpen;
    float     m_openRotations;
    bool      m_hasOpenSensor;
};
