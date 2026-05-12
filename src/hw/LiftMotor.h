#pragma once
#include "mbed.h"
#include "StepperTMC2209.h"

/**
 * @file LiftMotor.h
 * @brief Lift-Achse: fährt Vials hoch/runter, greift mit Hubmagnet.
 *
 * Hardware:
 *  - Schrittmotor mit TMC2209-Treiber
 *  - Endschalter oben + unten (TCST2103, Phototransistor active LOW)
 *  - Hubmagnet (HIGH = angezogen = Vial gegriffen)
 */
class LiftMotor {
public:
    LiftMotor(PinName stepPin, PinName dirPin, PinName enPin,
              PinName sensorLift,
              PinName magnetPin,
              float speed = 1.0f);

    // --- Bewegung (nicht-blockierend) ---
    void moveUp();
    void moveDown();
    void stop();

    // --- Greifer ---
    void grab();
    void release();
    bool isGrabbing();

    // --- Endschalter (TCST2103: active LOW) ---
    bool isAtEnd();
    bool isAtTop();

    // --- Flankenzaehler (fuer Lift-Down Positionierung) ---
    void update();               // bleibt fuer API-Kompatibilitaet (no-op)
    void resetTriggerCount();
    int  getTriggerCount() const;

    int32_t getSteps();

private:
    StepperTMC2209 m_stepper;
    DigitalIn      m_sensorLift;
    DigitalOut     m_magnet;
    float          m_speed;
    Ticker         m_sensorTicker;     // unabhaengig vom Haupt-Loop
    volatile bool  m_wasBlocked;
    volatile int   m_triggerCount;

    void pollSensor();  // Ticker-Callback, alle 5 ms
};
