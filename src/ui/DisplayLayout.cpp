#include "DisplayLayout.h"
#include "StateMachine.h"   // für State + stateName()
#include <cstdio>

// --------------------------------------------------------------------------
// Button-Positionen (x, y, w, h) – anpassen wenn nötig
// --------------------------------------------------------------------------
const DisplayLayout::Rect DisplayLayout::BTN_STARTSTOP = {4, 130, 120, 24};

// --------------------------------------------------------------------------
DisplayLayout::DisplayLayout(Display& display)
    : m_disp(display)
{}

void DisplayLayout::drawSplash()
{
    m_disp.fillScreen(Display::BLACK);
    m_disp.drawText(20, 60, "VIAL", Display::WHITE);
    m_disp.drawText(10, 72, "ROBOT", Display::BLUE);
    m_disp.drawText(24, 90, "INIT...", Display::GRAY);
}

// --------------------------------------------------------------------------
void DisplayLayout::update(State state, int vialIndex, int timerMs,
                           bool running, int measureMs)
{
    drawHeader(state, running);
    drawVialCounter(vialIndex);

    if (state == State::MEASURING)
        drawProgressBar(timerMs, measureMs);
    else
        m_disp.fillRect(4, 28, Display::WIDTH - 8, 8, Display::BLACK);

    if (state == State::ERROR)
        drawErrorOverlay();

    drawStartStopButton(running);
}

// --------------------------------------------------------------------------
void DisplayLayout::drawHeader(State state, bool running)
{
    m_disp.fillRect(0, 0, Display::WIDTH, 12, Display::BLACK);
    uint16_t color = running ? Display::GREEN : Display::YELLOW;
    m_disp.drawText(2, 2, stateName(state), color);
}

void DisplayLayout::drawVialCounter(int vialIndex)
{
    char buf[20];
    snprintf(buf, sizeof(buf), "Vial: %d", vialIndex + 1);
    m_disp.fillRect(0, 14, Display::WIDTH, 12, Display::BLACK);
    m_disp.drawText(2, 14, buf, Display::WHITE);
}

void DisplayLayout::drawProgressBar(int timerMs, int measureMs)
{
    if (measureMs <= 0) return;
    int filled = (timerMs * (Display::WIDTH - 8)) / measureMs;
    if (filled > Display::WIDTH - 8) filled = Display::WIDTH - 8;
    m_disp.fillRect(4,          28, filled,                      8, Display::BLUE);
    m_disp.fillRect(4 + filled, 28, Display::WIDTH - 8 - filled, 8, Display::GRAY);
}

void DisplayLayout::drawStartStopButton(bool running)
{
    drawButton(BTN_STARTSTOP,
               running ? "STOP" : "START",
               running ? Display::RED : Display::GREEN);
}

void DisplayLayout::drawErrorOverlay()
{
    m_disp.fillRect(0, 40, Display::WIDTH, 80, Display::BLACK);
    m_disp.drawText(4,  50, "! ERROR !",  Display::RED);
    m_disp.drawText(4,  62, "Press STOP", Display::WHITE);
}

void DisplayLayout::drawButton(const Rect& r, const char* label, uint16_t bg)
{
    m_disp.fillRect(r.x, r.y, r.w, r.h, bg);
    int tx = r.x + (r.w - static_cast<int>(strlen(label)) * 6) / 2;
    int ty = r.y + (r.h - 7) / 2;
    m_disp.drawText(tx, ty, label, Display::WHITE, bg);
}
