#include "DHT11.h"

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
    uint8_t data[5] = {0, 0, 0, 0, 0};

    // --- Start-Signal: Host zieht LOW für 18 ms, dann loslassen ---
    m_DataPin.output();
    m_DataPin = 0;
    wait_us(18000);
    m_DataPin = 1;
    wait_us(30);
    m_DataPin.input();

    // --- Sensor-Antwort abwarten: 80 µs LOW, 80 µs HIGH ---
    int timeout = 0;
    while (m_DataPin == 1) {
        wait_us(1);
        if (++timeout > 100) return false; // kein Pull-Up / Sensor fehlt
    }
    timeout = 0;
    while (m_DataPin == 0) {
        wait_us(1);
        if (++timeout > 100) return false;
    }
    timeout = 0;
    while (m_DataPin == 1) {
        wait_us(1);
        if (++timeout > 100) return false;
    }

    // --- 40 Datenbits einlesen ---
    for (int i = 0; i < 40; i++) {
        // Jedes Bit beginnt mit ~50 µs LOW
        timeout = 0;
        while (m_DataPin == 0) {
            wait_us(1);
            if (++timeout > 80) return false;
        }

        // HIGH-Dauer messen: 26–28 µs = 0, ~70 µs = 1
        int highUs = 0;
        while (m_DataPin == 1) {
            wait_us(1);
            if (++highUs > 90) return false;
        }

        // Bit speichern (MSB first)
        data[i / 8] <<= 1;
        if (highUs > 40) {
            data[i / 8] |= 1;
        }
    }

    // --- Checksumme prüfen ---
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) return false;

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
        bool ok = readSensor(temp, hum);

        if (ok) {
            m_temperature = temp;
            m_humidity    = hum;
            m_valid       = true;
            printf("[DHT11] Temp: %.1f C  Hum: %.1f %%\n", temp, hum);
        } else {
            m_valid = false;
            printf("[DHT11] Lesefehler (kein Sensor oder Checksumme falsch)\n");
        }
    }
}

void DHT11::sendThreadFlag()
{
    m_Thread.flags_set(m_ThreadFlag);
}
