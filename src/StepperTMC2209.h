#pragma once
#include "mbed.h"

/**
 * @brief Minimal STEP/DIR stepper motor driver for TMC2209.
 *
 * The TMC2209 is controlled via:
 *   STEP pin  – rising edge = one microstep
 *   DIR  pin  – HIGH = CW, LOW = CCW
 *   EN   pin  – LOW = driver enabled, HIGH = driver disabled (active LOW)
 *
 * Steps are generated with a Ticker/Thread pair identical to the pattern
 * used throughout this project. No software stall-detection or UART
 * features of the TMC2209 are used here.
 */
class StepperTMC2209 {
public:
    /**
     * @param stepPin   STEP output pin
     * @param dirPin    DIR output pin
     * @param enPin     EN  output pin  (active LOW)
     * @param stepsPerRev Full-step count per revolution × microstepping
     *                    e.g. 200 steps × 16 ustep = 3200
     */
    explicit StepperTMC2209(PinName stepPin,
                             PinName dirPin,
                             PinName enPin,
                             uint32_t stepsPerRev = 200 * 16);
    ~StepperTMC2209();

    // --- Driver enable/disable ---
    void enable();
    void disable();

    // --- Velocity mode (continuous rotation) ---
    // velocity in rotations/s, sign = direction; 0 = stop
    void setVelocity(float velocity);

    // --- Position mode ---
    // Move to absolute step position at given speed (rot/s)
    void setSteps(int32_t targetSteps, float velocity);
    // Move by delta steps at given speed
    void setStepsRelative(int32_t deltaSteps, float velocity);

    // --- Status ---
    int32_t getSteps()   const { return m_steps; }
    float   getVelocity() const { return m_velocity; }
    bool    isMoving()    const { return m_moving; }

private:
    static constexpr int PULSE_US   = 2;   // STEP pulse HIGH duration [µs]
    static constexpr int MIN_PRD_US = 10;  // minimum ticker period [µs]

    DigitalOut m_Step;
    DigitalOut m_Dir;
    DigitalOut m_En;

    uint32_t m_stepsPerRev;
    float    m_timeStepConst; // µs per step at 1 rot/s

    volatile int32_t m_steps;         // current absolute step count
    volatile int32_t m_targetSteps;
    volatile float   m_velocity;      // active velocity magnitude (rot/s)
    volatile bool    m_moving;

    Thread     m_Thread;
    Ticker     m_Ticker;
    Timeout    m_PulseTimeout;
    ThreadFlag m_ThreadFlag;

    int m_periodUs; // current ticker period

    void applyVelocity(float velocity, int32_t target);
    void step();
    void stepLow();
    void sendThreadFlag();
    void threadTask();
};
