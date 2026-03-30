#pragma once
#include "mbed.h"
#include "LiftMotor.h"
#include "Revolver.h"
#include "Lid.h"
#include "RobotConfig.h"

/**
 * @file StateMachine.h
 * @brief State Machine der Vial-Messanlage.
 *
 * Enthält:
 *  - enum class State  (alle Zustände)
 *  - stateName()       (für Logging + Display)
 *  - class StateMachine (Logik, Hardware-Referenzen, Zustandsübergänge)
 *
 * StateMachine kennt die Hardware-Klassen, aber nicht das Display.
 * main.cpp verbindet beides.
 */

// ============================================================
// Zustandsliste
// ============================================================
enum class State {
    IDLE,
    HOMING,
    ROTATE_TO_VIAL,
    LIFT_DOWN_PICK,
    GRAB,
    LIFT_UP,
    ROTATE_TO_HOLE,
    LIFT_DOWN_PLACE,
    RELEASE,
    LIFT_UP_EMPTY,
    CLOSE_LID,
    MEASURING,
    OPEN_LID,
    LIFT_DOWN_RETRIEVE,
    GRAB_AGAIN,
    LIFT_UP_RETURN,
    ROTATE_BACK,
    LIFT_DOWN_RETURN,
    RELEASE_HOME,
    LIFT_UP_FINAL,
    DONE,
    ERROR
};

/**
 * @brief Gibt den lesbaren Namen eines Zustands zurück.
 *        Wird von StateMachine und DisplayLayout genutzt.
 */
const char* stateName(State s);

// ============================================================
// State Machine
// ============================================================
class StateMachine {
public:
    /**
     * @param lift, revolver, lid  Referenzen auf die Hardware-Objekte.
     *                             Die StateMachine besitzt diese Objekte NICHT –
     *                             sie werden von main.cpp gehalten.
     */
    StateMachine(LiftMotor& lift, Revolver& revolver, Lid& lid);

    /**
     * @brief Einen Schritt der State Machine ausführen.
     *        Muss periodisch (z.B. alle 20 ms) aufgerufen werden.
     * @param deltaMs  Verstrichene Zeit seit letztem Aufruf [ms]
     */
    void update(int deltaMs);

    /**
     * @brief Start/Stop toggle.
     * @param run true = starten, false = stoppen und zurücksetzen
     */
    void setRunning(bool run);
    bool isRunning() const { return m_running; }

    State   getState()      const { return m_state; }
    int     getVialIndex()  const { return m_vialIndex; }
    int     getTimerMs()    const { return m_timerMs; }

private:
    // Hardware-Referenzen (kein Ownership)
    LiftMotor& m_lift;
    Revolver&  m_revolver;
    Lid&       m_lid;

    State m_state     = State::IDLE;
    bool  m_entry     = true;   // true = erster Aufruf in neuem Zustand
    int   m_timerMs   = 0;
    int   m_vialIndex = 0;
    bool  m_running   = false;

    void transitionTo(State next);
    void stopAll();

    // Ein Handler pro Zustand; gibt true zurück wenn Übergang erfolgte
    void handleIdle();
    void handleHoming();
    void handleRotateToVial();
    void handleLiftDownPick();
    void handleGrab();
    void handleLiftUp();
    void handleRotateToHole();
    void handleLiftDownPlace();
    void handleRelease();
    void handleLiftUpEmpty();
    void handleCloseLid();
    void handleMeasuring();
    void handleOpenLid();
    void handleLiftDownRetrieve();
    void handleGrabAgain();
    void handleLiftUpReturn();
    void handleRotateBack();
    void handleLiftDownReturn();
    void handleReleaseHome();
    void handleLiftUpFinal();
    void handleDone();
    void handleError();
};
