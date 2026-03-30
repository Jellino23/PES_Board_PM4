#pragma once
#include "mbed.h"
#include "StepperTMC2209.h"

/**
 * @brief Lift-Achse: fährt Vials hoch/runter, greift via Hubmagnet.
 *
 * Hardware:
 *  - Schrittmotor mit TMC2209-Treiber (STEP / DIR / EN)
 *  - Endschalter oben und unten (TCST2103, active LOW mit PullUp)
 *  - Hubmagnet (active HIGH: Strom = Magnet angezogen = Vial gegriffen)
 */
class LiftMotor {
public:
    LiftMotor(PinName stepPin, PinName dirPin, PinName enPin,
              PinName sensorTop, PinName sensorBottom,
              PinName magnetPin);

    // Bewegung (nicht-blockierend)
    void moveUp();
    void moveDown();
    void stop();

    // Greifer
    void grab();
    void release();
    bool isGrabbing() const;

    // Endschalter (active LOW → read()==0 bedeutet aktiv)
    bool isAtTop()    const;
    bool isAtBottom() const;

    // Homing: fährt hoch bis Endschalter oben, setzt Schritt-Nullpunkt
    void home();

    // Fahre auf absolute Position (Steps) mit Geschwindigkeit (rot/s)
    void moveTo(int32_t steps, float velocity = SPEED);

    // Gibt aktuellen Schrittstand zurück
    int32_t getSteps() const;

    static constexpr float SPEED = 1.0f; // [rot/s]

private:
    StepperTMC2209 m_stepper;
    DigitalIn      m_sensorTop;
    DigitalIn      m_sensorBottom;
    DigitalOut     m_magnet;
};
