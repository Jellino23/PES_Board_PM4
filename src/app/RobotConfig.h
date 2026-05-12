#pragma once
#include "mbed.h"
#include "PESBoardPinMap.h"

/**
 * @file RobotConfig.h
 * @brief Zentrale Konfigurationsdatei – alle Konstanten, Pins und Timing-Werte.
 *
 * Anpassen an die tatsächliche Verdrahtung und Mechanik.
 */

// ============================================================
//  TIMING
// ============================================================
namespace RobotConfig {

    static constexpr int MAIN_PERIOD_MS  = 20;   // Haupt-Loop-Periode [ms]
    static constexpr int MEASURE_MS      = 5000; // Messdauer [ms]
    static constexpr int GRAB_WAIT_MS    = 1000;  // Wartezeit nach Magnet an/aus [ms]
    static constexpr int TIMEOUT_LIFT_MS = 16000; // Max. Zeit für Lift-Bewegung [ms]
    static constexpr int TIMEOUT_REV_MS  = 15000; // Max. Zeit für Revolver-Bewegung [ms]
    static constexpr int TIMEOUT_LID_MS  = 10000; // Max. Zeit für Deckel-Bewegung [ms]

// ============================================================
//  GESCHWINDIGKEITEN
// ============================================================
    static constexpr float LIFT_SPEED    = 1.0f;  // [rot/s]
    static constexpr float REVOLVER_SPEED = 0.1f; // [rot/s]
    static constexpr float LID_SPEED     = 0.1f; // [rot/s]

    static constexpr float LID_OPEN_ROTATIONS = 0.35f;  // Umdrehungen für voll-offen (~0.4 gemessen)

// ============================================================
//  MOTOR-PARAMETER
// ============================================================
    static constexpr uint32_t STEPPER_STEPS_PER_REV = 200 * 64; // 64 Mikroschritte
    static constexpr float    LID_GEAR_RATIO         = 25.0f;
    static constexpr float    LID_KN                 = 140.0f / 12.0f; // [rpm/V]
    static constexpr float    LID_VOLTAGE_MAX        = 12.0f;

// ============================================================
//  PINS  –  LIFT (Schrittmotor + TMC2209)
// ============================================================
    // Auf PES-Board: freie digitale Ausgänge verwenden
    static constexpr PinName LIFT_STEP   = PB_12;
    static constexpr PinName LIFT_DIR    = PA_11;
    static constexpr PinName LIFT_EN     = PB_2;
    static constexpr PinName LIFT_SEN    = PB_0;  // Endschalter unten (TCST2103)
    static constexpr PinName LIFT_MAGNET = PC_3; // Hubmagnet (war PC_8, Konflikt mit DISP_DC)

// ============================================================
//  PINS  –  REVOLVER (Schrittmotor + TMC2209)
// ============================================================
    static constexpr PinName REV_STEP   = PA_8;
    static constexpr PinName REV_DIR    = PB_7;
    static constexpr PinName REV_EN     = PB_13;
    static constexpr PinName  REV_VIAL            = PC_5; // Lichtschranke – nur Vial-Position
    static constexpr int32_t REV_STEPS_VIAL_TO_HOLE = 320;  // Mikroschritte Vial → Loch (9° bei 1/64)

// ============================================================
//  PINS  –  DECKEL (DC-Motor M1)
// ============================================================
    static constexpr PinName LID_PWM   = PB_PWM_M3;
    static constexpr PinName LID_ENCA  = PB_ENC_A_M3;
    static constexpr PinName LID_ENCB  = PB_ENC_B_M3;
    static constexpr PinName LID_CLOSE = PC_6;    // Endschalter geschlossen

// ============================================================
//  PINS  –  DISPLAY (SPI1)
// ============================================================
    static constexpr PinName DISP_MOSI = PA_7;   // SPI1_MOSI (war PC_12 / SPI3)
    static constexpr PinName DISP_SCLK = PB_3;   // SPI1_SCK  (PA_5 belegt durch LED1)
    static constexpr PinName DISP_CS   = PA_4;   // SPI1_NSS  (war PD_2)
    static constexpr PinName DISP_DC   = PC_8;   // Data/Command (unverändert)
    static constexpr PinName DISP_RST  = PC_9;   // Reset        (unverändert)
    // DISP_MISO entfernt – Display ist write-only (NC in main.cpp)

// ============================================================
//  PINS  –  TOUCH (XPT2046, SPI1 geteilt)
// ============================================================
    static constexpr PinName TOUCH_MISO = PA_6;  // SPI1_MISO – Touch sendet zurück
    static constexpr PinName TOUCH_CS   = PA_15; // SPI1_NSS  – Touch Chip Select
    static constexpr PinName TOUCH_IRQ  = PB_8;  // GPIO_IN   – LOW bei Berührung

// ============================================================
//  PINS  –  BEDIENELEMENTE
// ============================================================
    static constexpr PinName START_BTN  = PB_1;   // Externer Startschalter (active-low, PullUp)

// ============================================================
//  PINS  –  DHT11 Temperatursensor (KY-015)
// ============================================================
    static constexpr PinName DHT_DATA = PB_D3;  // PB_12 – freier digitaler Pin

} // namespace RobotConfig
