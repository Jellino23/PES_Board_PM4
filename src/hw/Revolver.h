#pragma once
#include "mbed.h"
#include "StepperTMC2209.h"

/**
 * @file Revolver.h
 * @brief Revolver-Achse mit Lichtschranken-Positionierung.
 *
 * Slot-Sequenz auf dem Revolver (zyklisch):
 *   VIAL – LOCH – VIAL – VIAL – LOCH – …
 *
 * Hardware:
 *  - Schrittmotor mit TMC2209-Treiber
 *  - Lichtschranke Vial-Position (TCST2103, active LOW)
 *  - Lichtschranke Loch-Position (TCST2103, active LOW)
 *
 * Die Positionierung erfolgt ausschliesslich über Lichtschranken –
 * kein Schrittzähler als primäre Referenz.
 */
class Revolver {
public:
    Revolver(PinName stepPin, PinName dirPin, PinName enPin,
             PinName sensorVial, PinName sensorHole,
             float speed = 0.5f);

    // --- Nicht-blockierende Drehbefehle ---
    void turnCW();
    void turnCCW();
    void stop();

    // --- Lichtschranken (TCST2103: active LOW) ---
    bool isAtVial();
    bool isAtHole();

    int32_t getSteps();

private:
    StepperTMC2209 m_stepper;
    DigitalIn      m_sensorVial;
    DigitalIn      m_sensorHole;
    float          m_speed;
};
