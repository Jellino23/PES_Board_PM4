#pragma once
#include "Display.h"

// Vorwärts-Deklaration um zirkuläre Abhängigkeiten zu vermeiden
enum class State;

/**
 * @file DisplayLayout.h
 * @brief Applikations-spezifisches Display-Layout.
 *
 * Verantwortlich für:
 *  - Aufbau der Bildschirm-Bereiche (Header, Status, Fortschritt, Button)
 *  - Übersetzung von Applikationszustand → Pixel
 *
 * Kennt den Display-Treiber und den State-Typ,
 * aber keine Hardware-Klassen (LiftMotor, Revolver, …).
 */
class DisplayLayout {
public:
    struct Rect { int x, y, w, h; };

    // Button-Zonen für Touch-Abfrage (öffentlich für main.cpp / XPT2046)
    static const Rect BTN_STARTSTOP;

    explicit DisplayLayout(Display& display);

    /**
     * @brief Vollständiges Neuzeichnen des Bildschirms.
     * @param state       Aktueller Maschinenzustand
     * @param vialIndex   Aktuell gemessenes Vial (0-basiert)
     * @param timerMs     Verstrichene Zeit im aktuellen Zustand [ms]
     * @param running     true = Anlage läuft
     * @param measureMs   Gesamte Messdauer [ms] (für Fortschrittsbalken)
     */
    void update(State state, int vialIndex, int timerMs,
                bool running, int measureMs);

    void drawButton(const Rect& r, const char* label, uint16_t bg);

    /**
     * @brief Splash-Screen beim Start.
     */
    void drawSplash();

private:
    Display& m_disp;

    void drawHeader(State state, bool running);
    void drawVialCounter(int vialIndex);
    void drawProgressBar(int timerMs, int measureMs);
    void drawStartStopButton(bool running);
    void drawErrorOverlay();
};
