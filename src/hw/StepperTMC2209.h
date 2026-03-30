#pragma once
#include "mbed.h"
#include "ThreadFlag.h"

/**
 * @file StepperTMC2209.h
 * @brief Direkter STEP/DIR/EN-Treiber für den TMC2209 Schrittmotor-Treiber.
 *
 * Der TMC2209 wird über drei Pins angesteuert:
 *   STEP – steigende Flanke = ein Mikroschritt
 *   DIR  – HIGH = CW, LOW = CCW
 *   EN   – LOW  = Treiber aktiv (active-LOW!)
 *
 * Schritte werden über Ticker → Thread → Timeout erzeugt,
 * identisch zum Muster der bestehenden Repo-Klassen (Stepper, DCMotor).
 * Kein Software-Stall-Detection, kein UART-Protokoll.
 */
class StepperTMC2209 {
public:
    /**
     * @param stepPin       STEP-Ausgangspin
     * @param dirPin        DIR-Ausgangspin
     * @param enPin         EN-Ausgangspin (active LOW)
     * @param stepsPerRev   Schritte pro Umdrehung inkl. Mikroschrittauflösung
     *                      z.B. 200 Vollschritte × 16 Mikroschritte = 3200
     */
    explicit StepperTMC2209(PinName stepPin,
                             PinName dirPin,
                             PinName enPin,
                             uint32_t stepsPerRev = 200 * 16);
    ~StepperTMC2209();

    void enable();
    void disable();

    /**
     * @brief Kontinuierliche Drehung (Velocity-Modus).
     * @param velocity  Drehzahl in Umdrehungen/s; Vorzeichen = Richtung; 0 = Stopp
     */
    void setVelocity(float velocity);

    /**
     * @brief Absolute Schrittposition anfahren.
     * @param targetSteps  Ziel-Schrittposition
     * @param velocity     Geschwindigkeit [rot/s], muss > 0 sein
     */
    void setSteps(int32_t targetSteps, float velocity);

    /**
     * @brief Relative Schrittanzahl fahren.
     */
    void setStepsRelative(int32_t deltaSteps, float velocity);

    int32_t getSteps()    const { return m_steps; }
    float   getVelocity() const { return m_velocity; }
    bool    isMoving()    const { return m_moving; }

private:
    static constexpr int PULSE_US   = 10;  // STEP-Puls HIGH-Dauer [µs] (≥100 ns laut TMC2209 Datenblatt, 10 µs safe margin)
    static constexpr int MIN_PRD_US = PULSE_US + 2; // Mindest-Ticker-Periode [µs]

    DigitalOut m_Step;
    DigitalOut m_Dir;
    DigitalOut m_En;

    uint32_t m_stepsPerRev;
    float    m_timeStepConst;  // µs pro Schritt bei 1 rot/s

    volatile int32_t m_steps;
    volatile int32_t m_targetSteps;
    volatile float   m_velocity;
    volatile bool    m_moving;
    volatile bool    m_exitThread;

    Thread     m_Thread;
    Ticker     m_Ticker;
    Timeout    m_PulseTimeout;
    ThreadFlag m_ThreadFlag;

    int m_periodUs;

    void applyVelocity(float absVel);
    void step();
    void stepLow();
    void sendThreadFlag();
    void threadTask();
};
