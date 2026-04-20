#pragma once

#include "mbed.h"

/**
 * @file XPT2046.h
 * @brief Treiber für den XPT2046 resistiven Touch-Controller (Joy-It Display).
 *
 * Kommunikation: SPI Mode 0, max. 2 MHz, geteilt mit Display-IC (ST7735).
 * IRQ-Pin fällt auf LOW, wenn der Bildschirm berührt wird.
 *
 * Koordinaten werden beim IRQ automatisch ausgelesen und auf Pixel gemappt.
 * Kalibrierung: RAW_*_MIN/MAX können angepasst werden falls Berührungen
 * falsch gemappt werden.
 *
 * Verwendung:
 *   XPT2046 touch(PA_7, PA_6, PB_3, PA_15, PB_8);
 *   if (touch.isTouched()) {
 *       int x = touch.getX();  // 0..127
 *       int y = touch.getY();  // 0..159
 *   }
 */
class XPT2046
{
public:
    explicit XPT2046(PinName mosi, PinName miso, PinName sck,
                     PinName cs, PinName irq);
    virtual ~XPT2046() = default;

    /** @brief true wenn Bildschirm aktuell berührt wird (IRQ-Pin LOW). */
    bool isTouched() { return m_irq.read() == 0; }

    /** @brief Zuletzt gemessene X-Koordinate in Pixeln [0..127]. */
    int getX() const { return m_pixelX; }

    /** @brief Zuletzt gemessene Y-Koordinate in Pixeln [0..159]. */
    int getY() const { return m_pixelY; }

private:
    // Kalibrierungswerte – empirisch anpassen falls nötig
    static constexpr int SCREEN_W   = 128;
    static constexpr int SCREEN_H   = 160;
    static constexpr int RAW_X_MIN  = 200;
    static constexpr int RAW_X_MAX  = 3800;
    static constexpr int RAW_Y_MIN  = 200;
    static constexpr int RAW_Y_MAX  = 3800;

    SPI         m_spi;
    DigitalOut  m_cs;
    InterruptIn m_irq;

    volatile int m_pixelX{0};
    volatile int m_pixelY{0};

    uint16_t readRaw(uint8_t cmd);
    void     onIRQ();

    static int clamp(int val, int lo, int hi);
    static int mapToPixel(int raw, int rawMin, int rawMax, int screenSize);
};
