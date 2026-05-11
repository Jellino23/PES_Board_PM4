/**
 * @file main.cpp  [test/revolver]
 * @brief Revolver-Test: manueller Schritt-durch-Schritt-Modus.
 *
 * Jede Aktion laeuft automatisch bis zum Stopp.
 * Danach: Knopf (PB1) druecken um zum naechsten Schritt weiterzugehen.
 *
 * Reihenfolge:
 *   HOMING (CCW, Trigger) -> AT_VIAL -> TO_HOLE -> AT_HOLE
 *   -> TO_VIAL -> BACK_VIAL -> ADVANCE (CW, Trigger) -> NEXT_VIAL -> (Schleife)
 */

#include "mbed.h"
#include "PESBoardPinMap.h"
#include "DebounceIn.h"
#include "hw/Revolver.h"
#include "app/RobotConfig.h"

static const int PERIOD_MS        = 20;
static const int TEST_TIMEOUT_MS  = 15000;
static const int STARTUP_DELAY_MS = 200;

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

static volatile bool g_btnPressed = false;
static void onButton() { g_btnPressed = true; }

int main()
{
    Revolver revolver(
        RobotConfig::REV_STEP, RobotConfig::REV_DIR,
        RobotConfig::REV_EN,   RobotConfig::REV_VIAL,
        RobotConfig::REVOLVER_SPEED
    );

    // Lid-PWM-Pin explizit LOW halten – verhindert dass der angeschlossene
    // DC-Motor (M3) dreht solange er nicht benutzt wird.
    DigitalOut lidPwmHold(RobotConfig::LID_PWM, 0);

    DigitalOut ledBusy(LED1, 0);
    DebounceIn userBtn(RobotConfig::START_BTN, PullUp);
    userBtn.fall(callback(&onButton));

    printf("[REV-TEST] Bereit – PB1 druecken zum Starten.\n");
    printf("[REV-TEST] REV_STEPS_VIAL_TO_HOLE = %ld\n",
           RobotConfig::REV_STEPS_VIAL_TO_HOLE);

    TestState state       = TestState::IDLE;
    bool      entry       = true;
    int       timerMs     = 0;
    int       printCnt    = 0;
    bool      waitForBtn  = false;   // true = Aktion fertig, warten auf Knopf
    int       vialNum     = 0;
    bool      leftPos     = false;   // Debounce: erst Vial verlassen, dann naechsten Trigger zaehlen

    Timer loopTimer;
    loopTimer.start();

    while (true) {
        loopTimer.reset();

        revolver.update();

        const bool btnNow = g_btnPressed;
        g_btnPressed = false;

        if (!entry) timerMs += PERIOD_MS;

        // Warten auf Knopf zwischen den States
        if (waitForBtn) {
            if (btnNow) {
                waitForBtn = false;
                // Transition wurde bereits gesetzt, entry=true
            }
            int elapsed = duration_cast<milliseconds>(loopTimer.elapsed_time()).count();
            int toSleep = PERIOD_MS - elapsed;
            if (toSleep > 0) thread_sleep_for(toSleep);
            continue;
        }

        // Abbruch aus jedem State mit Knopf nur wenn Maschine laeuft
        // (In IDLE startet der Knopf)

        switch (state) {

        case TestState::IDLE:
            if (entry) {
                entry = false;
                ledBusy = 0;
                printf("[REV-TEST] IDLE – PB1 druecken zum Starten.\n");
            }
            if (btnNow) {
                ledBusy = 1;
                vialNum = 0;
                state = TestState::HOMING; entry = true; timerMs = 0;
            }
            break;

        case TestState::HOMING:
            if (entry) {
                entry = false;
                leftPos = false;
                printf("[REV-TEST] HOMING: CCW bis Vial-Sensor...\n");
                revolver.turnCCW();
            }
            // m_wasBlocked im Revolver verhindert Falsch-Trigger beim Start am Vial.
            // leftPos: falls wir mitten im Vial-Fenster starten, erst rausfahren.
            if (!leftPos && !revolver.isAtVial()) leftPos = true;
            if (leftPos && revolver.isAtVial()) {
                revolver.stop();
                printf("[REV-TEST] Homing fertig – steps=%ld\n", revolver.getSteps());
                printf("           >> Knopf druecken zum Weiterfahren.\n");
                state = TestState::AT_VIAL; entry = true; timerMs = 0;
                waitForBtn = true;
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
                printf("           >> Knopf druecken -> TO_HOLE.\n");
                waitForBtn = true;
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
                printf("[REV-TEST] TO_HOLE fertig – steps=%ld\n", revolver.getSteps());
                printf("           >> Knopf druecken -> AT_HOLE.\n");
                state = TestState::AT_HOLE; entry = true; timerMs = 0;
                waitForBtn = true;
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
                printf("           raw=1 -> Loch blockiert (Schritte anpassen)\n");
                printf("           >> Knopf druecken -> TO_VIAL.\n");
                waitForBtn = true;
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
                printf("[REV-TEST] TO_VIAL fertig – steps=%ld\n", revolver.getSteps());
                printf("           >> Knopf druecken -> BACK_VIAL.\n");
                state = TestState::BACK_VIAL; entry = true; timerMs = 0;
                waitForBtn = true;
            } else if (timerMs > TEST_TIMEOUT_MS) {
                state = TestState::ERROR_STATE; entry = true; timerMs = 0;
            }
            break;

        case TestState::BACK_VIAL:
            if (entry) {
                entry = false;
                printf("[REV-TEST] BACK_VIAL – raw=%d  steps=%ld\n",
                       revolver.isAtVial() ? 1 : 0, revolver.getSteps());
                printf("           >> Knopf druecken -> ADVANCE.\n");
                waitForBtn = true;
            }
            break;

        case TestState::ADVANCE:
            if (entry) {
                entry = false;
                leftPos = false;
                printf("[REV-TEST] ADVANCE: CW -> naechstes Vial...\n");
                revolver.turnCW();
            }
            // leftPos: erst aktuelles Vial verlassen, dann naechsten Trigger abwarten
            if (!leftPos && !revolver.isAtVial()) leftPos = true;
            if (leftPos && revolver.isAtVial()) {
                revolver.stop();
                printf("[REV-TEST] Naechstes Vial – steps=%ld\n", revolver.getSteps());
                printf("           >> Knopf druecken -> NEXT_VIAL.\n");
                state = TestState::NEXT_VIAL; entry = true; timerMs = 0;
                waitForBtn = true;
            } else if (timerMs > TEST_TIMEOUT_MS) {
                state = TestState::ERROR_STATE; entry = true; timerMs = 0;
            }
            break;

        case TestState::NEXT_VIAL:
            if (entry) {
                entry = false;
                printf("[REV-TEST] NEXT_VIAL – Zyklus fertig.\n");
                printf("           >> Knopf druecken -> naechstes AT_VIAL.\n");
                waitForBtn = true;
                state = TestState::AT_VIAL; entry = true; timerMs = 0;
            }
            break;

        case TestState::ERROR_STATE:
            if (entry) {
                entry = false;
                revolver.stop();
                printf("[REV-TEST] !!! TIMEOUT – gestoppt. Knopf druecken fuer Reset.\n");
            }
            ledBusy = (timerMs % 400 < 200) ? 1 : 0;
            if (btnNow) {
                ledBusy = 0;
                state = TestState::IDLE; entry = true; timerMs = 0;
            }
            break;
        }

        // Diagnose alle 500 ms (nur waehrend Bewegung sinnvoll)
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
