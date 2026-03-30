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
        m_entry = false;
        printf("[SM] Homing...\n");
        m_lift.release();
        m_lift.moveUp();
        m_revolver.turnCW();
    }

    const bool liftOk = m_lift.isAtTop();
    const bool revOk  = m_revolver.isAtVial();

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
    }
    transitionTo(State::LIFT_DOWN_PICK);
}

void StateMachine::handleLiftDownPick()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Lift runter -> Vial greifen\n");
        m_lift.moveDown();
    }
    if (m_lift.isAtBottom()) {
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
        printf("[SM] Magnet AN\n");
        m_lift.grab();
    }
    if (m_timerMs > RobotConfig::GRAB_WAIT_MS) {
        transitionTo(State::LIFT_UP);
    }
}

void StateMachine::handleLiftUp()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Lift hoch (Vial gegriffen)\n");
        m_lift.moveUp();
    }
    if (m_lift.isAtTop()) {
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
        printf("[SM] Revolver CW -> Loch\n");
        m_revolver.turnCW();
    }
    if (m_revolver.isAtHole()) {
        m_revolver.stop();
        transitionTo(State::LIFT_DOWN_PLACE);
    } else if (m_timerMs > RobotConfig::TIMEOUT_REV_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleLiftDownPlace()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Vial durch Loch in Messanlage\n");
        m_lift.moveDown();
    }
    if (m_lift.isAtBottom()) {
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
        printf("[SM] Magnet AUS (Vial in Messanlage)\n");
        m_lift.release();
    }
    if (m_timerMs > RobotConfig::GRAB_WAIT_MS) {
        transitionTo(State::LIFT_UP_EMPTY);
    }
}

void StateMachine::handleLiftUpEmpty()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Lift hoch (leer)\n");
        m_lift.moveUp();
    }
    if (m_lift.isAtTop()) {
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
        printf("[SM] Deckel schliessen\n");
        m_lid.closeLid();
    }
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
        m_entry = false;
        printf("[SM] Lift runter -> Vial holen\n");
        m_lift.moveDown();
    }
    if (m_lift.isAtBottom()) {
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
        printf("[SM] Magnet AN (Vial zurueckholen)\n");
        m_lift.grab();
    }
    if (m_timerMs > RobotConfig::GRAB_WAIT_MS) {
        transitionTo(State::LIFT_UP_RETURN);
    }
}

void StateMachine::handleLiftUpReturn()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Lift hoch (Vial zurueck)\n");
        m_lift.moveUp();
    }
    if (m_lift.isAtTop()) {
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
        printf("[SM] Revolver CCW -> zurueck zur Vial-Position\n");
        m_revolver.turnCCW(); // Gegenrichtung!
    }
    if (m_revolver.isAtVial()) {
        m_revolver.stop();
        transitionTo(State::LIFT_DOWN_RETURN);
    } else if (m_timerMs > RobotConfig::TIMEOUT_REV_MS) {
        transitionTo(State::ERROR);
    }
}

void StateMachine::handleLiftDownReturn()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Vial in Startposition ablassen\n");
        m_lift.moveDown();
    }
    if (m_lift.isAtBottom()) {
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
        printf("[SM] Magnet AUS (Vial abgelegt)\n");
        m_lift.release();
    }
    if (m_timerMs > RobotConfig::GRAB_WAIT_MS) {
        transitionTo(State::LIFT_UP_FINAL);
    }
}

void StateMachine::handleLiftUpFinal()
{
    if (m_entry) {
        m_entry = false;
        printf("[SM] Lift hoch (final)\n");
        m_lift.moveUp();
    }
    if (m_lift.isAtTop()) {
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
        printf("[SM] Zyklus %d abgeschlossen. Revolver -> naechstes Vial\n",
               m_vialIndex);
        // Revolver zum nächsten Vial-Slot drehen
        m_revolver.turnCW();
    }
    // Warte auf nächste Vial-Lichtschranke (überspringt Loch-Slots automatisch)
    if (m_revolver.isAtVial()) {
        m_revolver.stop();
        transitionTo(State::LIFT_DOWN_PICK); // direkt nächstes Vial
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
