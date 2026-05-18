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
    // Hubmagnet-Ausgang an PC_6 (=PB_D2), echter Digital-Ausgang.
    // PC_3/PC_2 (=PB_A1/PB_A0) sind ANALOG-Eingaenge (board-seitige RC-/
    // Teiler-Hardware) und als Treiber-Ausgang untauglich.
    //
    // INVERTIERTE LOGIK: Treiberstufe ist ein NPN-Pegelumsetzer
    //   GPIO -[150R]- Basis NPN,  Basis -[10k]- +5V (Pull-up),
    //   Emitter -> GND,  Collector -> MOSFET-Gate (Gate -[10k]- +12V).
    //   GPIO LOW  -> NPN sperrt -> Gate auf 12V -> MOSFET an -> Magnet AN
    //   GPIO HIGH -> NPN leitet -> Gate auf 0V  -> MOSFET aus -> Magnet AUS
    //   GPIO high-Z (Reset/Boot/Flash) -> 5V-Pull-up haelt NPN an
    //                                     -> Magnet sicher AUS.
    // Steuerdraht (NPN-Basis ueber 150R) an Anschluss PB_D2.
    static constexpr PinName MAGNET_PIN = PC_6; // PB_D2
    static constexpr int     MAG_ON  = 0;       // GPIO LOW  = Magnet AN
    static constexpr int     MAG_OFF = 1;       // GPIO HIGH = Magnet AUS
    DigitalOut magnet(MAGNET_PIN, MAG_OFF);

    // Statusanzeige
    DigitalOut led(LED1, 0);

    // Magnet-Treiber (BD139) wird NICHT ueber PB_ENABLE_DCMOTORS versorgt
    // (vom Nutzer bestaetigt) – Zeile bleibt ohne Wirkung auf den Magneten.
    DigitalOut enableMotors(PB_ENABLE_DCMOTORS, 0);

    // Physischer Startschalter: active-low mit internem PullUp,
    // fall() loest beim Druecken aus.
    DebounceIn userBtn(RobotConfig::START_BTN, PullUp);
    userBtn.fall(callback(&onButtonToggle));

    bool magnetOn = false;

    printf("\n[TEST] Hubmagnet-Test auf PC_6 (PB_D2), NPN-Stufe (invertiert)\n");
    printf("[TEST] Startschalter (PB_1) druecken zum Umschalten.\n");
    printf("[TEST] Aktueller Zustand: AUS\n");

    while (true) {
        if (g_toggleRequest) {
            g_toggleRequest = false;

            magnetOn = !magnetOn;
            magnet   = magnetOn ? MAG_ON : MAG_OFF;
            led      = magnetOn ? 1 : 0;

            printf("[TEST] Magnet %s  (PC_6 = %d)\n",
                   magnetOn ? "AN " : "AUS", magnet.read());
        }

        thread_sleep_for(10);
    }
}
