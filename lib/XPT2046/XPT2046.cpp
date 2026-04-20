#include "XPT2046.h"

// XPT2046 Befehlsbytes (differential mode, 12-bit)
static constexpr uint8_t CMD_READ_X = 0xD0; // A2=1 A1=0 A0=1
static constexpr uint8_t CMD_READ_Y = 0x90; // A2=1 A1=0 A0=0

XPT2046::XPT2046(PinName mosi, PinName miso, PinName sck,
                 PinName cs, PinName irq)
    : m_spi(mosi, miso, sck),
      m_cs(cs, 1),           // CS inaktiv (HIGH)
      m_irq(irq, PullUp)
{
    m_spi.format(8, 0);           // 8-Bit, SPI Mode 0
    m_spi.frequency(2'000'000);   // 2 MHz

    // IRQ-Handler bei fallender Flanke (Bildschirm wird berührt)
    m_irq.fall(callback(this, &XPT2046::onIRQ));
}

// ============================================================
// IRQ-Handler – wird im Interrupt-Kontext aufgerufen
// ============================================================
void XPT2046::onIRQ()
{
    uint16_t rawX = readRaw(CMD_READ_X);
    uint16_t rawY = readRaw(CMD_READ_Y);

    m_pixelX = mapToPixel(rawX, RAW_X_MIN, RAW_X_MAX, SCREEN_W);
    m_pixelY = mapToPixel(rawY, RAW_Y_MIN, RAW_Y_MAX, SCREEN_H);
}

// ============================================================
// SPI-Transaktion: 8-Bit Befehl senden, 16-Bit Antwort lesen
// Rückgabe: obere 12 Bit des ADC-Ergebnisses
// ============================================================
uint16_t XPT2046::readRaw(uint8_t cmd)
{
    m_cs = 0;

    m_spi.write(cmd);                          // Befehl senden
    uint16_t hi = m_spi.write(0x00);           // High-Byte lesen
    uint16_t lo = m_spi.write(0x00);           // Low-Byte lesen

    m_cs = 1;

    // 12-Bit Wert: hi[7:0] << 5 | lo[7:3]
    return static_cast<uint16_t>(((hi << 8) | lo) >> 3);
}

// ============================================================
// Hilfsfunktionen
// ============================================================
int XPT2046::clamp(int val, int lo, int hi)
{
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

int XPT2046::mapToPixel(int raw, int rawMin, int rawMax, int screenSize)
{
    raw = clamp(raw, rawMin, rawMax);
    return (raw - rawMin) * (screenSize - 1) / (rawMax - rawMin);
}
