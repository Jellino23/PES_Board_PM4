#pragma once
#include "mbed.h"
#include "StepperTMC2209.h"

/**
 * @file Revolver.h
 * @brief Revolver-Achse mit einer einzigen Lichtschranke.
 *
 * Stop-Sequenz (CW): Vial → Loch → Vial → Loch → …
 *
 * Hardware:
 *  - Schrittmotor mit TMC2209-Treiber
 *  - Eine Lichtschranke (TCST2103, active LOW) für alle Stoppositionen
 *
 * update() muss jede Loop-Iteration aufgerufen werden, damit
 * getTriggerCount() korrekte Flankenzählung liefert.
 */
class Revolver {
public:
    Revolver(PinName stepPin, PinName dirPin, PinName enPin,
             PinName sensorPin,
             float speed = 0.5f);

    // Muss jede Loop-Iteration aufgerufen werden (Flankenerkennung)
    void update();

    // --- Nicht-blockierende Drehbefehle ---
    void turnCW();
    void turnCCW();
    void stop();

    // --- Präzise Schrittbewegung (nutzt StepperTMC2209-internen Step-Thread) ---
    void moveSteps(int32_t steps);  // positiv = CW, negativ = CCW
    bool isMoving() const;

    // --- Lichtschranke (TCST2103: active LOW) ---
    // Beide Methoden lesen denselben Sensor; der Kontext (State) entscheidet,
    // was die Unterbrechung bedeutet.
    bool isAtVial();
    bool isAtHole();

    // Flankenzähler – für DONE-State (2 Trigger = Loch überspringen)
    void resetTriggerCount();
    int  getTriggerCount() const;

    int32_t getSteps();

private:
    StepperTMC2209 m_stepper;
    DigitalIn      m_sensor;
    bool           m_wasBlocked;
    int            m_triggerCount;
    float          m_speed;
};
