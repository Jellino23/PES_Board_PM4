/**
 * @file main.cpp
 * @brief DC-Motor (Deckel) Test – Encoder und Drehrichtung prüfen.
 *
 * Knopf (PB1) durchläuft folgende Zustände:
 *   0 IDLE      – Motor gestoppt
 *   1 FWD_SLOW  – vorwärts 0.05 rot/s
 *   2 FWD_FAST  – vorwärts 0.15 rot/s
 *   3 BWD_SLOW  – rückwärts 0.05 rot/s
 *   4 BWD_FAST  – rückwärts 0.15 rot/s
 *   5 OPEN      – setRotation(-openRot) → Deckel auffahren
 *   6 CLOSE     – setRotation(+openRot) → Deckel zufahren
 *   → zurück zu 0
 *
 * Serial (115200): jede Loop Rotation, Velocity, Encoder, Voltage ausgeben.
 */

#include "mbed.h"
#include "PESBoardPinMap.h"
#include "DebounceIn.h"
#include "hw/Lid.h"
#include "app/RobotConfig.h"

static volatile bool g_btnPressed = false;
static void onBtn() { g_btnPressed = true; }

int main()
{
    DigitalOut  enableMotors(PB_ENABLE_DCMOTORS, 1);   // H-Brücke einschalten
    DigitalOut  led(LED1, 0);

    Lid lid(
        RobotConfig::LID_PWM,  RobotConfig::LID_ENCA,
        RobotConfig::LID_ENCB, RobotConfig::LID_GEAR_RATIO,
        RobotConfig::LID_KN,   RobotConfig::LID_VOLTAGE_MAX,
        RobotConfig::LID_SPEED, RobotConfig::LID_OPEN_ROTATIONS
    );

    DebounceIn btn(RobotConfig::START_BTN, PullUp);
    btn.fall(callback(&onBtn));

    const int   NUM_STATES  = 7;
    const char* stateName[] = {
        "IDLE", "FWD_SLOW", "FWD_FAST", "BWD_SLOW", "BWD_FAST", "OPEN", "CLOSE"
    };
    int state = 0;

    printf("\n=== DC-Motor Test ===\n");
    printf("Knopf (PB1) druecken zum Weiterschalten.\n");
    printf("Zustand: %s\n", stateName[state]);

    Timer loopTimer;
    loopTimer.start();
    int printCount = 0;

    while (true) {
        loopTimer.reset();

        if (g_btnPressed) {
            g_btnPressed = false;
            state = (state + 1) % NUM_STATES;

            switch (state) {
                case 0: lid.stopLid();              break;
                case 1: lid.runVelocity( 0.05f);    break;
                case 2: lid.runVelocity( 0.15f);    break;
                case 3: lid.runVelocity(-0.05f);    break;
                case 4: lid.runVelocity(-0.15f);    break;
                case 5: lid.openLid();              break;
                case 6: lid.closeLid();             break;
            }

            printf("[BTN] -> %s\n", stateName[state]);
            led = (state != 0) ? 1 : 0;
        }

        // Alle 500 ms Diagnose ausgeben
        if (++printCount >= 25) {
            printCount = 0;
            printf("[LID] state=%-9s  rot=%6.3f  vel=%6.3f  enc=%6ld  V=%5.2f\n",
                   stateName[state],
                   lid.getRotation(),
                   lid.getVelocity(),
                   lid.getEncoderCount(),
                   lid.getVoltage());
        }

        int elapsed = duration_cast<milliseconds>(loopTimer.elapsed_time()).count();
        int toSleep = RobotConfig::MAIN_PERIOD_MS - elapsed;
        if (toSleep > 0) thread_sleep_for(toSleep);
    }
}
