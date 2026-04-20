#pragma once

#include "mbed.h"
#include "ThreadFlag.h"

/**
 * @file DHT11.h
 * @brief Treiber für den DHT11 Temperatur- und Luftfeuchtigkeitssensor (KY-015).
 *
 * Kommunikationsprotokoll: Single-Wire (1-Wire Bit-Bang)
 * Messintervall: 10 Sekunden (Minimum laut Datenblatt: 2 s)
 * Messbereich Temperatur: 0–50 °C (±2 °C)
 * Messbereich Luftfeuchtigkeit: 20–90 % RH (±5 % RH)
 *
 * Verwendung:
 *   DHT11 sensor(PB_12);
 *   float temp = sensor.readTemperature();
 *   float hum  = sensor.readHumidity();
 */
class DHT11
{
public:
    explicit DHT11(PinName pin);
    virtual ~DHT11();

    float readTemperature() const { return m_temperature; }
    float readHumidity()    const { return m_humidity; }
    bool  isValid()         const { return m_valid; }

private:
    static constexpr int64_t PERIOD_MUS = 10'000'000; // 10 s

    DigitalInOut m_DataPin;

    Thread     m_Thread;
    Ticker     m_Ticker;
    ThreadFlag m_ThreadFlag;

    float m_temperature{0.0f};
    float m_humidity{0.0f};
    bool  m_valid{false};

    bool readSensor(float& temp, float& hum);
    void threadTask();
    void sendThreadFlag();
};
