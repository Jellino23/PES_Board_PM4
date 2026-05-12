/**
 * @file main.cpp
 * @brief Vial-Messanlage – Hauptprogramm.
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
#include "ui/DisplayLayout.h"
#include "ui/DHT11.h"
#include "XPT2046.h"

// ISR-Flag für physischen Startschalter
static volatile bool g_toggleRequest = false;
static void onButtonToggle() { g_toggleRequest = true; }

// ============================================================
// MAIN
// ============================================================
int main()
{
    // ---- Hardware ----
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
    StateMachine robot(lift, revolver, lid);

    // ---- Bedienelemente ----
    DigitalOut ledBusy(LED1, 0);
    DigitalOut enableMotors(PB_ENABLE_DCMOTORS, 0);
    DebounceIn userBtn(RobotConfig::START_BTN, PullUp);
    userBtn.fall(callback(&onButtonToggle));

    // ---- Display ----
    Display display(
        RobotConfig::DISP_MOSI, NC,
        RobotConfig::DISP_SCLK,
        RobotConfig::DISP_CS,
        RobotConfig::DISP_DC,
        RobotConfig::DISP_RST,
        8000000
    );
    display.init();

    DisplayLayout layout(display);
    layout.drawSplash();
    thread_sleep_for(1000);

    // ---- Touch ----
    XPT2046 touch(
        RobotConfig::DISP_MOSI,  // geteilter MOSI (PA_7)
        RobotConfig::TOUCH_MISO, // MISO Touch (PA_6)
        RobotConfig::DISP_SCLK,  // geteilter SCK  (PB_3)
        RobotConfig::TOUCH_CS,
        RobotConfig::TOUCH_IRQ
    );

    // ---- DHT11 ----
    DHT11 dht(RobotConfig::DHT_DATA);

    printf("[main] Bereit – Startschalter oder Touch druecken.\n");

    // ---- Loop-Variablen ----
    Timer loopTimer;
    loopTimer.start();

    State lastDispState  = State::ERROR; // Ungültig → erzwingt ersten Draw
    bool  lastDispRun    = false;
    int   dispRefreshMs  = 0;            // Zähler für Measuring-Balken (200ms)
    int   sensorRefreshMs = 0;           // Zähler für DHT11-Update (5000ms)

    float dispTemp  = 0.0f;
    float dispHum   = 0.0f;
    bool  dispValid = false;

    int   printCount = 0;

    // Initialen Draw erzwingen
    bool forceRedraw = true;

    while (true) {
        loopTimer.reset();

        // ---- Physischer Startschalter ----
        if (g_toggleRequest) {
            g_toggleRequest = false;
            robot.setRunning(!robot.isRunning());
            forceRedraw = true;
        }

        // ---- Touch-Button ----
        if (touch.update()) {
            // Neues Touch-Ereignis: prüfen ob im START/STOP-Bereich
            int tx = touch.getX();
            int ty = touch.getY();
            const DisplayLayout::Rect& btn = DisplayLayout::BTN_STARTSTOP;
            if (tx >= btn.x && tx < btn.x + btn.w &&
                ty >= btn.y && ty < btn.y + btn.h) {
                robot.setRunning(!robot.isRunning());
                forceRedraw = true;
            }
        }

        robot.update(RobotConfig::MAIN_PERIOD_MS);

        enableMotors = robot.isRunning() ? 1 : 0;

        // ---- LED ----
        if (robot.getState() == State::ERROR)
            ledBusy = (robot.getTimerMs() % 400 < 200) ? 1 : 0;
        else
            ledBusy = robot.isRunning() ? 1 : 0;

        // ---- DHT11: alle 5 Sekunden aktualisieren ----
        sensorRefreshMs += RobotConfig::MAIN_PERIOD_MS;
        if (sensorRefreshMs >= 5000) {
            sensorRefreshMs = 0;
            dispTemp  = dht.readTemperature();
            dispHum   = dht.readHumidity();
            dispValid = dht.isValid();
            forceRedraw = true;
        }

        // ---- Display: bei Zustandsänderung oder alle 200ms während MEASURING ----
        State cur     = robot.getState();
        bool  running = robot.isRunning();

        if (forceRedraw || cur != lastDispState || running != lastDispRun) {
            lastDispState = cur;
            lastDispRun   = running;
            dispRefreshMs = 0;
            forceRedraw   = false;
            layout.update(cur, robot.getTimerMs(), running,
                          RobotConfig::MEASURE_MS,
                          dispTemp, dispHum, dispValid);
        } else if (cur == State::MEASURING) {
            dispRefreshMs += RobotConfig::MAIN_PERIOD_MS;
            if (dispRefreshMs >= 200) {
                dispRefreshMs = 0;
                // Nur Balken aktualisieren – kein Flicker auf Status/Sensor/Button
                layout.updateProgress(robot.getTimerMs(), RobotConfig::MEASURE_MS);
            }
        }

        // ---- Serieller Status alle 2 s ----
        if (++printCount >= 100) {
            printCount = 0;
            printf("[SM] %s  run:%d  t:%d ms  T:%.0f C  H:%.0f%%\n",
                   stateName(robot.getState()),
                   (int)robot.isRunning(),
                   robot.getTimerMs(),
                   dispTemp, dispHum);
        }

        int elapsed = duration_cast<milliseconds>(loopTimer.elapsed_time()).count();
        int toSleep = RobotConfig::MAIN_PERIOD_MS - elapsed;
        if (toSleep > 0)
            thread_sleep_for(toSleep);
        else
            printf("Warnung: Loop %d ms zu langsam!\n", -toSleep);
    }
}
