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
              PinName sensorTop, PinName sensorBottom,
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
    bool isAtTop();
    bool isAtBottom();

    int32_t getSteps();

private:
    StepperTMC2209 m_stepper;
    DigitalIn      m_sensorTop;
    DigitalIn      m_sensorBottom;
    DigitalOut     m_magnet;
    float          m_speed;
};
