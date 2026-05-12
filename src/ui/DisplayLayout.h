#pragma once
#include "Display.h"

enum class State;

/**
 * @file DisplayLayout.h
 * @brief Applikations-spezifisches Display-Layout (Landscape 160×128).
 *
 * Layout-Zonen:
 *  Y=  4..17  Haupt-Status-Text (scale=2): "PICK+PLACE" / "MEASURING" / "IDLE" / "ERROR"
 *  Y= 22..31  Fortschrittsbalken (nur MEASURING)
 *  Y= 36..49  Temperatur (scale=2)
 *  Y= 54..67  Luftfeuchtigkeit (scale=2)
 *  Y= 76..123 START/STOP-Knopf (gross, touch-freundlich)
 */
class DisplayLayout {
public:
    struct Rect { int x, y, w, h; };

    // Touch-Zone START/STOP-Button
    static const Rect BTN_STARTSTOP;

    explicit DisplayLayout(Display& display);

    /**
     * @brief Vollständiges Neuzeichnen.
     * @param state       Maschinenzustand
     * @param timerMs     Verstrichene Zeit im aktuellen Zustand [ms]
     * @param running     true = Anlage läuft
     * @param measureMs   Gesamte Messdauer [ms] (für Fortschrittsbalken)
     * @param temperature Temperatur vom DHT11 [°C]
     * @param humidity    Luftfeuchtigkeit vom DHT11 [%]
     * @param sensorValid true wenn DHT11-Daten gültig
     */
    void update(State state, int vialIndex, int timerMs, bool running, int measureMs,
                float temperature, float humidity, bool sensorValid);

    /** @brief Nur Fortschrittsbalken + Prozent aktualisieren (schnell, kein Flicker). */
    void updateProgress(int timerMs, int measureMs);

    void drawButton(const Rect& r, const char* label, uint16_t bg, int textScale = 1);
    void drawSplash();

private:
    Display& m_disp;

    void drawStatusText(State state, bool running);
    void drawProgressBar(int timerMs, int measureMs);
    void drawSensor(float temperature, float humidity, bool valid);
    void drawVialCounter(int vialIndex);
    void drawStartStopButton(bool running);
};
