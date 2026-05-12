#include "XPT2046.h"

static constexpr uint8_t CMD_READ_X = 0xD0;
static constexpr uint8_t CMD_READ_Y = 0x90;

XPT2046::XPT2046(PinName mosi, PinName miso, PinName sck,
                 PinName cs, PinName irq)
    : m_spi(mosi, miso, sck),
      m_cs(cs, 1),
      m_irq(irq, PullUp)
{
    m_spi.format(8, 0);
    m_spi.frequency(2'000'000);

    // IRQ: nur Flag setzen, kein SPI im ISR-Kontext
    m_irq.fall(callback(this, &XPT2046::onIRQ));
}

// ============================================================
// Haupt-Loop: SPI-Lesen hier (nicht im ISR)
// ============================================================
bool XPT2046::update()
{
    bool touched = isTouched();

    // Koordinaten lesen wenn Flag gesetzt (von IRQ) und Finger noch drauf
    if (m_touched && touched) {
        m_touched = false;

        uint16_t rawX = readRaw(CMD_READ_X);
        uint16_t rawY = readRaw(CMD_READ_Y);

        // Landscape-Mapping (MADCTL 0x60, 90°CW):
        // physikalisch-X → Landscape-Y, physikalisch-Y → Landscape-X (gespiegelt)
        m_pixelY = mapToPixel(rawX, RAW_X_MIN, RAW_X_MAX, SCREEN_H);
        m_pixelX = (SCREEN_W - 1) - mapToPixel(rawY, RAW_Y_MIN, RAW_Y_MAX, SCREEN_W);
    }

    // Flanken-Detektion: true nur beim ersten Frame der Berührung
    bool newTouch = touched && !m_wasTouched;
    m_wasTouched  = touched;
    return newTouch;
}

// ============================================================
// ISR: nur Flag – kein SPI!
// ============================================================
void XPT2046::onIRQ()
{
    m_touched = true;
}

// ============================================================
// SPI-Transaktion: 8-Bit Befehl, 16-Bit Antwort (12-Bit ADC)
// ============================================================
uint16_t XPT2046::readRaw(uint8_t cmd)
{
    m_cs = 0;
    m_spi.write(cmd);
    uint16_t hi = m_spi.write(0x00);
    uint16_t lo = m_spi.write(0x00);
    m_cs = 1;
    return static_cast<uint16_t>(((hi << 8) | lo) >> 3);
}

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
