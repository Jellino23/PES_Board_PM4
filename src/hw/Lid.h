#pragma once
#include "mbed.h"
#include "DCMotor.h"

/**
 * @file Lid.h
 * @brief Deckel-Achse (DC-Motor).
 *
 * Schliessen: Motor läuft bis Endschalter "geschlossen" aktiv.
 * Öffnen:     Motor dreht für eine feste Anzahl Umdrehungen
 *             (kein Endschalter auf der Öffnungsseite erforderlich).
 *
 * Hardware:
 *  - DC-Motor mit Encoder (DCMotor-Klasse aus dem Repo)
 *  - Endschalter "geschlossen" (TCST2103, active LOW, PullUp)
 */
class Lid {
public:
    /**
     * @param motorPWM, encA, encB   DC-Motor-Pins
     * @param gearRatio              Getriebeübersetzung
     * @param kn                     Motorkonstante [rpm/V]
     * @param voltageMax             Versorgungsspannung [V]
     * @param sensorClose            Endschalter "geschlossen" (active LOW)
     * @param speed                  Fahrgeschwindigkeit [rot/s]
     * @param openRotations          Umdrehungen zum vollständigen Öffnen
     */
    Lid(PinName motorPWM, PinName encA, PinName encB,
        float gearRatio, float kn, float voltageMax,
        float speed         = 0.1f,
        float openRotations = 0.35f);

    void openLid();
    void closeLid();
    void stopLid();

    bool  isClosed();
    bool  isOpen();
    float getRotation() const;

private:
    DCMotor m_motor;
    float   m_speed;
    float   m_openRotations;
};
