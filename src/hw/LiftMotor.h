#pragma once
#include "mbed.h"
#include "StepperTMC2209.h"

/**
 * @file LiftMotor.h
 * @brief Lift-Achse: fährt Vials hoch/runter, greift mit Hubmagnet.
 *
 * Hardware:
 *  - Schrittmotor mit TMC2209-Treiber
 *  - Endschalter oben + unten (TCST2103, Phototransistor active HIGH)
 *  - Hubmagnet (HIGH = angezogen = Vial gegriffen)
 */
class LiftMotor {
public:
    LiftMotor(PinName stepPin, PinName dirPin, PinName enPin,
              PinName sensorLift,
              PinName magnetPin,
              float speed = 1.0f,
              int32_t stepsDown = -3200);

    void update();

    void moveUp();
    void moveDown();
    void stop();

    void grab();
    void release();
    bool isGrabbing();

    // Lichtschranke oben (TCST2103: active HIGH = unterbrochen)
    bool isAtTop();
    // Schrittbasiert: true nachdem setHome() aufgerufen und stepsDown Schritte nach unten gefahren
    bool isAtBot();
    int  sensorRaw();

    // Nach erfolgreichem Homing aufrufen – setzt Schritt-Referenz für isAtBot()
    void setHome();

    void resetTriggerCount();
    int  getTriggerCount() const;

    int32_t getSteps();

private:
    StepperTMC2209 m_stepper;
    DigitalIn      m_sensorLift;
    DigitalOut     m_magnet;
    bool           m_wasBlocked;
    int            m_triggerCount;
    float          m_speed;
    int32_t        m_stepsDown;
    bool           m_homed;
};
