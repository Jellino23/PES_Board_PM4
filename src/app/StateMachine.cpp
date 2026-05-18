#include "StateMachine.h"
#include <cstdio>

// ============================================================
// stateName()
// ============================================================
const char* stateName(State s)
{
    switch (s) {
        case State::IDLE:               return "IDLE";
        case State::HOMING:             return "HOMING";
        case State::ROTATE_TO_VIAL:     return "ROT>VIAL";
        case State::LIFT_DOWN_PICK:     return "LFT DN PK";
        case State::GRAB:               return "GRAB";
        case State::LIFT_UP:            return "LIFT UP";
        case State::ROTATE_TO_HOLE:     return "ROT>HOLE";
        case State::LIFT_DOWN_PLACE:    return "LFT DN PL";
        case State::RELEASE:            return "RELEASE";
        case State::LIFT_UP_EMPTY:      return "LIFT EMPT";
        case State::CLOSE_LID:          return "CLOSE LID";
        case State::MEASURING:          return "MEASURING";
        case State::OPEN_LID:           return "OPEN LID";
        case State::LIFT_DOWN_RETRIEVE: return "LFT DN RT";
        case State::GRAB_AGAIN:         return "GRAB AGAIN";
        case State::LIFT_UP_RETURN:     return "LIFT UP R";
        case State::ROTATE_BACK:        return "ROT BACK";
        case State::LIFT_DOWN_RETURN:   return "LFT DN RH";
        case State::RELEASE_HOME:       return "REL HOME";
        case State::LIFT_UP_FINAL:      return "LIFT FIN";
        case State::DONE:               return "DONE";
        case State::ERROR:              return "ERROR";
    }
    return "?";
}

// ============================================================
// Konstruktor
// ============================================================
StateMachine::StateMachine(LiftMotor& lift, Revolver& revolver, Lid& lid)
    : m_lift(lift)
    , m_revolver(revolver)
    , m_lid(lid)
{}

// ============================================================
// Öffentliche API
// ============================================================
void StateMachine::setRunning(bool run)
{
    m_running = run;
    if (!run) {
        stopAll();
        m_state     = State::IDLE;
        m_entry     = true;
        m_timerMs   = 0;
        m_vialIndex = 0;
        printf("[SM] Gestoppt und zurueckgesetzt.\n");
    } else {
        printf("[SM] Start angefordert.\n");
    }
}

void StateMachine::update(int deltaMs)
{
    if (!m_running) return;

    m_lift.update();
    m_revolver.update();

    if (!m_entry)
        m_timerMs += deltaMs;

    switch (m_state) {
        case State::IDLE:               handleIdle();              break;
        case State::HOMING:             handleHoming();            break;
        case State::ROTATE_TO_VIAL:     handleRotateToVial();      break;
        case State::LIFT_DOWN_PICK:     handleLiftDownPick();      break;
        case State::GRAB:               handleGrab();              break;
        case State::LIFT_UP:            handleLiftUp();            break;
        case State::ROTATE_TO_HOLE:     handleRotateToHole();      break;
        case State::LIFT_DOWN_PLACE:    handleLiftDownPlace();     break;
        case State::RELEASE:            handleRelease();           break;
        case State::LIFT_UP_EMPTY:      handleLiftUpEmpty();       break;
        case State::CLOSE_LID:          handleCloseLid();          break;
        case State::MEASURING:          handleMeasuring();         break;
        case State::OPEN_LID:           handleOpenLid();           break;
        case State::LIFT_DOWN_RETRIEVE: handleLiftDownRetrieve();  break;
        case State::GRAB_AGAIN:         handleGrabAgain();         break;
        case State::LIFT_UP_RETURN:     handleLiftUpReturn();      break;
        case State::ROTATE_BACK:        handleRotateBack();        break;
        case State::LIFT_DOWN_RETURN:   handleLiftDownReturn();    break;
        case State::RELEASE_HOME:       handleReleaseHome();       break;
        case State::LIFT_UP_FINAL:      handleLiftUpFinal();       break;
        case State::DONE:               handleDone();              break;
        case State::ERROR:              handleError();             break;
    }
}

// ============================================================
// Private Hilfsmethoden
// ============================================================
void StateMachine::transitionTo(State next)
{
    printf("[SM] %s -> %s\n", stateName(m_state), stateName(next));
    m_state   = next;
    m_entry   = true;
    m_timerMs = 0;
}

void StateMachine::stopAll()
{
    m_lift.stop();
    m_lift.release();
    m_revolver.stop();
    m_lid.stopLid();
}

// ============================================================
// Zustandshandler
// ============================================================

void StateMachine::handleIdle()
{
    if (m_entry) {
        m_entry = false;
        stopAll();
    }
    // Warten auf setRunning(true); der Übergang nach HOMING
    // erfolgt beim nächsten update() nach dem Start.
    // Da setRunning(true) m_state auf IDLE belässt, triggern wir hier:
    transitionTo(State::HOMING);
}

void StateMachine::handleHoming()
{
    if (m_entry) {
        m_entry = false; m_leftPos = false;
        printf("[SM] Homing... (Magnet zu)\n");
        m_lift.grab();                 // HOMING = zu
        m_lift.moveUp();
        m_revolver.turnCW();
    }

    const bool liftOk = m_lift.isAtTop();
    if (!m_leftPos && !m_revolver.isAtVial()) m_leftPos = true;
    const bool revOk  = m_leftPos && m_revolver.isAtVial();

    if (liftOk)  m_lift.stop();
    if (revOk)   m_revolver.stop();

    if (liftOk && revOk) {
        transitionTo(State::ROTATE_TO_VIAL);
    } else if (m_timerMs > RobotConfig::TIMEOUT_LIFT_MS + RobotConfig::TIMEOUT_REV_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleRotateToVial()
{
    // Nach Homing stehen wir bereits auf der Vial-Position.
    // Bei Folgezyklen ist der Revolver nach DONE bereits auf der nächsten
    // Vial-Position angehalten.
    if (m_entry) {
        m_entry = false;
        m_lift.grab();                 // ROTATE_TO_VIAL = zu
    }
    transitionTo(State::LIFT_DOWN_PICK);
}

void StateMachine::handleLiftDownPick()
{
    if (m_entry) {
        m_entry = false; m_leftPos = false;
        printf("[SM] Lift runter -> Vial greifen\n");
        m_lift.resetTriggerCount();
        m_lift.moveDown();
    }
    if (!m_leftPos && !m_lift.isAtTop()) m_leftPos = true;
    if (m_leftPos && m_lift.getTriggerCount() >= 1) {
        m_lift.stop();
        transitionTo(State::GRAB);
    } else if (m_timerMs > RobotConfig::TIMEOUT_LIFT_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleGrab()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Vial greifen (Magnet unveraendert = zu)\n");
    }
    if (m_timerMs > RobotConfig::GRAB_WAIT_MS) {
        transitionTo(State::LIFT_UP);
    }
}

void StateMachine::handleLiftUp()
{
    if (m_entry) {
        m_entry = false; m_leftPos = false;
        printf("[SM] Lift hoch\n");
        m_lift.moveUp();
    }
    if (!m_leftPos && !m_lift.isAtTop()) m_leftPos = true;
    if (m_leftPos && m_lift.isAtTop()) {
        m_lift.stop();
        transitionTo(State::ROTATE_TO_HOLE);
    } else if (m_timerMs > RobotConfig::TIMEOUT_LIFT_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleRotateToHole()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Revolver %ld Schritte CW -> Loch\n", RobotConfig::REV_STEPS_VIAL_TO_HOLE);
        m_revolver.moveSteps(RobotConfig::REV_STEPS_VIAL_TO_HOLE);
    }
    if (!m_revolver.isMoving()) {
        transitionTo(State::LIFT_DOWN_PLACE);
    } else if (m_timerMs > RobotConfig::TIMEOUT_REV_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleLiftDownPlace()
{
    if (m_entry) {
        m_entry = false; m_leftPos = false;
        printf("[SM] Vial durch Loch in Messanlage\n");
        m_lift.resetTriggerCount();
        m_lift.moveDown();
    }
    if (!m_leftPos && !m_lift.isAtTop()) m_leftPos = true;
    if (m_leftPos && m_lift.getTriggerCount() >= 2) {
        m_lift.stop();
        transitionTo(State::RELEASE);
    } else if (m_timerMs > RobotConfig::TIMEOUT_LIFT_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleRelease()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Vial in Messanlage loesen (Magnet auf)\n");
        m_lift.release();              // RELEASE = auf
    }
    if (m_timerMs > RobotConfig::GRAB_WAIT_MS) {
        transitionTo(State::LIFT_UP_EMPTY);
    }
}

void StateMachine::handleLiftUpEmpty()
{
    if (m_entry) {
        m_entry = false; m_leftPos = false;
        printf("[SM] Lift hoch (leer)\n");
        m_lift.moveUp();
    }
    if (!m_leftPos && !m_lift.isAtTop()) m_leftPos = true;
    if (m_leftPos && m_lift.isAtTop()) {
        m_lift.stop();
        transitionTo(State::CLOSE_LID);
    } else if (m_timerMs > RobotConfig::TIMEOUT_LIFT_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleCloseLid()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Deckel schliessen (Magnet zu)  openRot=%.3f\n", m_lid.getRotation());
        m_lift.grab();                 // CLOSE_LID = zu
        m_lid.closeLid();
    }
    if (m_timerMs % 500 < RobotConfig::MAIN_PERIOD_MS)
        printf("[LID CLOSE] t=%d  rot=%.3f  isClosed=%d\n",
               m_timerMs, m_lid.getRotation(), (int)m_lid.isClosed());
    if (m_lid.isClosed()) {
        m_lid.stopLid();
        transitionTo(State::MEASURING);
    } else if (m_timerMs > RobotConfig::TIMEOUT_LID_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleMeasuring()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Messung laeuft (%d ms)...\n", RobotConfig::MEASURE_MS);
    }
    if (m_timerMs >= RobotConfig::MEASURE_MS) {
        printf("[SM] Messung abgeschlossen.\n");
        transitionTo(State::OPEN_LID);
    }
}

void StateMachine::handleOpenLid()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Deckel oeffnen\n");
        m_lid.openLid();
    }
    if (m_lid.isOpen()) {
        m_lid.stopLid();
        transitionTo(State::LIFT_DOWN_RETRIEVE);
    } else if (m_timerMs > RobotConfig::TIMEOUT_LID_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleLiftDownRetrieve()
{
    if (m_entry) {
        m_entry = false; m_leftPos = false;
        printf("[SM] Lift runter -> Vial holen\n");
        m_lift.resetTriggerCount();
        m_lift.moveDown();
    }
    if (!m_leftPos && !m_lift.isAtTop()) m_leftPos = true;
    if (m_leftPos && m_lift.getTriggerCount() >= 1) {
        m_lift.stop();
        transitionTo(State::GRAB_AGAIN);
    } else if (m_timerMs > RobotConfig::TIMEOUT_LIFT_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleGrabAgain()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Vial zurueckholen (Magnet unveraendert = zu)\n");
    }
    if (m_timerMs > RobotConfig::GRAB_WAIT_MS) {
        transitionTo(State::LIFT_UP_RETURN);
    }
}

void StateMachine::handleLiftUpReturn()
{
    if (m_entry) {
        m_entry = false; m_leftPos = false;
        m_lift.resetTriggerCount();
        printf("[SM] Lift hoch (Vial zurueck)\n");
        m_lift.moveUp();
    }
    if (!m_leftPos && !m_lift.isAtTop()) m_leftPos = true;
    if (m_leftPos && m_lift.getTriggerCount() >= 2) {
        m_lift.stop();
        transitionTo(State::ROTATE_BACK);
    } else if (m_timerMs > RobotConfig::TIMEOUT_LIFT_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleRotateBack()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Revolver %ld Schritte CCW -> Vial\n", RobotConfig::REV_STEPS_VIAL_TO_HOLE);
        m_revolver.moveSteps(-RobotConfig::REV_STEPS_VIAL_TO_HOLE);
    }
    if (!m_revolver.isMoving()) {
        transitionTo(State::LIFT_DOWN_RETURN);
    } else if (m_timerMs > RobotConfig::TIMEOUT_REV_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleLiftDownReturn()
{
    if (m_entry) {
        m_entry = false; m_leftPos = false;
        printf("[SM] Vial in Startposition ablassen\n");
        m_lift.resetTriggerCount();
        m_lift.moveDown();
    }
    if (!m_leftPos && !m_lift.isAtTop()) m_leftPos = true;
    if (m_leftPos && m_lift.getTriggerCount() >= 1) {
        m_lift.stop();
        transitionTo(State::RELEASE_HOME);
    } else if (m_timerMs > RobotConfig::TIMEOUT_LIFT_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleReleaseHome()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Vial in Startposition loesen (Magnet auf)\n");
        m_lift.release();              // RELEASE_HOME = auf
    }
    if (m_timerMs > RobotConfig::GRAB_WAIT_MS) {
        transitionTo(State::LIFT_UP_FINAL);
    }
}

void StateMachine::handleLiftUpFinal()
{
    if (m_entry) {
        m_entry = false; m_leftPos = false;
        printf("[SM] Lift hoch (final)\n");
        m_lift.moveUp();
    }
    if (!m_leftPos && !m_lift.isAtTop()) m_leftPos = true;
    if (m_leftPos && m_lift.isAtTop()) {
        m_lift.stop();
        transitionTo(State::DONE);
    } else if (m_timerMs > RobotConfig::TIMEOUT_LIFT_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleDone()
{
    if (m_entry) {
        m_entry = false;
        m_vialIndex++;
        printf("[SM] Zyklus %d abgeschlossen (Magnet zu). Revolver -> naechstes Vial\n",
               m_vialIndex);
        m_lift.grab();                 // DONE = zu
        m_revolver.resetTriggerCount();
        m_revolver.turnCW();
    }
    // Sensor nur bei Vial: erster Trigger = nächstes Vial
    if (m_revolver.getTriggerCount() >= 1) {
        m_revolver.stop();
        transitionTo(State::LIFT_DOWN_PICK);
    } else if (m_timerMs > RobotConfig::TIMEOUT_REV_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleError()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] !!! FEHLER – alle Aktoren gestoppt !!!\n");
        stopAll();
    }
    // Verbleibt im ERROR-Zustand bis setRunning(false) aufgerufen wird
}
