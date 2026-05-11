/**
 * @file main.cpp
 * @brief Vial-Messanlage – Hauptprogramm.
 *
 * Objekte werden in main() erstellt (nicht als statische Globals),
 * da statische Initialisierung vor dem RTOS-Start zu Crashes führt.
 */

#include "mbed.h"
#include "PESBoardPinMap.h"
#include "DebounceIn.h"

#include "hw/LiftMotor.h"
#include "hw/Revolver.h"
#include "hw/Lid.h"

#include "app/StateMachine.h"
#include "app/RobotConfig.h"
#include "ui/Display.h"

// Flag wird im ISR gesetzt, setRunning() im Haupt-Loop aufgerufen
// (printf darf nicht aus ISR-Kontext aufgerufen werden)
static volatile bool g_toggleRequest = false;

static void onButtonToggle()
{
    g_toggleRequest = true;
}

// ============================================================
// MAIN
// ============================================================
int main()
{
    // Hardware
    LiftMotor lift(
        RobotConfig::LIFT_STEP, RobotConfig::LIFT_DIR,
        RobotConfig::LIFT_EN,   RobotConfig::LIFT_SEN,
        RobotConfig::LIFT_MAGNET, RobotConfig::LIFT_SPEED
    );
    Revolver revolver(
        RobotConfig::REV_STEP, RobotConfig::REV_DIR,
        RobotConfig::REV_EN,   RobotConfig::REV_VIAL,
        RobotConfig::REVOLVER_SPEED
    );
    Lid lid(
        RobotConfig::LID_PWM,  RobotConfig::LID_ENCA,
        RobotConfig::LID_ENCB, RobotConfig::LID_GEAR_RATIO,
        RobotConfig::LID_KN,   RobotConfig::LID_VOLTAGE_MAX,
        RobotConfig::LID_SPEED, RobotConfig::LID_OPEN_ROTATIONS
    );
    // Applikation
    StateMachine robot(lift, revolver, lid);

    // Bedienelemente
    DigitalOut ledBusy(LED1, 0);
    DigitalOut enableMotors(PB_ENABLE_DCMOTORS, 0);
    DebounceIn userBtn(RobotConfig::START_BTN, PullUp);
    userBtn.fall(callback(&onButtonToggle));

    // Display
    Display display(
        RobotConfig::DISP_MOSI, NC,
        RobotConfig::DISP_SCLK,
        RobotConfig::DISP_CS,
        RobotConfig::DISP_DC,
        RobotConfig::DISP_RST,
        8000000
    );
    display.init();
    display.fillScreen(Display::BLACK);
    display.drawText(2, 2, "STATE:", Display::GRAY);
    display.drawText(2, 16, "IDLE", Display::WHITE);

    printf("[main] Bereit – Startschalter (PB1) druecken zum Starten.\n");

    Timer loopTimer;
    loopTimer.start();

    State lastState  = State::IDLE;
    int   printCount = 0;

    while (true) {
        loopTimer.reset();

        // Button-Toggle aus ISR-Flag verarbeiten
        if (g_toggleRequest) {
            g_toggleRequest = false;
            robot.setRunning(!robot.isRunning());
        }

        robot.update(RobotConfig::MAIN_PERIOD_MS);

        enableMotors = robot.isRunning() ? 1 : 0;

        // LED
        if (robot.getState() == State::ERROR)
            ledBusy = (robot.getTimerMs() % 400 < 200) ? 1 : 0;
        else
            ledBusy = robot.isRunning() ? 1 : 0;

        // State-Wechsel sofort ausgeben
        State cur = robot.getState();
        if (cur != lastState) {
            lastState = cur;
            printf("[SM] -> %s\n", stateName(cur));
            uint16_t col = (cur == State::ERROR) ? Display::RED
                         : robot.isRunning()      ? Display::GREEN
                                                  : Display::WHITE;
            display.fillRect(0, 16, Display::WIDTH, 10, Display::BLACK);
            display.drawText(2, 16, stateName(cur), col);
        }

        // Alle 2 s aktuellen State wiederholen
        if (++printCount >= 100) {
            printCount = 0;
            printf("[SM] %s  run:%d  t:%d ms\n",
                   stateName(robot.getState()),
                   (int)robot.isRunning(),
                   robot.getTimerMs());
        }

        int elapsed = duration_cast<milliseconds>(loopTimer.elapsed_time()).count();
        int toSleep = RobotConfig::MAIN_PERIOD_MS - elapsed;
        if (toSleep > 0)
            thread_sleep_for(toSleep);
        else
            printf("Warnung: Loop %d ms zu langsam!\n", -toSleep);
    }
}
