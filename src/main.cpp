/**
 * @file main.cpp
 * @brief Vial-Messanlage – Hauptprogramm
 *
 * Steuerung:
 *  - Mechanischer Knopf (USER_BUTTON): Toggle Start/Stop
 *  - Touchscreen (SPI, ILI9163/ST7735 kompatibel): Status + Start/Stop Taste
 *
 * Revolver-Slot-Sequenz (von Slot 0 an):
 *   [0] VIAL  [1] LOCH  [2] VIAL  [3] VIAL  [4] LOCH  (dann wiederholt)
 *
 * Die State Machine:
 *  IDLE → HOMING → ROTATE_TO_VIAL → LIFT_DOWN_PICK →
 *  GRAB → LIFT_UP → ROTATE_TO_HOLE → LIFT_DOWN_PLACE →
 *  RELEASE → LIFT_UP_EMPTY → CLOSE_LID → MEASURING →
 *  OPEN_LID → LIFT_DOWN_RETRIEVE → GRAB_AGAIN → LIFT_UP_RETURN →
 *  ROTATE_BACK → LIFT_DOWN_RETURN → RELEASE_HOME → LIFT_UP_FINAL →
 *  NEXT_VIAL_OR_DONE → DONE / IDLE (Nächster Zyklus)
 */

#include "mbed.h"
#include "PESBoardPinMap.h"
#include "DebounceIn.h"
#include "LiftMotor.h"
#include "Revolver.h"
#include "Lid.h"

// ============================================================
// TOUCHSCREEN  (SPI-Display, z.B. JoyIT 1.8" 160×128)
// Minimales SW-only Framebuffer-Display ohne externe Library.
// Pixel-Ausgabe via SPI, DC/CS/RST via DigitalOut.
// Befehle basieren auf ST7735 / ILI9163 Protokoll.
// ============================================================
#include "SPI.h"

namespace Display {
    // --- Pinbelegung anpassen ---
    SPI        spi(PB_5, PB_4, PB_3);   // MOSI, MISO, SCLK
    DigitalOut cs (PA_4, 1);
    DigitalOut dc (PA_3, 0);
    DigitalOut rst(PA_2, 1);

    // ST7735-Farben (RGB565)
    static constexpr uint16_t BLACK  = 0x0000;
    static constexpr uint16_t WHITE  = 0xFFFF;
    static constexpr uint16_t RED    = 0xF800;
    static constexpr uint16_t GREEN  = 0x07E0;
    static constexpr uint16_t BLUE   = 0x001F;
    static constexpr uint16_t YELLOW = 0xFFE0;
    static constexpr uint16_t GRAY   = 0x7BEF;

    static constexpr int W = 128;
    static constexpr int H = 160;

    inline void cmd(uint8_t c) {
        dc = 0; cs = 0;
        spi.write(c);
        cs = 1;
    }
    inline void data8(uint8_t d) {
        dc = 1; cs = 0;
        spi.write(d);
        cs = 1;
    }
    inline void data16(uint16_t d) {
        dc = 1; cs = 0;
        spi.write(d >> 8);
        spi.write(d & 0xFF);
        cs = 1;
    }

    void init() {
        spi.format(8, 0);
        spi.frequency(8000000);
        // Hardware-Reset
        rst = 0; thread_sleep_for(50);
        rst = 1; thread_sleep_for(120);
        // Software-Reset + minimal init (ST7735S)
        cmd(0x01); thread_sleep_for(150); // SWRESET
        cmd(0x11); thread_sleep_for(250); // SLPOUT
        cmd(0x3A); data8(0x05);           // COLMOD 16bit
        cmd(0x36); data8(0x00);           // MADCTL
        cmd(0x29);                         // DISPON
        // Bildschirm leer (schwarz)
        cmd(0x2A); data8(0); data8(0); data8(0); data8(W - 1);
        cmd(0x2B); data8(0); data8(0); data8(0); data8(H - 1);
        cmd(0x2C);
        dc = 1; cs = 0;
        for (int i = 0; i < W * H; i++) { spi.write(0x00); spi.write(0x00); }
        cs = 1;
    }

    void setWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
        cmd(0x2A); data8(0); data8(x0); data8(0); data8(x1);
        cmd(0x2B); data8(0); data8(y0); data8(0); data8(y1);
        cmd(0x2C);
    }

    void fillRect(int x, int y, int w, int h, uint16_t color) {
        setWindow(x, y, x + w - 1, y + h - 1);
        dc = 1; cs = 0;
        for (int i = 0; i < w * h; i++) {
            spi.write(color >> 8);
            spi.write(color & 0xFF);
        }
        cs = 1;
    }

    // 5×7 Mini-Font (ASCII 32..126), kein externes File
    static const uint8_t font5x7[][5] = {
        {0x00,0x00,0x00,0x00,0x00}, // ' '
        {0x00,0x00,0x5F,0x00,0x00}, // '!'
        {0x00,0x07,0x00,0x07,0x00}, // '"'
        {0x14,0x7F,0x14,0x7F,0x14}, // '#'
        {0x24,0x2A,0x7F,0x2A,0x12}, // '$'
        {0x23,0x13,0x08,0x64,0x62}, // '%'
        {0x36,0x49,0x55,0x22,0x50}, // '&'
        {0x00,0x05,0x03,0x00,0x00}, // '\''
        {0x00,0x1C,0x22,0x41,0x00}, // '('
        {0x00,0x41,0x22,0x1C,0x00}, // ')'
        {0x08,0x2A,0x1C,0x2A,0x08}, // '*'
        {0x08,0x08,0x3E,0x08,0x08}, // '+'
        {0x00,0x50,0x30,0x00,0x00}, // ','
        {0x08,0x08,0x08,0x08,0x08}, // '-'
        {0x00,0x60,0x60,0x00,0x00}, // '.'
        {0x20,0x10,0x08,0x04,0x02}, // '/'
        {0x3E,0x51,0x49,0x45,0x3E}, // '0'
        {0x00,0x42,0x7F,0x40,0x00}, // '1'
        {0x42,0x61,0x51,0x49,0x46}, // '2'
        {0x21,0x41,0x45,0x4B,0x31}, // '3'
        {0x18,0x14,0x12,0x7F,0x10}, // '4'
        {0x27,0x45,0x45,0x45,0x39}, // '5'
        {0x3C,0x4A,0x49,0x49,0x30}, // '6'
        {0x01,0x71,0x09,0x05,0x03}, // '7'
        {0x36,0x49,0x49,0x49,0x36}, // '8'
        {0x06,0x49,0x49,0x29,0x1E}, // '9'
        {0x00,0x36,0x36,0x00,0x00}, // ':'
        {0x00,0x56,0x36,0x00,0x00}, // ';'
        {0x00,0x08,0x14,0x22,0x41}, // '<'
        {0x14,0x14,0x14,0x14,0x14}, // '='
        {0x41,0x22,0x14,0x08,0x00}, // '>'
        {0x02,0x01,0x51,0x09,0x06}, // '?'
        {0x30,0x49,0x79,0x41,0x3E}, // '@'
        {0x7E,0x11,0x11,0x11,0x7E}, // 'A'
        {0x7F,0x49,0x49,0x49,0x36}, // 'B'
        {0x3E,0x41,0x41,0x41,0x22}, // 'C'
        {0x7F,0x41,0x41,0x22,0x1C}, // 'D'
        {0x7F,0x49,0x49,0x49,0x41}, // 'E'
        {0x7F,0x09,0x09,0x09,0x01}, // 'F'
        {0x3E,0x41,0x41,0x49,0x7A}, // 'G'
        {0x7F,0x08,0x08,0x08,0x7F}, // 'H'
        {0x00,0x41,0x7F,0x41,0x00}, // 'I'
        {0x20,0x40,0x41,0x3F,0x01}, // 'J'
        {0x7F,0x08,0x14,0x22,0x41}, // 'K'
        {0x7F,0x40,0x40,0x40,0x40}, // 'L'
        {0x7F,0x02,0x04,0x02,0x7F}, // 'M'
        {0x7F,0x04,0x08,0x10,0x7F}, // 'N'
        {0x3E,0x41,0x41,0x41,0x3E}, // 'O'
        {0x7F,0x09,0x09,0x09,0x06}, // 'P'
        {0x3E,0x41,0x51,0x21,0x5E}, // 'Q'
        {0x7F,0x09,0x19,0x29,0x46}, // 'R'
        {0x46,0x49,0x49,0x49,0x31}, // 'S'
        {0x01,0x01,0x7F,0x01,0x01}, // 'T'
        {0x3F,0x40,0x40,0x40,0x3F}, // 'U'
        {0x1F,0x20,0x40,0x20,0x1F}, // 'V'
        {0x7F,0x20,0x18,0x20,0x7F}, // 'W'
        {0x63,0x14,0x08,0x14,0x63}, // 'X'
        {0x03,0x04,0x78,0x04,0x03}, // 'Y'
        {0x61,0x51,0x49,0x45,0x43}, // 'Z'
        {0x00,0x00,0x7F,0x41,0x41}, // '['
        {0x02,0x04,0x08,0x10,0x20}, // '\\'
        {0x41,0x41,0x7F,0x00,0x00}, // ']'
        {0x04,0x02,0x01,0x02,0x04}, // '^'
        {0x40,0x40,0x40,0x40,0x40}, // '_'
        {0x00,0x01,0x02,0x04,0x00}, // '`'
        {0x20,0x54,0x54,0x54,0x78}, // 'a'
        {0x7F,0x48,0x44,0x44,0x38}, // 'b'
        {0x38,0x44,0x44,0x44,0x20}, // 'c'
        {0x38,0x44,0x44,0x48,0x7F}, // 'd'
        {0x38,0x54,0x54,0x54,0x18}, // 'e'
        {0x08,0x7E,0x09,0x01,0x02}, // 'f'
        {0x08,0x14,0x54,0x54,0x3C}, // 'g'
        {0x7F,0x08,0x04,0x04,0x78}, // 'h'
        {0x00,0x44,0x7D,0x40,0x00}, // 'i'
        {0x20,0x40,0x44,0x3D,0x00}, // 'j'
        {0x00,0x7F,0x10,0x28,0x44}, // 'k'
        {0x00,0x41,0x7F,0x40,0x00}, // 'l'
        {0x7C,0x04,0x18,0x04,0x78}, // 'm'
        {0x7C,0x08,0x04,0x04,0x78}, // 'n'
        {0x38,0x44,0x44,0x44,0x38}, // 'o'
        {0x7C,0x14,0x14,0x14,0x08}, // 'p'
        {0x08,0x14,0x14,0x18,0x7C}, // 'q'
        {0x7C,0x08,0x04,0x04,0x08}, // 'r'
        {0x48,0x54,0x54,0x54,0x20}, // 's'
        {0x04,0x3F,0x44,0x40,0x20}, // 't'
        {0x3C,0x40,0x40,0x40,0x7C}, // 'u'
        {0x1C,0x20,0x40,0x20,0x1C}, // 'v'
        {0x3C,0x40,0x30,0x40,0x3C}, // 'w'
        {0x44,0x28,0x10,0x28,0x44}, // 'x'
        {0x0C,0x50,0x50,0x50,0x3C}, // 'y'
        {0x44,0x64,0x54,0x4C,0x44}, // 'z'
    };

    void drawChar(int x, int y, char c, uint16_t fg, uint16_t bg) {
        if (c < 32 || c > 'z') c = ' ';
        const uint8_t* glyph = font5x7[c - 32];
        for (int col = 0; col < 5; col++) {
            for (int row = 0; row < 7; row++) {
                bool on = (glyph[col] >> row) & 1;
                fillRect(x + col, y + row, 1, 1, on ? fg : bg);
            }
        }
    }

    void drawText(int x, int y, const char* s, uint16_t fg = WHITE, uint16_t bg = BLACK) {
        while (*s) {
            drawChar(x, y, *s++, fg, bg);
            x += 6;
            if (x > W - 6) { x = 0; y += 9; }
        }
    }

    // Einfacher Button-Bereich prüfen (keine Touchscreen-IC – nur zur 
    // Demonstration der Layoutstruktur; echter Touch-IC-Read kommt via SPI)
    // Für echten XPT2046-Touch-IC: separate Klasse erforderlich.
    struct Rect { int x, y, w, h; };

    void drawButton(const Rect& r, const char* label, uint16_t bg = BLUE) {
        fillRect(r.x, r.y, r.w, r.h, bg);
        int tx = r.x + (r.w - static_cast<int>(strlen(label)) * 6) / 2;
        int ty = r.y + (r.h - 7) / 2;
        drawText(tx, ty, label, WHITE, bg);
    }
} // namespace Display

// ============================================================
// PIN MAP – Anpassen je nach tatsächlicher Verdrahtung!
// ============================================================
// Lift (Schrittmotor + TMC2209)
#define LIFT_STEP   PB_D0
#define LIFT_DIR    PB_D1
#define LIFT_EN     PB_D2
#define LIFT_TOP    PB_A0
#define LIFT_BOT    PB_A1
#define LIFT_MAGNET PB_D3

// Revolver (Schrittmotor + TMC2209)
// Zweiter Satz digitaler Ausgänge – z.B. über freie Pins
#define REV_STEP    PC_6    // PB_PWM_M1 kann als GPIO genutzt werden wenn kein DC-Motor
#define REV_DIR     PC_8
#define REV_EN      PB_12
#define REV_VIAL    PB_A2
#define REV_HOLE    PB_A3

// Deckel (DC-Motor M1)
#define LID_PWM     PB_PWM_M1
#define LID_ENCA    PB_ENC_A_M1
#define LID_ENCB    PB_ENC_B_M1
#define LID_CLOSE   PC_1    // Endschalter geschlossen
// #define LID_OPEN NC     // kein zweiter Sensor

// ============================================================
// STATE MACHINE
// ============================================================
enum class State {
    IDLE,
    HOMING,
    ROTATE_TO_VIAL,
    LIFT_DOWN_PICK,
    GRAB,
    LIFT_UP,
    ROTATE_TO_HOLE,
    LIFT_DOWN_PLACE,
    RELEASE,
    LIFT_UP_EMPTY,
    CLOSE_LID,
    MEASURING,
    OPEN_LID,
    LIFT_DOWN_RETRIEVE,
    GRAB_AGAIN,
    LIFT_UP_RETURN,
    ROTATE_BACK,
    LIFT_DOWN_RETURN,
    RELEASE_HOME,
    LIFT_UP_FINAL,
    DONE,
    ERROR
};

static const char* stateName(State s) {
    switch (s) {
        case State::IDLE:               return "IDLE";
        case State::HOMING:             return "HOMING";
        case State::ROTATE_TO_VIAL:     return "ROT->VIAL";
        case State::LIFT_DOWN_PICK:     return "LFT DN PK";
        case State::GRAB:               return "GRAB";
        case State::LIFT_UP:            return "LIFT UP";
        case State::ROTATE_TO_HOLE:     return "ROT->HOLE";
        case State::LIFT_DOWN_PLACE:    return "LFT DN PL";
        case State::RELEASE:            return "RELEASE";
        case State::LIFT_UP_EMPTY:      return "LIFT EMPT";
        case State::CLOSE_LID:          return "CLOSE LID";
        case State::MEASURING:          return "MEASURING";
        case State::OPEN_LID:           return "OPEN LID";
        case State::LIFT_DOWN_RETRIEVE: return "LFT DN RT";
        case State::GRAB_AGAIN:         return "GRAB AGN";
        case State::LIFT_UP_RETURN:     return "LIFT UP R";
        case State::ROTATE_BACK:        return "ROT BACK";
        case State::LIFT_DOWN_RETURN:   return "LFT DN RH";
        case State::RELEASE_HOME:       return "REL HOME";
        case State::LIFT_UP_FINAL:      return "LIFT FIN";
        case State::DONE:               return "DONE";
        case State::ERROR:              return "ERROR";
    }
    return "?";
}

// ============================================================
// GLOBALE VARIABLEN
// ============================================================
static volatile bool g_running = false;  // Start/Stop toggle
static volatile bool g_resetReq = false;

static State   g_state     = State::IDLE;
static State   g_prevState = State::IDLE;
static bool    g_entry     = true;
static int     g_timerMs   = 0;
static int     g_vialIndex = 0; // Aktuell gemessenes Vial (0-basiert)

static constexpr int MEASURE_MS    = 5000;
static constexpr int GRAB_WAIT_MS  = 300;
static constexpr int TIMEOUT_LIFT  = 6000;
static constexpr int TIMEOUT_REV   = 5000;
static constexpr int TIMEOUT_LID   = 3000;
static constexpr int MAIN_PERIOD   = 20;  // ms

// ============================================================
// DISPLAY-BUTTON LAYOUT
// ============================================================
static const Display::Rect BTN_STARTSTOP = {4, 130, 120, 24};

// ============================================================
// HARDWARE OBJEKTE
// ============================================================
DigitalOut         ledBusy(LED1, 0);
DebounceIn         userBtn(BUTTON1);

LiftMotor   lift(LIFT_STEP, LIFT_DIR, LIFT_EN,
                 LIFT_TOP,  LIFT_BOT,
                 LIFT_MAGNET);

Revolver    revolver(REV_STEP, REV_DIR, REV_EN,
                     REV_VIAL, REV_HOLE);

Lid         lid(LID_PWM, LID_ENCA, LID_ENCB,
                100.0f,   // gear ratio
                140.0f / 12.0f, // kn [rpm/V]
                LID_CLOSE);

// ============================================================
// HILFSFUNKTIONEN
// ============================================================
static void transitionTo(State next)
{
    printf("[SM] %s -> %s\n", stateName(g_state), stateName(next));
    g_prevState = g_state;
    g_state     = next;
    g_entry     = true;
    g_timerMs   = 0;
}

static void stopAll()
{
    lift.stop();
    lift.release();
    revolver.stop();
    lid.stopLid();
}

static void updateDisplay()
{
    // Status-Zeile
    Display::fillRect(0, 0, Display::W, 12, Display::BLACK);
    Display::drawText(2, 2, stateName(g_state),
                      g_running ? Display::GREEN : Display::YELLOW);

    // Vial-Zähler
    char buf[20];
    snprintf(buf, sizeof(buf), "Vial: %d", g_vialIndex + 1);
    Display::fillRect(0, 14, Display::W, 12, Display::BLACK);
    Display::drawText(2, 14, buf, Display::WHITE);

    // Timer-Balken während Messung
    if (g_state == State::MEASURING) {
        int pct = (g_timerMs * 120) / MEASURE_MS;
        Display::fillRect(4, 28, pct, 8, Display::BLUE);
        Display::fillRect(4 + pct, 28, 120 - pct, 8, Display::GRAY);
    }

    // Start/Stop Button
    Display::drawButton(BTN_STARTSTOP,
                        g_running ? "STOP" : "START",
                        g_running ? Display::RED : Display::GREEN);

    // Error
    if (g_state == State::ERROR) {
        Display::fillRect(0, 40, Display::W, 80, Display::BLACK);
        Display::drawText(4, 50, "! ERROR !", Display::RED);
        Display::drawText(4, 62, "Press STOP", Display::WHITE);
    }
}

// ============================================================
// TOGGLE-CALLBACK
// ============================================================
static void onButtonFall()
{
    g_running = !g_running;
    if (g_running) {
        g_resetReq = true;
    }
}

// ============================================================
// MAIN
// ============================================================
int main()
{
    // Display initialisieren
    Display::init();
    Display::fillRect(0, 0, Display::W, Display::H, Display::BLACK);
    Display::drawText(20, 70, "INIT...", Display::WHITE);

    // Button-Callback
    userBtn.fall(callback(&onButtonFall));

    // Haupt-Timer
    Timer mainTimer;
    mainTimer.start();

    printf("System bereit. Button oder Touch druecken.\n");

    updateDisplay();

    while (true) {
        mainTimer.reset();

        // --------------------------------------------------------
        // RUNNING
        // --------------------------------------------------------
        if (g_running) {
            ledBusy = 1;

            if (!g_entry)
                g_timerMs += MAIN_PERIOD;

            // ----------------------------------------------------
            // STATE MACHINE
            // ----------------------------------------------------
            switch (g_state) {

            // ---- IDLE -------------------------------------------
            case State::IDLE:
                if (g_entry) {
                    g_entry = false;
                    stopAll();
                    g_vialIndex = 0;
                }
                if (g_resetReq) {
                    g_resetReq = false;
                    transitionTo(State::HOMING);
                }
                break;

            // ---- HOMING -----------------------------------------
            case State::HOMING:
                if (g_entry) {
                    g_entry = false;
                    printf("Homing...\n");
                    lift.release();
                    // Revolver zur Vial-Position homen
                    revolver.turnCW();
                    // Lift nach oben homen
                    lift.moveUp();
                }
                // Warte bis beide Achsen in Referenzposition
                if (lift.isAtTop() && revolver.isAtVial()) {
                    lift.stop();
                    revolver.stop();
                    transitionTo(State::ROTATE_TO_VIAL);
                } else if (g_timerMs > TIMEOUT_LIFT + TIMEOUT_REV) {
                    transitionTo(State::ERROR);
                } else {
                    if (lift.isAtTop()) lift.stop();
                    if (revolver.isAtVial()) revolver.stop();
                }
                break;

            // ---- ROTATE_TO_VIAL ---------------------------------
            // Revolver steht bereits auf Vial (nach Homing oder Return)
            case State::ROTATE_TO_VIAL:
                if (g_entry) {
                    g_entry = false;
                    printf("Warte auf Vial-Position...\n");
                    // Wir sind bereits in Vial-Position nach Homing
                    // (bei nachfolgendem Zyklus: Revolver dreht zurück)
                }
                transitionTo(State::LIFT_DOWN_PICK);
                break;

            // ---- LIFT_DOWN_PICK ---------------------------------
            case State::LIFT_DOWN_PICK:
                if (g_entry) {
                    g_entry = false;
                    printf("Lift faehrt runter zum Vial...\n");
                    lift.moveDown();
                }
                if (lift.isAtBottom()) {
                    lift.stop();
                    transitionTo(State::GRAB);
                } else if (g_timerMs > TIMEOUT_LIFT) {
                    transitionTo(State::ERROR);
                }
                break;

            // ---- GRAB -------------------------------------------
            case State::GRAB:
                if (g_entry) {
                    g_entry = false;
                    printf("Vial greifen (Magnet AN)...\n");
                    lift.grab();
                }
                if (g_timerMs > GRAB_WAIT_MS) {
                    transitionTo(State::LIFT_UP);
                }
                break;

            // ---- LIFT_UP ----------------------------------------
            case State::LIFT_UP:
                if (g_entry) {
                    g_entry = false;
                    printf("Lift mit Vial hoch...\n");
                    lift.moveUp();
                }
                if (lift.isAtTop()) {
                    lift.stop();
                    transitionTo(State::ROTATE_TO_HOLE);
                } else if (g_timerMs > TIMEOUT_LIFT) {
                    transitionTo(State::ERROR);
                }
                break;

            // ---- ROTATE_TO_HOLE ---------------------------------
            // Revolver-Slot-Sequenz: VIAL–LOCH–VIAL–VIAL–LOCH–…
            // Für Vial 0 (Slot 0=VIAL, Slot 1=LOCH): 1 Slot CW
            // Für Vial 2 (Slot 2=VIAL, Slot 4=LOCH): 2 Slots CW
            // Generell: immer zum nächsten LOCH-Slot vorfahren.
            case State::ROTATE_TO_HOLE:
                if (g_entry) {
                    g_entry = false;
                    printf("Revolver zum Loch drehen...\n");
                    revolver.turnCW();
                }
                if (revolver.isAtHole()) {
                    revolver.stop();
                    transitionTo(State::LIFT_DOWN_PLACE);
                } else if (g_timerMs > TIMEOUT_REV) {
                    transitionTo(State::ERROR);
                }
                break;

            // ---- LIFT_DOWN_PLACE --------------------------------
            case State::LIFT_DOWN_PLACE:
                if (g_entry) {
                    g_entry = false;
                    printf("Vial durch Loch in Messanlage...\n");
                    lift.moveDown();
                }
                if (lift.isAtBottom()) {
                    lift.stop();
                    transitionTo(State::RELEASE);
                } else if (g_timerMs > TIMEOUT_LIFT) {
                    transitionTo(State::ERROR);
                }
                break;

            // ---- RELEASE ----------------------------------------
            case State::RELEASE:
                if (g_entry) {
                    g_entry = false;
                    printf("Vial loslassen (Magnet AUS)...\n");
                    lift.release();
                }
                if (g_timerMs > GRAB_WAIT_MS) {
                    transitionTo(State::LIFT_UP_EMPTY);
                }
                break;

            // ---- LIFT_UP_EMPTY ----------------------------------
            case State::LIFT_UP_EMPTY:
                if (g_entry) {
                    g_entry = false;
                    printf("Leeres Seil hoch...\n");
                    lift.moveUp();
                }
                if (lift.isAtTop()) {
                    lift.stop();
                    transitionTo(State::CLOSE_LID);
                } else if (g_timerMs > TIMEOUT_LIFT) {
                    transitionTo(State::ERROR);
                }
                break;

            // ---- CLOSE_LID --------------------------------------
            case State::CLOSE_LID:
                if (g_entry) {
                    g_entry = false;
                    printf("Deckel schliessen...\n");
                    lid.closeLid();
                }
                if (lid.isClosed()) {
                    lid.stopLid();
                    transitionTo(State::MEASURING);
                } else if (g_timerMs > TIMEOUT_LID) {
                    transitionTo(State::ERROR);
                }
                break;

            // ---- MEASURING --------------------------------------
            case State::MEASURING:
                if (g_entry) {
                    g_entry = false;
                    printf("Messung laeuft (%d ms)...\n", MEASURE_MS);
                    ledBusy = 0; // optional: LED aus = dunkel
                }
                if (g_timerMs >= MEASURE_MS) {
                    ledBusy = 1;
                    printf("Messung abgeschlossen.\n");
                    transitionTo(State::OPEN_LID);
                }
                break;

            // ---- OPEN_LID ---------------------------------------
            case State::OPEN_LID:
                if (g_entry) {
                    g_entry = false;
                    printf("Deckel oeffnen...\n");
                    lid.openLid();
                }
                if (lid.isOpen()) {
                    lid.stopLid();
                    transitionTo(State::LIFT_DOWN_RETRIEVE);
                } else if (g_timerMs > TIMEOUT_LID) {
                    transitionTo(State::ERROR);
                }
                break;

            // ---- LIFT_DOWN_RETRIEVE -----------------------------
            case State::LIFT_DOWN_RETRIEVE:
                if (g_entry) {
                    g_entry = false;
                    printf("Lift runter, Vial holen...\n");
                    lift.moveDown();
                }
                if (lift.isAtBottom()) {
                    lift.stop();
                    transitionTo(State::GRAB_AGAIN);
                } else if (g_timerMs > TIMEOUT_LIFT) {
                    transitionTo(State::ERROR);
                }
                break;

            // ---- GRAB_AGAIN -------------------------------------
            case State::GRAB_AGAIN:
                if (g_entry) {
                    g_entry = false;
                    printf("Vial wieder greifen...\n");
                    lift.grab();
                }
                if (g_timerMs > GRAB_WAIT_MS) {
                    transitionTo(State::LIFT_UP_RETURN);
                }
                break;

            // ---- LIFT_UP_RETURN ---------------------------------
            case State::LIFT_UP_RETURN:
                if (g_entry) {
                    g_entry = false;
                    printf("Lift mit Vial hoch (Return)...\n");
                    lift.moveUp();
                }
                if (lift.isAtTop()) {
                    lift.stop();
                    transitionTo(State::ROTATE_BACK);
                } else if (g_timerMs > TIMEOUT_LIFT) {
                    transitionTo(State::ERROR);
                }
                break;

            // ---- ROTATE_BACK ------------------------------------
            // Revolver in Gegenrichtung zurück zur ursprünglichen Vial-Position
            case State::ROTATE_BACK:
                if (g_entry) {
                    g_entry = false;
                    printf("Revolver zurueck zur Vial-Position...\n");
                    revolver.turnCCW(); // Gegenrichtung!
                }
                if (revolver.isAtVial()) {
                    revolver.stop();
                    transitionTo(State::LIFT_DOWN_RETURN);
                } else if (g_timerMs > TIMEOUT_REV) {
                    transitionTo(State::ERROR);
                }
                break;

            // ---- LIFT_DOWN_RETURN -------------------------------
            case State::LIFT_DOWN_RETURN:
                if (g_entry) {
                    g_entry = false;
                    printf("Vial in Startposition ablassen...\n");
                    lift.moveDown();
                }
                if (lift.isAtBottom()) {
                    lift.stop();
                    transitionTo(State::RELEASE_HOME);
                } else if (g_timerMs > TIMEOUT_LIFT) {
                    transitionTo(State::ERROR);
                }
                break;

            // ---- RELEASE_HOME -----------------------------------
            case State::RELEASE_HOME:
                if (g_entry) {
                    g_entry = false;
                    printf("Vial in Startposition loslassen...\n");
                    lift.release();
                }
                if (g_timerMs > GRAB_WAIT_MS) {
                    transitionTo(State::LIFT_UP_FINAL);
                }
                break;

            // ---- LIFT_UP_FINAL ----------------------------------
            case State::LIFT_UP_FINAL:
                if (g_entry) {
                    g_entry = false;
                    printf("Lift final hoch...\n");
                    lift.moveUp();
                }
                if (lift.isAtTop()) {
                    lift.stop();
                    transitionTo(State::DONE);
                } else if (g_timerMs > TIMEOUT_LIFT) {
                    transitionTo(State::ERROR);
                }
                break;

            // ---- DONE -------------------------------------------
            case State::DONE:
                if (g_entry) {
                    g_entry = false;
                    g_vialIndex++;
                    printf("Zyklus %d abgeschlossen. Revolver weiter...\n",
                           g_vialIndex);
                    stopAll();
                    // Revolver zum nächsten Vial-Slot fahren
                    // Slot-Sequenz: VIAL–LOCH–VIAL–VIAL–LOCH…
                    // Nach Ablegen sind wir wieder auf einem VIAL-Slot.
                    // Nächstes Vial ist je nach Sequenz 1 oder 2 Slots weiter CW.
                    // Einfache Implementierung: drehe CW bis nächste Vial-Lichtschranke.
                    revolver.turnCW();
                }
                // Warte auf nächste Vial-Position (überspringt Loch-Slots)
                if (revolver.isAtVial()) {
                    revolver.stop();
                    transitionTo(State::LIFT_DOWN_PICK); // direkt nächstes Vial
                }
                // Optional: nach N Vialen aufhören
                // if (g_vialIndex >= TOTAL_VIALS) { g_running = false; }
                break;

            // ---- ERROR ------------------------------------------
            case State::ERROR:
                if (g_entry) {
                    g_entry = false;
                    printf("!!! FEHLER !!! Alle Aktoren gestoppt.\n");
                    stopAll();
                    ledBusy = 0;
                }
                // Schnelles Blinken
                ledBusy = (g_timerMs % 400 < 200) ? 1 : 0;
                // Nur Reset durch erneutes Start/Stop möglich
                break;

            } // switch

        } else {
            // --------------------------------------------------------
            // NOT RUNNING
            // --------------------------------------------------------
            if (g_resetReq) {
                g_resetReq = false;
                // Reset State Machine
                g_state     = State::IDLE;
                g_prevState = State::IDLE;
                g_entry     = true;
                g_timerMs   = 0;
                g_vialIndex = 0;
                stopAll();
                ledBusy = 0;
                printf("System gestoppt und zurueckgesetzt.\n");
            }
            // LED langsam blinken
            ledBusy = (duration_cast<milliseconds>(
                           mainTimer.elapsed_time()).count() % 1000 < 100) ? 1 : 0;
        }

        // Display jede 5. Iteration aktualisieren (100 ms)
        static int dispCnt = 0;
        if (++dispCnt >= 5) {
            dispCnt = 0;
            updateDisplay();
        }

        // Timing
        int elapsed = duration_cast<milliseconds>(mainTimer.elapsed_time()).count();
        int sleep   = MAIN_PERIOD - elapsed;
        if (sleep > 0)
            thread_sleep_for(sleep);
        else
            printf("Warnung: Main-Task %d ms zu langsam!\n", -sleep);
    }
}
