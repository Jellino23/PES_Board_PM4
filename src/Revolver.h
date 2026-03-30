#pragma once
#include "mbed.h"
#include "StepperTMC2209.h"

/**
 * @brief Revolver-Achse.
 *
 * Revolverrad mit abwechselnden Vial- und Lochpositionen.
 * Positionen werden über die Lichtschranke (TCST2103) erkannt.
 *
 * Sequenz der Slots auf dem Rad (unendlich wiederholt):
 *   VIAL – LOCH – VIAL – VIAL – LOCH – …
 *
 * Zwischen zwei benachbarten Slots dreht der Revolver SLOT_STEPS Schritte.
 * Die genaue Anzahl wird mechanisch durch die Konstruktion bestimmt und
 * ist im Konstruktor konfigurierbar.
 *
 * Hardware:
 *  - Schrittmotor mit TMC2209-Treiber
 *  - Lichtschranke am Revolver-Vial-Slot (TCST2103, active LOW)
 *  - Lichtschranke am Revolver-Loch-Slot (TCST2103, active LOW)
 */
class Revolver {
public:
    /**
     * @param stepPin, dirPin, enPin  TMC2209-Pins
     * @param sensorVial  Lichtschranke signalisiert Vial-Position
     * @param sensorHole  Lichtschranke signalisiert Loch-Position
     * @param slotSteps   Schritte zwischen zwei benachbarten Slots
     */
    Revolver(PinName stepPin, PinName dirPin, PinName enPin,
             PinName sensorVial, PinName sensorHole,
             int32_t slotSteps = 200 * 16 / 5); // example: 5 slots / rev

    // Nicht-blockierend drehen
    void turnCW();
    void turnCCW();
    void stop();

    // Sensor-Abfragen (TCST2103: active LOW)
    bool isAtVial() const;
    bool isAtHole() const;

    // Blockierend zur nächsten Vial-Position drehen (CW)
    void stepToNextVial();

    // Blockierend zur nächsten Loch-Position drehen (CW)
    void stepToNextHole();

    // Homing: dreht bis erste Vial-Lichtschranke aktiv, setzt Null
    void home();

    // Schrittstand
    int32_t getSteps() const;

    static constexpr float SPEED = 0.5f; // [rot/s]

private:
    StepperTMC2209 m_stepper;
    DigitalIn      m_sensorVial;
    DigitalIn      m_sensorHole;
    int32_t        m_slotSteps;

    // Drive until sensorFn returns true (blocks calling thread)
    void driveUntil(Callback<bool()> sensorFn, bool cw);
};
