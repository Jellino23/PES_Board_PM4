#include "mbed.h"
#include "PESBoardPinMap.h"
#include "DebounceIn.h"

// ================================================
// STATE MACHINE DEFINITION
// ================================================
enum RobotState {
    STATE_IDLE,               // 0 - Warte auf Start
    STATE_INIT,               // 1 - Initialisierung der Anlage
    STATE_ROPE_DOWN_PICKUP,   // 2 - Seil fährt runter zum Vial im Revolver
    STATE_GRAB_VIAL,          // 3 - Vial wird angehoben / gegriffen
    STATE_ROPE_UP_WITH_VIAL,  // 4 - Seil + Vial hoch
    STATE_REVOLVER_ROTATE,    // 5 - Revolver dreht zum Messloch
    STATE_LOWER_VIAL,         // 6 - Vial durch Loch runter in Messanlage
    STATE_RELEASE_VIAL,       // 7 - Seil lässt Vial los
    STATE_ROPE_UP_EMPTY,      // 8 - Leeres Seil fährt hoch
    STATE_CLOSE_LID,          // 9 - Deckel schliesst sich
    STATE_MEASURING,          // 10 - Messung läuft (im Dunkeln)
    STATE_OPEN_LID,           // 11 - Deckel öffnet sich
    STATE_ROPE_DOWN_RETRIEVE, // 12 - Seil fährt runter zum Vial holen
    STATE_GRAB_VIAL_AGAIN,    // 13 - Vial wieder greifen
    STATE_ROPE_UP_RETURN,     // 14 - Seil + Vial hoch zurück
    STATE_REVOLVER_RETURN,    // 15 - Revolver dreht zurück zur Startposition
    STATE_LOWER_VIAL_HOME,    // 16 - Vial zurück in Startposition ablassen
    STATE_RELEASE_VIAL_HOME,  // 17 - Vial loslassen in Startposition
    STATE_ROPE_UP_FINAL,      // 18 - Seil fährt final hoch
    STATE_DONE,               // 19 - Zyklus abgeschlossen
    STATE_ERROR               // 20 - Fehlerzustand
};

// ================================================
// CONSTANTS
// ================================================
const float ROPE_DOWN_SPEED     = 0.3f;  // m/s oder normiert 0..1
const float ROPE_UP_SPEED       = 0.3f;
const float REVOLVER_DEGREES    = 45.0f; // Grad pro Schritt
const float LID_OPEN_POS        = 1.0f;  // normiert
const float LID_CLOSED_POS      = 0.0f;
const int   MEASURE_TIME_MS     = 5000;  // 5 Sekunden Messzeit

// Timeouts (ms) – Sicherheitsgrenzen pro State
const int   TIMEOUT_ROPE_MS     = 4000;
const int   TIMEOUT_REVOLVER_MS = 3000;
const int   TIMEOUT_LID_MS      = 2000;
const int   TIMEOUT_MEASURE_MS  = MEASURE_TIME_MS + 500;

// ================================================
// GLOBAL STATE VARIABLES
// ================================================
bool do_execute_main_task = false;
bool do_reset_all_once    = false;

RobotState current_state  = STATE_IDLE;
RobotState previous_state = STATE_IDLE;
bool       state_entry     = true;  // true = erster Durchlauf in neuem State
int        state_timer_ms  = 0;     // Zeit im aktuellen State
bool       vial_in_hand    = false; // Tracking ob Seil Vial hält

// ================================================
// HARDWARE OBJECTS (Platzhalter – anpassen!)
// ================================================
DebounceIn user_button(BUTTON1);
void toggle_do_execute_main_fcn();

DigitalOut user_led(LED1);
DigitalOut led_busy(PB_9);          // Leuchtet wenn Anlage aktiv

// Endschalter (DigitalIn mit PullUp, active LOW)
DigitalIn  sensor_rope_top(PA_0);       // Seil oben
DigitalIn  sensor_rope_bottom(PA_1);    // Seil unten / Vial erreicht
DigitalIn  sensor_lid_open(PA_4);       // Deckel offen
DigitalIn  sensor_lid_closed(PB_0);     // Deckel geschlossen
DigitalIn  sensor_revolver_pos(PC_1);   // Revolver in Position

// Aktoren (PwmOut oder DigitalOut – je nach Hardware)
PwmOut     motor_rope(PB_4);        // Seilmotor
PwmOut     motor_revolver(PB_5);    // Revolvermotor
PwmOut     actuator_lid(PB_6);      // Deckelantrieb
DigitalOut gripper(PC_8);           // Greifer / Klemme am Seil

// ================================================
// HELPER FUNCTIONS
// ================================================

// Gibt den State-Namen als String zurück (für Debug-Output)
const char* state_name(RobotState s) {
    switch(s) {
        case STATE_IDLE:               return "IDLE";
        case STATE_INIT:               return "INIT";
        case STATE_ROPE_DOWN_PICKUP:   return "ROPE_DOWN_PICKUP";
        case STATE_GRAB_VIAL:          return "GRAB_VIAL";
        case STATE_ROPE_UP_WITH_VIAL:  return "ROPE_UP_WITH_VIAL";
        case STATE_REVOLVER_ROTATE:    return "REVOLVER_ROTATE";
        case STATE_LOWER_VIAL:         return "LOWER_VIAL";
        case STATE_RELEASE_VIAL:       return "RELEASE_VIAL";
        case STATE_ROPE_UP_EMPTY:      return "ROPE_UP_EMPTY";
        case STATE_CLOSE_LID:          return "CLOSE_LID";
        case STATE_MEASURING:          return "MEASURING";
        case STATE_OPEN_LID:           return "OPEN_LID";
        case STATE_ROPE_DOWN_RETRIEVE: return "ROPE_DOWN_RETRIEVE";
        case STATE_GRAB_VIAL_AGAIN:    return "GRAB_VIAL_AGAIN";
        case STATE_ROPE_UP_RETURN:     return "ROPE_UP_RETURN";
        case STATE_REVOLVER_RETURN:    return "REVOLVER_RETURN";
        case STATE_LOWER_VIAL_HOME:    return "LOWER_VIAL_HOME";
        case STATE_RELEASE_VIAL_HOME:  return "RELEASE_VIAL_HOME";
        case STATE_ROPE_UP_FINAL:      return "ROPE_UP_FINAL";
        case STATE_DONE:               return "DONE";
        case STATE_ERROR:              return "ERROR";
        default:                       return "UNKNOWN";
    }
}

// Zustandswechsel mit Debug-Ausgabe
void transition_to(RobotState next) {
    printf("State: %s -> %s\n", state_name(current_state), state_name(next));
    previous_state = current_state;
    current_state  = next;
    state_entry    = true;
    state_timer_ms = 0;
}

// Alle Aktoren stoppen
void stop_all_actuators() {
    motor_rope.write(0.5f);      // 0.5 = Stop bei H-Brücke (anpassen!)
    motor_revolver.write(0.5f);
    actuator_lid.write(0.5f);
}

// ================================================
// MAIN
// ================================================
int main()
{
    user_button.fall(&toggle_do_execute_main_fcn);

    const int main_task_period_ms = 20;
    Timer main_task_timer;

    // Sensor Pull-Ups aktivieren
    sensor_rope_top.mode(PullUp);
    sensor_rope_bottom.mode(PullUp);
    sensor_lid_open.mode(PullUp);
    sensor_lid_closed.mode(PullUp);
    sensor_revolver_pos.mode(PullUp);

    // PWM Frequenz setzen
    motor_rope.period(0.02f);
    motor_revolver.period(0.02f);
    actuator_lid.period(0.02f);

    stop_all_actuators();
    gripper = 0;

    main_task_timer.start();

    printf("System ready. Press blue button to start.\n");

    while (true) {
        main_task_timer.reset();

        // ============================================
        // MAIN TASK ACTIVE
        // ============================================
        if (do_execute_main_task) {
            led_busy = 1;

            // State-Timer hochzählen (20 ms pro Zyklus)
            if (!state_entry)
                state_timer_ms += main_task_period_ms;

            // ==========================================
            // STATE MACHINE
            // ==========================================
            switch (current_state) {

                // ------------------------------------------
                case STATE_IDLE:
                    if (state_entry) {
                        state_entry = false;
                        printf("Waiting for start command...\n");
                        stop_all_actuators();
                    }
                    // Automatisch zur Initialisierung
                    transition_to(STATE_INIT);
                    break;

                // ------------------------------------------
                case STATE_INIT:
                    if (state_entry) {
                        state_entry = false;
                        printf("Initializing system...\n");
                        stop_all_actuators();
                        gripper = 0;
                    }
                    // Prüfe ob Seil oben und Deckel offen
                    if (sensor_rope_top.read() == 0 && sensor_lid_open.read() == 0) {
                        printf("Init OK: Rope at top, lid open.\n");
                        transition_to(STATE_ROPE_DOWN_PICKUP);
                    } else if (state_timer_ms > TIMEOUT_ROPE_MS) {
                        printf("ERROR: Init timeout!\n");
                        transition_to(STATE_ERROR);
                    } else {
                        // Fahre Seil hoch und öffne Deckel
                        if (sensor_rope_top.read() != 0)
                            motor_rope.write(0.5f + ROPE_UP_SPEED * 0.5f);
                        if (sensor_lid_open.read() != 0)
                            actuator_lid.write(LID_OPEN_POS);
                    }
                    break;

                // ------------------------------------------
                case STATE_ROPE_DOWN_PICKUP:
                    if (state_entry) {
                        state_entry = false;
                        printf("Lowering rope to vial...\n");
                        motor_rope.write(0.5f - ROPE_DOWN_SPEED * 0.5f); // Seil runter
                    }
                    if (sensor_rope_bottom.read() == 0) {
                        motor_rope.write(0.5f); // Stop
                        transition_to(STATE_GRAB_VIAL);
                    } else if (state_timer_ms > TIMEOUT_ROPE_MS) {
                        transition_to(STATE_ERROR);
                    }
                    break;

                // ------------------------------------------
                case STATE_GRAB_VIAL:
                    if (state_entry) {
                        state_entry = false;
                        printf("Grabbing vial...\n");
                        gripper = 1; // Greifer schliessen
                    }
                    // Kurze Wartezeit für den Greifer
                    if (state_timer_ms > 500) {
                        vial_in_hand = true;
                        transition_to(STATE_ROPE_UP_WITH_VIAL);
                    }
                    break;

                // ------------------------------------------
                case STATE_ROPE_UP_WITH_VIAL:
                    if (state_entry) {
                        state_entry = false;
                        printf("Lifting vial up...\n");
                        motor_rope.write(0.5f + ROPE_UP_SPEED * 0.5f); // Seil hoch
                    }
                    if (sensor_rope_top.read() == 0) {
                        motor_rope.write(0.5f);
                        transition_to(STATE_REVOLVER_ROTATE);
                    } else if (state_timer_ms > TIMEOUT_ROPE_MS) {
                        transition_to(STATE_ERROR);
                    }
                    break;

                // ------------------------------------------
                case STATE_REVOLVER_ROTATE:
                    if (state_entry) {
                        state_entry = false;
                        printf("Rotating revolver %.1f degrees...\n", REVOLVER_DEGREES);
                        motor_revolver.write(0.5f + 0.2f); // Vorwärts drehen
                    }
                    if (sensor_revolver_pos.read() == 0) {
                        motor_revolver.write(0.5f);
                        transition_to(STATE_LOWER_VIAL);
                    } else if (state_timer_ms > TIMEOUT_REVOLVER_MS) {
                        transition_to(STATE_ERROR);
                    }
                    break;

                // ------------------------------------------
                case STATE_LOWER_VIAL:
                    if (state_entry) {
                        state_entry = false;
                        printf("Lowering vial into measurement chamber...\n");
                        motor_rope.write(0.5f - ROPE_DOWN_SPEED * 0.5f);
                    }
                    if (sensor_rope_bottom.read() == 0) {
                        motor_rope.write(0.5f);
                        transition_to(STATE_RELEASE_VIAL);
                    } else if (state_timer_ms > TIMEOUT_ROPE_MS) {
                        transition_to(STATE_ERROR);
                    }
                    break;

                // ------------------------------------------
                case STATE_RELEASE_VIAL:
                    if (state_entry) {
                        state_entry = false;
                        printf("Releasing vial...\n");
                        gripper = 0; // Greifer öffnen
                    }
                    if (state_timer_ms > 500) {
                        vial_in_hand = false;
                        transition_to(STATE_ROPE_UP_EMPTY);
                    }
                    break;

                // ------------------------------------------
                case STATE_ROPE_UP_EMPTY:
                    if (state_entry) {
                        state_entry = false;
                        printf("Pulling rope back up (empty)...\n");
                        motor_rope.write(0.5f + ROPE_UP_SPEED * 0.5f);
                    }
                    if (sensor_rope_top.read() == 0) {
                        motor_rope.write(0.5f);
                        transition_to(STATE_CLOSE_LID);
                    } else if (state_timer_ms > TIMEOUT_ROPE_MS) {
                        transition_to(STATE_ERROR);
                    }
                    break;

                // ------------------------------------------
                case STATE_CLOSE_LID:
                    if (state_entry) {
                        state_entry = false;
                        printf("Closing lid for measurement...\n");
                        actuator_lid.write(LID_CLOSED_POS);
                    }
                    if (sensor_lid_closed.read() == 0) {
                        actuator_lid.write(0.5f);
                        transition_to(STATE_MEASURING);
                    } else if (state_timer_ms > TIMEOUT_LID_MS) {
                        transition_to(STATE_ERROR);
                    }
                    break;

                // ------------------------------------------
                case STATE_MEASURING:
                    if (state_entry) {
                        state_entry = false;
                        printf("Measuring... (darkness mode, %d ms)\n", MEASURE_TIME_MS);
                        user_led = 0; // LED aus = dunkel!
                    }
                    if (state_timer_ms >= MEASURE_TIME_MS) {
                        printf("Measurement complete.\n");
                        user_led = 1;
                        transition_to(STATE_OPEN_LID);
                    }
                    break;

                // ------------------------------------------
                case STATE_OPEN_LID:
                    if (state_entry) {
                        state_entry = false;
                        printf("Opening lid...\n");
                        actuator_lid.write(LID_OPEN_POS);
                    }
                    if (sensor_lid_open.read() == 0) {
                        actuator_lid.write(0.5f);
                        transition_to(STATE_ROPE_DOWN_RETRIEVE);
                    } else if (state_timer_ms > TIMEOUT_LID_MS) {
                        transition_to(STATE_ERROR);
                    }
                    break;

                // ------------------------------------------
                case STATE_ROPE_DOWN_RETRIEVE:
                    if (state_entry) {
                        state_entry = false;
                        printf("Lowering rope to retrieve vial...\n");
                        motor_rope.write(0.5f - ROPE_DOWN_SPEED * 0.5f);
                    }
                    if (sensor_rope_bottom.read() == 0) {
                        motor_rope.write(0.5f);
                        transition_to(STATE_GRAB_VIAL_AGAIN);
                    } else if (state_timer_ms > TIMEOUT_ROPE_MS) {
                        transition_to(STATE_ERROR);
                    }
                    break;

                // ------------------------------------------
                case STATE_GRAB_VIAL_AGAIN:
                    if (state_entry) {
                        state_entry = false;
                        printf("Grabbing vial for return...\n");
                        gripper = 1;
                    }
                    if (state_timer_ms > 500) {
                        vial_in_hand = true;
                        transition_to(STATE_ROPE_UP_RETURN);
                    }
                    break;

                // ------------------------------------------
                case STATE_ROPE_UP_RETURN:
                    if (state_entry) {
                        state_entry = false;
                        printf("Lifting vial for return...\n");
                        motor_rope.write(0.5f + ROPE_UP_SPEED * 0.5f);
                    }
                    if (sensor_rope_top.read() == 0) {
                        motor_rope.write(0.5f);
                        transition_to(STATE_REVOLVER_RETURN);
                    } else if (state_timer_ms > TIMEOUT_ROPE_MS) {
                        transition_to(STATE_ERROR);
                    }
                    break;

                // ------------------------------------------
                case STATE_REVOLVER_RETURN:
                    if (state_entry) {
                        state_entry = false;
                        printf("Rotating revolver back %.1f degrees...\n", REVOLVER_DEGREES);
                        motor_revolver.write(0.5f - 0.2f); // Rückwärts
                    }
                    if (sensor_revolver_pos.read() == 0) {
                        motor_revolver.write(0.5f);
                        transition_to(STATE_LOWER_VIAL_HOME);
                    } else if (state_timer_ms > TIMEOUT_REVOLVER_MS) {
                        transition_to(STATE_ERROR);
                    }
                    break;

                // ------------------------------------------
                case STATE_LOWER_VIAL_HOME:
                    if (state_entry) {
                        state_entry = false;
                        printf("Lowering vial back to start position...\n");
                        motor_rope.write(0.5f - ROPE_DOWN_SPEED * 0.5f);
                    }
                    if (sensor_rope_bottom.read() == 0) {
                        motor_rope.write(0.5f);
                        transition_to(STATE_RELEASE_VIAL_HOME);
                    } else if (state_timer_ms > TIMEOUT_ROPE_MS) {
                        transition_to(STATE_ERROR);
                    }
                    break;

                // ------------------------------------------
                case STATE_RELEASE_VIAL_HOME:
                    if (state_entry) {
                        state_entry = false;
                        printf("Releasing vial at home position...\n");
                        gripper = 0;
                    }
                    if (state_timer_ms > 500) {
                        vial_in_hand = false;
                        transition_to(STATE_ROPE_UP_FINAL);
                    }
                    break;

                // ------------------------------------------
                case STATE_ROPE_UP_FINAL:
                    if (state_entry) {
                        state_entry = false;
                        printf("Returning rope to top...\n");
                        motor_rope.write(0.5f + ROPE_UP_SPEED * 0.5f);
                    }
                    if (sensor_rope_top.read() == 0) {
                        motor_rope.write(0.5f);
                        transition_to(STATE_DONE);
                    } else if (state_timer_ms > TIMEOUT_ROPE_MS) {
                        transition_to(STATE_ERROR);
                    }
                    break;

                // ------------------------------------------
                case STATE_DONE:
                    if (state_entry) {
                        state_entry = false;
                        printf("Cycle complete! Press button to run again.\n");
                        stop_all_actuators();
                    }
                    // Taste nochmals drücken → neuer Zyklus
                    // (do_execute_main_task wird durch Button auf false gesetzt,
                    //  beim nächsten Start beginnt es wieder bei IDLE)
                    break;

                // ------------------------------------------
                case STATE_ERROR:
                    if (state_entry) {
                        state_entry = false;
                        printf("!!! ERROR STATE !!! All actuators stopped.\n");
                        stop_all_actuators();
                        gripper = 0;
                        led_busy = 0;
                    }
                    // Schnelles Blinken als Fehlerindikator
                    if (state_timer_ms % 200 < 100)
                        user_led = 1;
                    else
                        user_led = 0;
                    // Nur Reset durch Button-Neustart möglich
                    break;

            } // end switch

        } else {
            // ============================================
            // MAIN TASK INACTIVE (Button nicht gedrückt)
            // ============================================
            if (do_reset_all_once) {
                do_reset_all_once = false;
                // Reset State Machine
                current_state  = STATE_IDLE;
                previous_state = STATE_IDLE;
                state_entry    = true;
                state_timer_ms = 0;
                vial_in_hand   = false;
                stop_all_actuators();
                gripper  = 0;
                led_busy = 0;
                printf("System reset. Ready.\n");
            }
            user_led = !user_led; // Langsames Blinken im Idle
        }

        // Timing
        int elapsed = duration_cast<milliseconds>(main_task_timer.elapsed_time()).count();
        if (main_task_period_ms - elapsed < 0)
            printf("Warning: Main task took longer than %d ms\n", main_task_period_ms);
        else
            thread_sleep_for(main_task_period_ms - elapsed);
    }
}

void toggle_do_execute_main_fcn()
{
    do_execute_main_task = !do_execute_main_task;
    if (do_execute_main_task)
        do_reset_all_once = true;
}
