#include "DisplayLayout.h"
#include "StateMachine.h"
#include <cstdio>
#include <cstring>

// ============================================================
// Layout-Koordinaten  (Landscape 160×128)
// ============================================================
//  Y=  4..17  Status-Text       scale=2  "PICK+PLACE" / "MEASURING" / ...
//  Y= 20..29  Fortschrittsbalken 10px    nur MEASURING
//  Y= 32..38  Sensor T+H         scale=1 "T:23C  H:55%"
//  Y= 43..56  Vial-Zähler        scale=2 "Vial: 3"
//  Y= 60..120 START/STOP-Knopf   h=60

const DisplayLayout::Rect DisplayLayout::BTN_STARTSTOP = {4, 60, 152, 60};

// ============================================================
DisplayLayout::DisplayLayout(Display& display)
    : m_disp(display)
{}

// ============================================================
void DisplayLayout::drawSplash()
{
    m_disp.fillScreen(Display::BLACK);
    m_disp.drawText((Display::WIDTH - 48) / 2, 30, "VIAL",  Display::WHITE, Display::BLACK, 2);
    m_disp.drawText((Display::WIDTH - 60) / 2, 52, "ROBOT", Display::WHITE, Display::BLACK, 2);
    m_disp.drawText((Display::WIDTH - 42) / 2, 80, "INIT...", Display::GRAY, Display::BLACK, 1);
}

// ============================================================
void DisplayLayout::update(State state, int vialIndex, int timerMs, bool running,
                           int measureMs, float temperature, float humidity,
                           bool sensorValid)
{
    m_disp.fillScreen(Display::BLACK);
    drawStatusText(state, running);
    if (state == State::MEASURING)
        drawProgressBar(timerMs, measureMs);
    drawSensor(temperature, humidity, sensorValid);
    drawVialCounter(vialIndex);
    drawStartStopButton(running);
}

// ============================================================
void DisplayLayout::updateProgress(int timerMs, int measureMs)
{
    m_disp.fillRect(4, 20, Display::WIDTH - 8, 18, Display::BLACK);
    drawProgressBar(timerMs, measureMs);
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

    int tx = (Display::WIDTH - static_cast<int>(strlen(text)) * 12) / 2;
    m_disp.drawText(tx, 4, text, color, Display::BLACK, 2);
}

// ============================================================
void DisplayLayout::drawProgressBar(int timerMs, int measureMs)
{
    if (measureMs <= 0) return;

    int total  = Display::WIDTH - 8;
    int filled = (timerMs * total) / measureMs;
    if (filled > total) filled = total;
    if (filled < 0)     filled = 0;

    m_disp.fillRect(4,          20, filled,         10, Display::BLUE);
    m_disp.fillRect(4 + filled, 20, total - filled, 10, Display::GRAY);

    char pct[16];
    int  pctVal = (timerMs * 100) / measureMs;
    if (pctVal > 100) pctVal = 100;
    snprintf(pct, sizeof(pct), "%d%%", pctVal);
    int tx = 4 + (total - static_cast<int>(strlen(pct)) * 6) / 2;
    m_disp.drawText(tx, 21, pct, Display::WHITE, Display::BLACK, 1);
}

// ============================================================
void DisplayLayout::drawSensor(float temperature, float humidity, bool valid)
{
    char buf[24];
    if (valid)
        snprintf(buf, sizeof(buf), "T:%2dC  H:%2d%%",
                 static_cast<int>(temperature), static_cast<int>(humidity));
    else
        snprintf(buf, sizeof(buf), "T:--C  H:--%%" );

    m_disp.drawText(4, 32, buf, Display::WHITE, Display::BLACK, 1);
}

// ============================================================
void DisplayLayout::drawVialCounter(int vialIndex)
{
    char buf[24];
    snprintf(buf, sizeof(buf), "Vial: %d", vialIndex + 1);
    int tx = (Display::WIDTH - static_cast<int>(strlen(buf)) * 12) / 2;
    m_disp.drawText(tx, 43, buf, Display::WHITE, Display::BLACK, 2);
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
