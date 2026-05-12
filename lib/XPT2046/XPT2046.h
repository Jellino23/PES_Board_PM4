#pragma once

#include "mbed.h"

/**
 * @file XPT2046.h
 * @brief Treiber für den XPT2046 resistiven Touch-Controller (Joy-It Display).
 *
 * Kommunikation: SPI Mode 0, max. 2 MHz, geteilt mit Display-IC (ST7735).
 * IRQ-Pin fällt auf LOW wenn der Bildschirm berührt wird.
 *
 * Wichtig: onIRQ() führt KEIN SPI aus (ISR-Sicherheit bei geteiltem Bus).
 * Stattdessen: update() im Haupt-Loop aufrufen. Gibt true zurück bei
 * neuem Touch-Ereignis (fallende Flanke).
 *
 * Koordinaten sind für Landscape-Modus (160×128) kalibriert.
 * Falls Berührungen falsch gemappt werden: RAW_*_MIN/MAX anpassen.
 */
class XPT2046
{
public:
    explicit XPT2046(PinName mosi, PinName miso, PinName sck,
                     PinName cs, PinName irq);
    virtual ~XPT2046() = default;

    /**
     * @brief Im Haupt-Loop aufrufen.
     * @return true bei neuem Touch-Ereignis (einmalig pro Berührung).
     *         getX()/getY() liefern dann die aktuellen Koordinaten.
     */
    bool update();

    /** @brief true solange Bildschirm berührt wird (IRQ-Pin LOW). */
    bool isTouched() { return m_irq.read() == 0; }

    /** @brief Zuletzt gemessene X-Koordinate in Pixeln [0..159] (Landscape). */
    int getX() const { return m_pixelX; }

    /** @brief Zuletzt gemessene Y-Koordinate in Pixeln [0..127] (Landscape). */
    int getY() const { return m_pixelY; }

private:
    // Landscape-Auflösung
    static constexpr int SCREEN_W = 160;
    static constexpr int SCREEN_H = 128;

    // Kalibrierungswerte – empirisch anpassen falls nötig
    static constexpr int RAW_X_MIN = 200;
    static constexpr int RAW_X_MAX = 3800;
    static constexpr int RAW_Y_MIN = 200;
    static constexpr int RAW_Y_MAX = 3800;

    SPI         m_spi;
    DigitalOut  m_cs;
    InterruptIn m_irq;

    volatile bool m_touched{false};
    bool          m_wasTouched{false};
    int           m_pixelX{0};
    int           m_pixelY{0};

    uint16_t readRaw(uint8_t cmd);
    void     onIRQ();

    static int clamp(int val, int lo, int hi);
    static int mapToPixel(int raw, int rawMin, int rawMax, int screenSize);
};
