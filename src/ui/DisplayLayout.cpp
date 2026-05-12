#include "DisplayLayout.h"
#include "StateMachine.h"
#include <cstdio>
#include <cstring>

// ============================================================
// Layout-Konstanten  (Landscape 160×128)
// ============================================================
//  Y=  4..17  Status-Text  (scale=2, 14px hoch)
//  Y= 22..31  Fortschrittsbalken (10px, nur MEASURING)
//  Y= 36..49  Temperatur   (scale=2)
//  Y= 54..67  Luftf.       (scale=2)
//  Y= 76..123 START/STOP-Knopf

const DisplayLayout::Rect DisplayLayout::BTN_STARTSTOP = {4, 76, 152, 48};

// ============================================================
DisplayLayout::DisplayLayout(Display& display)
    : m_disp(display)
{}

// ============================================================
void DisplayLayout::drawSplash()
{
    m_disp.fillScreen(Display::BLACK);
    // "VIAL" zentriert (4 Zeichen × 12px = 48px)
    m_disp.drawText((Display::WIDTH - 48) / 2,  30, "VIAL",  Display::WHITE, Display::BLACK, 2);
    // "ROBOT" zentriert (5 Zeichen × 12px = 60px)
    m_disp.drawText((Display::WIDTH - 60) / 2,  52, "ROBOT", Display::WHITE, Display::BLACK, 2);
    m_disp.drawText((Display::WIDTH - 42) / 2,  80, "INIT...", Display::GRAY,  Display::BLACK, 1);
}

// ============================================================
void DisplayLayout::update(State state, int timerMs, bool running, int measureMs,
                           float temperature, float humidity, bool sensorValid)
{
    // Hintergrund komplett leeren
    m_disp.fillScreen(Display::BLACK);

    drawStatusText(state, running);

    if (state == State::MEASURING)
        drawProgressBar(timerMs, measureMs);

    drawSensor(temperature, humidity, sensorValid);
    drawStartStopButton(running);
}

// ============================================================
void DisplayLayout::drawStatusText(State state, bool running)
{
    const char* text;
    uint16_t    color;

    if (state == State::ERROR) {
        text  = "ERROR";
        color = Display::RED;
    } else if (!running) {
        text  = "IDLE";
        color = Display::WHITE;
    } else if (state == State::MEASURING) {
        text  = "MEASURING";
        color = Display::WHITE;
    } else {
        text  = "PICK+PLACE";
        color = Display::WHITE;
    }

    // Zentriert bei scale=2 (jedes Zeichen 12px breit)
    int tx = (Display::WIDTH - static_cast<int>(strlen(text)) * 12) / 2;
    m_disp.drawText(tx, 4, text, color, Display::BLACK, 2);
}

// ============================================================
void DisplayLayout::updateProgress(int timerMs, int measureMs)
{
    // Nur Balkenbereich leeren und neu zeichnen – kein Flicker auf Rest
    m_disp.fillRect(4, 22, Display::WIDTH - 8, 18, Display::BLACK);
    drawProgressBar(timerMs, measureMs);
}

// ============================================================
void DisplayLayout::drawProgressBar(int timerMs, int measureMs)
{
    if (measureMs <= 0) return;

    int total  = Display::WIDTH - 8;
    int filled = (timerMs * total) / measureMs;
    if (filled > total) filled = total;
    if (filled < 0)     filled = 0;

    m_disp.fillRect(4,          22, filled,         10, Display::BLUE);
    m_disp.fillRect(4 + filled, 22, total - filled, 10, Display::GRAY);

    // Prozent zentriert auf Balken (scale=1, 7px hoch → passt in 10px)
    char pct[16];
    int  pctVal = (timerMs * 100) / measureMs;
    if (pctVal > 100) pctVal = 100;
    snprintf(pct, sizeof(pct), "%d%%", pctVal);
    int tx = 4 + (total - static_cast<int>(strlen(pct)) * 6) / 2;
    m_disp.drawText(tx, 23, pct, Display::WHITE, Display::BLACK, 1);
}

// ============================================================
void DisplayLayout::drawSensor(float temperature, float humidity, bool valid)
{
    char tBuf[12];
    char hBuf[12];

    if (valid) {
        snprintf(tBuf, sizeof(tBuf), "T:%2d C", static_cast<int>(temperature));
        snprintf(hBuf, sizeof(hBuf), "H:%2d%%", static_cast<int>(humidity));
    } else {
        snprintf(tBuf, sizeof(tBuf), "T: -- C");
        snprintf(hBuf, sizeof(hBuf), "H: -- %%");
    }

    m_disp.drawText(4, 36, tBuf, Display::WHITE, Display::BLACK, 2);
    m_disp.drawText(4, 54, hBuf, Display::WHITE, Display::BLACK, 2);
}

// ============================================================
void DisplayLayout::drawStartStopButton(bool running)
{
    drawButton(BTN_STARTSTOP,
               running ? "STOP" : "START",
               running ? Display::RED : Display::GREEN,
               2);
}

// ============================================================
void DisplayLayout::drawButton(const Rect& r, const char* label, uint16_t bg, int textScale)
{
    m_disp.fillRect(r.x, r.y, r.w, r.h, bg);
    int charW = 6 * textScale;
    int charH = 7 * textScale;
    int tx = r.x + (r.w - static_cast<int>(strlen(label)) * charW) / 2;
    int ty = r.y + (r.h - charH) / 2;
    m_disp.drawText(tx, ty, label, Display::WHITE, bg, textScale);
}
