/**
 * @file main.cpp  [test/revolver]
 * @brief Revolver-Test: Homing, Schritt-Kalibrierung, Vial-zu-Vial-Zyklus.
 *
 * Workflow:
 *   Knopf -> HOMING (CCW, 1. Trigger) -> AT_VIAL
 *         -> TO_HOLE (+steps) -> AT_HOLE (Step-Ausgabe fuer Kalibrierung)
 *         -> TO_VIAL (-steps) -> BACK_VIAL
 *         -> ADVANCE (CW, 1. Trigger) -> NEXT_VIAL -> (Schleife)
 *
 * STARTUP_DELAY_MS: Motor muss sich erst physisch bewegen,
 * bevor die Lichtschranke als Stoppsignal gilt.
 */

#include "mbed.h"
#include "PESBoardPinMap.h"
#include "DebounceIn.h"
#include "hw/Revolver.h"
#include "app/RobotConfig.h"

static const int PERIOD_MS        = 20;
static const int TEST_TIMEOUT_MS  = 15000;
static const int STARTUP_DELAY_MS = 200;
static const int PAUSE_MS         = 500;

enum class TestState {
    IDLE,
    HOMING,
    AT_VIAL,
    TO_HOLE,
    AT_HOLE,
    TO_VIAL,
    BACK_VIAL,
    ADVANCE,
    NEXT_VIAL,
    ERROR_STATE
};

static const char* testStateName(TestState s)
{
    switch (s) {
        case TestState::IDLE:        return "IDLE";
        case TestState::HOMING:      return "HOMING";
        case TestState::AT_VIAL:     return "AT_VIAL";
        case TestState::TO_HOLE:     return "TO_HOLE";
        case TestState::AT_HOLE:     return "AT_HOLE";
        case TestState::TO_VIAL:     return "TO_VIAL";
        case TestState::BACK_VIAL:   return "BACK_VIAL";
        case TestState::ADVANCE:     return "ADVANCE";
        case TestState::NEXT_VIAL:   return "NEXT_VIAL";
        case TestState::ERROR_STATE: return "ERROR";
    }
    return "?";
}

static volatile bool g_toggleRequest = false;
static void onButton() { g_toggleRequest = true; }

int main()
{
    Revolver revolver(
        RobotConfig::REV_STEP, RobotConfig::REV_DIR,
        RobotConfig::REV_EN,   RobotConfig::REV_VIAL,
        RobotConfig::REVOLVER_SPEED
    );

    DigitalOut ledBusy(LED1, 0);
    DigitalOut enableMotors(PB_ENABLE_DCMOTORS, 0);
    DebounceIn userBtn(RobotConfig::START_BTN, PullUp);
    userBtn.fall(callback(&onButton));

    printf("[REV-TEST] Bereit – PB1 druecken zum Starten.\n");
    printf("[REV-TEST] REV_STEPS_VIAL_TO_HOLE = %ld\n",
           RobotConfig::REV_STEPS_VIAL_TO_HOLE);

    TestState state   = TestState::IDLE;
    bool      entry   = true;
    int       timerMs = 0;
    int       printCnt = 0;
    bool      running = false;
    int       vialNum = 0;

    Timer loopTimer;
    loopTimer.start();

    while (true) {
        loopTimer.reset();

        revolver.update();

        // Knopf: Start/Stop toggle
        if (g_toggleRequest) {
            g_toggleRequest = false;
            if (!running) {
                running      = true;
                enableMotors = 1;
                ledBusy      = 1;
                vialNum      = 0;
                state        = TestState::HOMING;
                entry        = true;
                timerMs      = 0;
                printf("[REV-TEST] Start!\n");
            } else {
                running      = false;
                enableMotors = 0;
                ledBusy      = 0;
                revolver.stop();
                state        = TestState::IDLE;
                entry        = true;
                timerMs      = 0;
                printf("[REV-TEST] Gestoppt.\n");
            }
        }

        if (!running) {
            int elapsed = duration_cast<milliseconds>(loopTimer.elapsed_time()).count();
            int toSleep = PERIOD_MS - elapsed;
            if (toSleep > 0) thread_sleep_for(toSleep);
            continue;
        }

        if (!entry) timerMs += PERIOD_MS;

        switch (state) {

        case TestState::IDLE:
            break;

        case TestState::HOMING:
            if (entry) {
                entry = false;
                printf("[REV-TEST] HOMING: CCW bis Vial...\n");
                revolver.resetTriggerCount();
                revolver.turnCCW();
            }
            if (timerMs > STARTUP_DELAY_MS && revolver.getTriggerCount() >= 1) {
                revolver.stop();
                printf("[REV-TEST] Homing fertig – trig=%d  steps=%ld\n",
                       revolver.getTriggerCount(), revolver.getSteps());
                state = TestState::AT_VIAL; entry = true; timerMs = 0;
            } else if (timerMs > TEST_TIMEOUT_MS) {
                state = TestState::ERROR_STATE; entry = true; timerMs = 0;
            }
            break;

        case TestState::AT_VIAL:
            if (entry) {
                entry = false;
                vialNum++;
                printf("[REV-TEST] AT_VIAL #%d – raw=%d  steps=%ld\n",
                       vialNum, revolver.isAtVial() ? 1 : 0, revolver.getSteps());
            }
            if (timerMs > PAUSE_MS) {
                state = TestState::TO_HOLE; entry = true; timerMs = 0;
            }
            break;

        case TestState::TO_HOLE:
            if (entry) {
                entry = false;
                printf("[REV-TEST] TO_HOLE: %ld Schritte CW\n",
                       RobotConfig::REV_STEPS_VIAL_TO_HOLE);
                revolver.moveSteps(RobotConfig::REV_STEPS_VIAL_TO_HOLE);
            }
            if (!revolver.isMoving()) {
                state = TestState::AT_HOLE; entry = true; timerMs = 0;
            } else if (timerMs > TEST_TIMEOUT_MS) {
                state = TestState::ERROR_STATE; entry = true; timerMs = 0;
            }
            break;

        case TestState::AT_HOLE:
            if (entry) {
                entry = false;
                printf("[REV-TEST] AT_HOLE – steps=%ld  raw=%d\n",
                       revolver.getSteps(), revolver.isAtVial() ? 1 : 0);
                printf("           raw=0 -> Loch frei (korrekt)\n");
                printf("           raw=1 -> Loch blockiert (REV_STEPS_VIAL_TO_HOLE anpassen)\n");
            }
            if (timerMs > PAUSE_MS) {
                state = TestState::TO_VIAL; entry = true; timerMs = 0;
            }
            break;

        case TestState::TO_VIAL:
            if (entry) {
                entry = false;
                printf("[REV-TEST] TO_VIAL: %ld Schritte CCW (Rueckfahrt)\n",
                       RobotConfig::REV_STEPS_VIAL_TO_HOLE);
                revolver.moveSteps(-RobotConfig::REV_STEPS_VIAL_TO_HOLE);
            }
            if (!revolver.isMoving()) {
                state = TestState::BACK_VIAL; entry = true; timerMs = 0;
            } else if (timerMs > TEST_TIMEOUT_MS) {
                state = TestState::ERROR_STATE; entry = true; timerMs = 0;
            }
            break;

        case TestState::BACK_VIAL:
            if (entry) {
                entry = false;
                printf("[REV-TEST] BACK_VIAL – raw=%d  steps=%ld\n",
                       revolver.isAtVial() ? 1 : 0, revolver.getSteps());
            }
            if (timerMs > PAUSE_MS) {
                state = TestState::ADVANCE; entry = true; timerMs = 0;
            }
            break;

        case TestState::ADVANCE:
            if (entry) {
                entry = false;
                printf("[REV-TEST] ADVANCE: CW -> naechstes Vial\n");
                revolver.resetTriggerCount();
                revolver.turnCW();
            }
            // Nach STARTUP_DELAY_MS kann die Lichtschranke als Stop gelten
            if (timerMs > STARTUP_DELAY_MS && revolver.getTriggerCount() >= 1) {
                revolver.stop();
                printf("[REV-TEST] Naechstes Vial – trig=%d  steps=%ld\n",
                       revolver.getTriggerCount(), revolver.getSteps());
                state = TestState::NEXT_VIAL; entry = true; timerMs = 0;
            } else if (timerMs > TEST_TIMEOUT_MS) {
                state = TestState::ERROR_STATE; entry = true; timerMs = 0;
            }
            break;

        case TestState::NEXT_VIAL:
            if (entry) {
                entry = false;
                printf("[REV-TEST] NEXT_VIAL – weiter zu AT_VIAL\n");
            }
            if (timerMs > PAUSE_MS) {
                state = TestState::AT_VIAL; entry = true; timerMs = 0;
            }
            break;

        case TestState::ERROR_STATE:
            if (entry) {
                entry = false;
                revolver.stop();
                printf("[REV-TEST] !!! TIMEOUT – alles gestoppt. Knopf zum Reset.\n");
            }
            ledBusy = (timerMs % 400 < 200) ? 1 : 0;
            break;
        }

        // Diagnose alle 500 ms
        if (++printCnt >= (500 / PERIOD_MS)) {
            printCnt = 0;
            printf("[DIAG] %-10s  raw=%d  trig=%d  steps=%ld  t=%d ms\n",
                   testStateName(state),
                   revolver.isAtVial() ? 1 : 0,
                   revolver.getTriggerCount(),
                   revolver.getSteps(),
                   timerMs);
        }

        int elapsed = duration_cast<milliseconds>(loopTimer.elapsed_time()).count();
        int toSleep = PERIOD_MS - elapsed;
        if (toSleep > 0)
            thread_sleep_for(toSleep);
        else
            printf("Warnung: Loop %d ms zu langsam!\n", -toSleep);
    }
}
