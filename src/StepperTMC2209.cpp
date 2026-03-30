#include "StepperTMC2209.h"
#include <cmath>
#include <algorithm>

StepperTMC2209::StepperTMC2209(PinName stepPin, PinName dirPin, PinName enPin, uint32_t stepsPerRev)
    : m_Step(stepPin, 0)
    , m_Dir(dirPin, 0)
    , m_En(enPin, 1)          // start disabled (EN active LOW)
    , m_stepsPerRev(stepsPerRev)
    , m_timeStepConst(1.0e6f / static_cast<float>(stepsPerRev))
    , m_steps(0)
    , m_targetSteps(0)
    , m_velocity(0.0f)
    , m_moving(false)
    , m_Thread(osPriorityHigh2)
    , m_periodUs(0)
{
    m_Thread.start(callback(this, &StepperTMC2209::threadTask));
}

StepperTMC2209::~StepperTMC2209()
{
    m_Ticker.detach();
    m_En = 1; // disable driver
    m_Thread.terminate();
}

void StepperTMC2209::enable()  { m_En = 0; }
void StepperTMC2209::disable() { m_En = 1; m_Ticker.detach(); m_moving = false; }

// --------------------------------------------------------------------------
void StepperTMC2209::setVelocity(float velocity)
{
    if (velocity > 0.0f)
        applyVelocity(velocity, INT32_MAX);
    else if (velocity < 0.0f)
        applyVelocity(-velocity, INT32_MIN);
    else {
        m_Ticker.detach();
        m_moving   = false;
        m_velocity = 0.0f;
        m_periodUs = 0;
        m_targetSteps = m_steps;
    }
}

void StepperTMC2209::setSteps(int32_t targetSteps, float velocity)
{
    if (targetSteps == m_steps || velocity == 0.0f) {
        m_Ticker.detach();
        m_moving = false;
        m_velocity = 0.0f;
        m_targetSteps = m_steps;
        return;
    }
    m_targetSteps = targetSteps;
    applyVelocity(std::fabs(velocity),
                  (targetSteps > m_steps) ? INT32_MAX : INT32_MIN);
    // DIR is set inside applyVelocity according to targetSteps
    m_Dir = (targetSteps > m_steps) ? 1 : 0;
}

void StepperTMC2209::setStepsRelative(int32_t delta, float velocity)
{
    setSteps(m_steps + delta, velocity);
}

// --------------------------------------------------------------------------
void StepperTMC2209::applyVelocity(float absVel, int32_t /*sentinel target not used here*/)
{
    int newPeriod = static_cast<int>(m_timeStepConst / absVel + 0.5f);
    newPeriod = std::max(newPeriod, MIN_PRD_US);

    m_velocity = absVel;
    m_moving   = true;

    if (newPeriod != m_periodUs) {
        m_periodUs = newPeriod;
        m_Ticker.attach(callback(this, &StepperTMC2209::sendThreadFlag),
                        std::chrono::microseconds{newPeriod});
    }
}

// --------------------------------------------------------------------------
void StepperTMC2209::step()
{
    // Generate one STEP pulse (HIGH then LOW via Timeout)
    m_Step = 1;
    m_PulseTimeout.attach(callback(this, &StepperTMC2209::stepLow),
                          std::chrono::microseconds{PULSE_US});

    // Increment position counter
    if (m_Dir.read() == 1)
        m_steps++;
    else
        m_steps--;
}

void StepperTMC2209::stepLow() { m_Step = 0; }

// --------------------------------------------------------------------------
void StepperTMC2209::threadTask()
{
    while (true) {
        ThisThread::flags_wait_any(m_ThreadFlag);

        if (!m_moving) continue;

        // Check if we have reached the target in position mode
        const int32_t t = m_targetSteps;
        if (t != INT32_MAX && t != INT32_MIN) {
            if (m_steps == t) {
                m_Ticker.detach();
                m_moving   = false;
                m_velocity = 0.0f;
                m_periodUs = 0;
                continue;
            }
            // Keep direction correct
            m_Dir = (t > m_steps) ? 1 : 0;
        }
        step();
    }
}

void StepperTMC2209::sendThreadFlag()
{
    m_Thread.flags_set(m_ThreadFlag);
}
