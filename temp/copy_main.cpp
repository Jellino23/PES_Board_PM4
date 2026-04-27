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
#include "DHT11.h"
#include "XPT2046.h"

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
    RobotConfig::LIFT_SEN,
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

static DHT11 dht(RobotConfig::DHT_DATA);

// ============================================================
// UI-INSTANZEN
// ============================================================
static Display display(
    RobotConfig::DISP_MOSI,
    NC,                        // ST7735 ist write-only; SPI1 via PA_7+PB_3 identifiziert
    RobotConfig::DISP_SCLK,
    RobotConfig::DISP_CS,
    RobotConfig::DISP_DC,
    RobotConfig::DISP_RST
);

static XPT2046 touch(
    RobotConfig::DISP_MOSI,
    RobotConfig::TOUCH_MISO,
    RobotConfig::DISP_SCLK,
    RobotConfig::TOUCH_CS,
    RobotConfig::TOUCH_IRQ
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

        // --- Touch-Eingabe ---
        if (touch.isTouched()) {
            const auto& btn = DisplayLayout::BTN_STARTSTOP;
            int tx = touch.getX();
            int ty = touch.getY();
            if (tx >= btn.x && tx < btn.x + btn.w &&
                ty >= btn.y && ty < btn.y + btn.h) {
                robot.setRunning(!robot.isRunning());
            }
        }

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
