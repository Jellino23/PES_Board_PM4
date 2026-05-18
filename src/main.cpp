/**
 * @file main.cpp
 * @brief TEST – Hubmagnet an Pin PC_3.
 *
 * Branch: test/hubmagnet-pc3
 *
 * Zweck: Isolierter Test des Hubmagneten ohne State-Machine, Display etc.
 *
 * Bedienung:
 *   - Physischer Startschalter (PB_1, active-low) druecken → Magnet toggelt an/aus.
 *   - LED1 zeigt den aktuellen Magnet-Zustand (an = Magnet bestromt).
 *   - Serielle Ausgabe (115200) meldet jeden Schaltvorgang.
 *
 * Verdrahtung: PC_3 → Basis BD139 (Treibertransistor) → Hubmagnet.
 * Magnet ist active-HIGH: PC_3 = 1 → Magnet zieht an.
 *
 * Zurueck zur lauffaehigen Version:  git checkout main
 */

#include "mbed.h"
#include "PESBoardPinMap.h"
#include "DebounceIn.h"
#include "app/RobotConfig.h"

// ISR-Flag fuer physischen Startschalter
static volatile bool g_toggleRequest = false;
static void onButtonToggle() { g_toggleRequest = true; }

int main()
{
    // Hubmagnet-Ausgang – startet AUS (0)
    DigitalOut magnet(RobotConfig::LIFT_MAGNET, 0);

    // Statusanzeige
    DigitalOut led(LED1, 0);

    // Manche Aufbauten speisen den Magnet-Treiber ueber die Motor-Enable-
    // Leitung. Dauerhaft aktivieren, damit der Magnet ueberhaupt Strom bekommt.
    DigitalOut enableMotors(PB_ENABLE_DCMOTORS, 1);

    // Physischer Startschalter: active-low mit internem PullUp,
    // fall() loest beim Druecken aus.
    DebounceIn userBtn(RobotConfig::START_BTN, PullUp);
    userBtn.fall(callback(&onButtonToggle));

    bool magnetOn = false;

    printf("\n[TEST] Hubmagnet-Test auf %s (PC_3)\n", "LIFT_MAGNET");
    printf("[TEST] Startschalter (PB_1) druecken zum Umschalten.\n");
    printf("[TEST] Aktueller Zustand: AUS\n");

    while (true) {
        if (g_toggleRequest) {
            g_toggleRequest = false;

            magnetOn = !magnetOn;
            magnet   = magnetOn ? 1 : 0;
            led      = magnetOn ? 1 : 0;

            printf("[TEST] Magnet %s  (PC_3 = %d)\n",
                   magnetOn ? "AN " : "AUS", magnet.read());
        }

        thread_sleep_for(10);
    }
}
