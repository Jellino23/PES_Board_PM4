/**
 * @file main.cpp
 * @brief Vial-Messanlage – Hauptprogramm (Orchestrierung).
 *
 * main.cpp ist nur noch Verdrahtung:
 *  1. Hardware-Objekte erstellen
 *  2. StateMachine und DisplayLayout damit verbinden
 *  3. Haupt-Loop: update() + display.update()
 *
 * Keine Logik, keine Konstanten, keine Magic Numbers hier.
 * Alles in StateMachine.cpp / RobotConfig.h / DisplayLayout.cpp.
 */

#include "mbed.h"
#include "PESBoardPinMap.h"
#include "DebounceIn.h"

// Hardware-Treiber
#include "hw/LiftMotor.h"
#include "hw/Revolver.h"
#include "hw/Lid.h"

// UI
#include "ui/Display.h"
#include "ui/DisplayLayout.h"

// Applikation
#include "app/StateMachine.h"
#include "app/RobotConfig.h"

// ============================================================
// HARDWARE-INSTANZEN
// ============================================================
static LiftMotor lift(
    RobotConfig::LIFT_STEP,
    RobotConfig::LIFT_DIR,
    RobotConfig::LIFT_EN,
    RobotConfig::LIFT_TOP,
    RobotConfig::LIFT_BOT,
    RobotConfig::LIFT_MAGNET,
    RobotConfig::LIFT_SPEED
);

static Revolver revolver(
    RobotConfig::REV_STEP,
    RobotConfig::REV_DIR,
    RobotConfig::REV_EN,
    RobotConfig::REV_VIAL,
    RobotConfig::REV_HOLE,
    RobotConfig::REVOLVER_SPEED
);

static Lid lid(
    RobotConfig::LID_PWM,
    RobotConfig::LID_ENCA,
    RobotConfig::LID_ENCB,
    RobotConfig::LID_GEAR_RATIO,
    RobotConfig::LID_KN,
    RobotConfig::LID_VOLTAGE_MAX,
    RobotConfig::LID_CLOSE,
    RobotConfig::LID_SPEED,
    RobotConfig::LID_OPEN_ROTATIONS
);

// ============================================================
// UI-INSTANZEN
// ============================================================
static Display display(
    RobotConfig::DISP_MOSI,
    RobotConfig::DISP_MISO,
    RobotConfig::DISP_SCLK,
    RobotConfig::DISP_CS,
    RobotConfig::DISP_DC,
    RobotConfig::DISP_RST
);
static DisplayLayout screen(display);

// ============================================================
// APPLIKATION
// ============================================================
static StateMachine robot(lift, revolver, lid);

// ============================================================
// BEDIENELEMENTE
// ============================================================
static DigitalOut  ledBusy(LED1, 0);
static DebounceIn  userBtn(BUTTON1);

static void onButtonToggle()
{
    robot.setRunning(!robot.isRunning());
}

// ============================================================
// MAIN
// ============================================================
int main()
{
    display.init();
    screen.drawSplash();

    userBtn.fall(callback(&onButtonToggle));

    Timer loopTimer;
    loopTimer.start();

    int dispCounter = 0;

    while (true) {
        loopTimer.reset();

        // --- State Machine ---
        robot.update(RobotConfig::MAIN_PERIOD_MS);

        // --- LED Feedback ---
        if (robot.getState() == State::ERROR) {
            // Schnelles Blinken bei Fehler
            ledBusy = (robot.getTimerMs() % 400 < 200) ? 1 : 0;
        } else {
            ledBusy = robot.isRunning() ? 1 : 0;
        }

        // --- Display (alle 5 Zyklen = 100 ms) ---
        if (++dispCounter >= 5) {
            dispCounter = 0;
            screen.update(
                robot.getState(),
                robot.getVialIndex(),
                robot.getTimerMs(),
                robot.isRunning(),
                RobotConfig::MEASURE_MS
            );
        }

        // --- Timing ---
        int elapsed = duration_cast<milliseconds>(loopTimer.elapsed_time()).count();
        int toSleep = RobotConfig::MAIN_PERIOD_MS - elapsed;
        if (toSleep > 0)
            thread_sleep_for(toSleep);
        else
            printf("Warnung: Loop %d ms zu langsam!\n", -toSleep);
    }
}
