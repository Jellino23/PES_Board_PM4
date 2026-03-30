#pragma once
#include "mbed.h"

/**
 * @file Display.h
 * @brief Low-Level SPI-Display-Treiber (ST7735 / ILI9163 kompatibel).
 *
 * Verantwortlich für:
 *  - SPI-Initialisierung und Hardware-Reset
 *  - Grafik-Primitives: fillRect, drawChar, drawText
 *
 * Kennt keine Applikationslogik – nur Pixel und Text.
 */
class Display {
public:
    // RGB565-Farbkonstanten
    static constexpr uint16_t BLACK  = 0x0000;
    static constexpr uint16_t WHITE  = 0xFFFF;
    static constexpr uint16_t RED    = 0xF800;
    static constexpr uint16_t GREEN  = 0x07E0;
    static constexpr uint16_t BLUE   = 0x001F;
    static constexpr uint16_t YELLOW = 0xFFE0;
    static constexpr uint16_t GRAY   = 0x7BEF;
    static constexpr uint16_t ORANGE = 0xFD20;

    static constexpr int WIDTH  = 128;
    static constexpr int HEIGHT = 160;

    /**
     * @param mosi, miso, sclk  SPI-Pins
     * @param cs, dc, rst       Chip-Select, Data/Command, Reset
     * @param freqHz            SPI-Taktfrequenz
     */
    Display(PinName mosi, PinName miso, PinName sclk,
            PinName cs,   PinName dc,   PinName rst,
            int freqHz = 8000000);

    void init();

    void fillRect(int x, int y, int w, int h, uint16_t color);
    void fillScreen(uint16_t color);

    /**
     * @brief Einzelnes Zeichen zeichnen (5×7 Font).
     * @param x, y   Obere linke Ecke in Pixel
     * @param c      ASCII-Zeichen (32–122)
     * @param fg     Vordergrundfarbe (RGB565)
     * @param bg     Hintergrundfarbe (RGB565)
     */
    void drawChar(int x, int y, char c, uint16_t fg, uint16_t bg = BLACK);

    /**
     * @brief String zeichnen.  Zeilenumbruch bei WIDTH-6.
     */
    void drawText(int x, int y, const char* s,
                  uint16_t fg = WHITE, uint16_t bg = BLACK);

private:
    SPI        m_spi;
    DigitalOut m_cs;
    DigitalOut m_dc;
    DigitalOut m_rst;

    void cmd(uint8_t c);
    void data8(uint8_t d);
    void data16(uint16_t d);
    void setWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

    // 5×7 Mini-Font, ASCII 32..122 ('z')
    static const uint8_t s_font[][5];
};
