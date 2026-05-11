#include "DHT11.h"
#include "platform/CriticalSectionLock.h"

DHT11::DHT11(PinName pin)
    : m_DataPin(pin, PIN_INPUT, OpenDrain, 1),
      m_Thread(osPriorityNormal)
{
    m_Thread.start(callback(this, &DHT11::threadTask));
    m_Ticker.attach(callback(this, &DHT11::sendThreadFlag),
                    std::chrono::microseconds{PERIOD_MUS});
}

DHT11::~DHT11()
{
    m_Ticker.detach();
    m_Thread.terminate();
}

// ============================================================
// Sensor auslesen – DHT11 Single-Wire Bit-Bang
// ============================================================
bool DHT11::readSensor(float& temp, float& hum)
{
    uint8_t data[5]  = {0, 0, 0, 0, 0};
    int     errStage = 0;
    int     errBit   = -1;

    // Start: LOW ≥18 ms, dann Bus freigeben
    m_DataPin.output();
    m_DataPin = 0;
    wait_us(18000);

    // Sofort locken, dann input() – gpio_dir() schaltet auf push-pull,
    // nicht OpenDrain: m_DataPin=1 würde 3,3 V treiben und der Sensor
    // könnte den Bus nicht auf LOW ziehen. input() ist garantiert high-Z.
    bool ok = [&]() -> bool {
        CriticalSectionLock lock;

        m_DataPin.input();  // Bus freigeben (high-Z) – Sensor kann jetzt ziehen
        wait_us(40);        // Host-Release: 20–40 µs HIGH (Spec)

        int timeout = 0;

        // Sensor-Antwort: ~80 µs LOW
        while (m_DataPin == 1) {
            wait_us(1);
            if (++timeout > 200) { errStage = 1; return false; }
        }

        // Sensor-Antwort: ~80 µs HIGH
        timeout = 0;
        while (m_DataPin == 0) {
            wait_us(1);
            if (++timeout > 200) { errStage = 2; return false; }
        }

        // Warten bis erste Bit-Präambel (LOW) beginnt
        timeout = 0;
        while (m_DataPin == 1) {
            wait_us(1);
            if (++timeout > 200) { errStage = 3; return false; }
        }

        // 40 Datenbits einlesen
        for (int i = 0; i < 40; i++) {
            // ~50 µs LOW-Präambel
            timeout = 0;
            while (m_DataPin == 0) {
                wait_us(1);
                if (++timeout > 100) { errStage = 4; errBit = i; return false; }
            }

            // HIGH-Dauer messen: 26–28 µs → 0, ~70 µs → 1
            int highUs = 0;
            while (m_DataPin == 1) {
                wait_us(1);
                if (++highUs > 100) { errStage = 5; errBit = i; return false; }
            }

            data[i / 8] <<= 1;
            if (highUs > 40) data[i / 8] |= 1;
        }

        return true;
    }();
    // CriticalSectionLock freigegeben – printf ab hier sicher

    if (!ok) {
        static const char* stages[] = {
            "", "kein ACK-LOW", "kein ACK-HIGH", "kein Daten-LOW",
            "Bit-LOW Timeout", "Bit-HIGH Timeout"
        };
        if (errBit >= 0)
            printf("[DHT11] Fehler: %s  Bit=%d\n", stages[errStage], errBit);
        else
            printf("[DHT11] Fehler: %s\n", stages[errStage]);
        return false;
    }

    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        printf("[DHT11] Checksumme: berechnet=%02X empfangen=%02X"
               "  Daten=%02X %02X %02X %02X\n",
               checksum, data[4], data[0], data[1], data[2], data[3]);
        return false;
    }

    hum  = static_cast<float>(data[0]);
    temp = static_cast<float>(data[2]);
    return true;
}

// ============================================================
// Thread + Ticker
// ============================================================
void DHT11::threadTask()
{
    while (true) {
        ThisThread::flags_wait_any(m_ThreadFlag);

        float temp = 0.0f;
        float hum  = 0.0f;

        if (readSensor(temp, hum)) {
            m_temperature = temp;
            m_humidity    = hum;
            m_valid       = true;
            printf("[DHT11] Temp: %.1f C  Hum: %.1f %%\n", temp, hum);
        } else {
            m_valid = false;
        }
    }
}

void DHT11::sendThreadFlag()
{
    m_Thread.flags_set(m_ThreadFlag);
}
