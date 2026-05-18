# Vial-Messanlage – Projektdokumentation

## Was diese Anlage macht

Die Anlage ist ein automatisierter Messroboter für Lumineszenz-/Fluoreszenzmessungen an Vials (kleine Probenbehälter). Sie entnimmt Vials aus einem drehbaren Revolver, transportiert sie in eine Messkammer, verschliesst diese lichtdicht, führt die Messung durch und legt die Vials anschliessend wieder zurück.

---

## Ablauf (ein vollständiger Messzyklus)

```
1. HOMING          Lift fährt nach oben (Endschalter), Revolver dreht bis Vial-Lichtschranke

2. LIFT_DOWN_PICK  Lift fährt nach unten zum Vial im Revolver

3. GRAB            Hubmagnet wird aktiviert → Vial wird gegriffen (300 ms warten)

4. LIFT_UP         Lift fährt nach oben mit Vial

5. ROTATE_TO_HOLE  Revolver dreht CW bis Loch-Lichtschranke
                   (Loch-Slots sind im Revolver zwischen den Vial-Slots)

6. LIFT_DOWN_PLACE Vial wird durch das Loch in die Messkammer gefahren

7. RELEASE         Hubmagnet aus → Vial bleibt in Messkammer

8. LIFT_UP_EMPTY   Leeres Seil/Lift fährt hoch (aus Messkammer raus)

9. CLOSE_LID       DC-Motor schiebt Deckel zu (stoppt am Endschalter)

10. MEASURING      Wartet MEASURE_MS (Standard: 5000 ms) → Messung läuft im Dunkeln

11. OPEN_LID       Deckel fährt auf (feste Anzahl Umdrehungen, kein Endschalter)

12. LIFT_DOWN_RETRIEVE  Lift fährt runter in die Messkammer

13. GRAB_AGAIN     Hubmagnet an → Vial greifen

14. LIFT_UP_RETURN Lift hoch mit Vial

15. ROTATE_BACK    Revolver dreht CCW zurück zur ursprünglichen Vial-Position
                   (WICHTIG: Gegenrichtung! Gleiche Lichtschranke wie Schritt 5)

16. LIFT_DOWN_RETURN  Vial in Startposition ablassen

17. RELEASE_HOME   Magnet aus → Vial ist zurück im Revolver

18. LIFT_UP_FINAL  Lift hoch (Parkposition)

19. DONE           Vial-Zähler +1, Revolver dreht CW zur nächsten Vial-Position → weiter mit Schritt 2
```

### Revolver-Slot-Sequenz (zyklisch)
```
[VIAL] [LOCH] [VIAL] [VIAL] [LOCH] [VIAL] [VIAL] [LOCH] ...
```
- Vials werden aus den VIAL-Slots entnommen
- Transport durch LOCH-Slots in die Messkammer
- Beim Zurücklegen dreht der Revolver in **Gegenrichtung (CCW)** zurück zum selben VIAL-Slot

---

## Hardware-Übersicht

| Komponente | Typ | Steuerung |
|---|---|---|
| Lift-Motor | Schrittmotor + TMC2209 | STEP/DIR/EN via GPIO |
| Revolver-Motor | Schrittmotor + TMC2209 | STEP/DIR/EN via GPIO |
| Deckel-Motor | DC-Motor M1 (PES-Board) | PWM + Encoder |
| Lift-Greifer | Hubmagnet 12V DC | DigitalOut (active HIGH) |
| Endschalter Lift oben/unten | Vishay TCST2103 | DigitalIn PullUp, active LOW |
| Endschalter Deckel geschlossen | Vishay TCST2103 | DigitalIn PullUp, active LOW |
| Revolver Vial-Position | Vishay TCST2103 | DigitalIn PullUp, active LOW |
| Revolver Loch-Position | Vishay TCST2103 | DigitalIn PullUp, active LOW |
| Display | JoyIT 1.8" 160×128, ST7735/ILI9163 | SPI |
| Touch-Controller | XPT2046 (auf Display-Modul) | SPI (eigener CS) |
| Bedienknopf | Mechanischer Toggle-Button | DebounceIn (BUTTON1) |

### TCST2103 Lichtschranke
- Phototransistor-Ausgang
- Strahl unterbrochen → Transistor leitet → Pin LOW (active LOW)
- Immer mit `PullUp` konfigurieren
- Alle `isAtTop()`, `isAtBottom()`, `isAtVial()`, `isAtHole()`, `isClosed()` geben `true` zurück wenn `read() == 0`

### TMC2209 Schrittmotortreiber
- Ansteuerung ausschliesslich über STEP/DIR/EN (kein UART, kein Software-Treiber)
- EN active LOW (0 = Treiber aktiv, 1 = deaktiviert)
- DIR: 1 = CW, 0 = CCW
- STEP: rising edge = ein Mikroschritt
- Minimale STEP-Pulsbreite laut Datenblatt: 100 ns → im Code: 10 µs (sicherer Wert)
- Microstepping: 16 → `STEPPER_STEPS_PER_REV = 200 × 16 = 3200`

### Hubmagnet
- Datenblatt: 0.2–6.6 Nm, 12V DC, 2W
- Active HIGH: `DigitalOut = 1` → Magnet angezogen → Vial gegriffen
- Wartezeit nach An/Aus: 300 ms (`GRAB_WAIT_MS`)

---

## Microcontroller & Framework

- **Board:** STM32 NUCLEO-F446RE (STM32F446RE)
- **Framework:** mbed OS (via PlatformIO)
- **Build:** PlatformIO mit `framework = mbed`

---

## Dateistruktur

```
projekt-root/
├── platformio.ini          ← Board + build_flags mit Include-Pfaden
├── include/
│   └── PESBoardPinMap.h    ← Pin-Makros des PES-Boards (aus Repo)
├── lib/
│   ├── DCMotor/            ← DC-Motor Klasse (aus Repo, nicht ändern)
│   ├── Stepper/            ← Repo-Stepper (wird NICHT verwendet)
│   ├── ThreadFlag/         ← ThreadFlag Klasse (aus Repo, wird genutzt)
│   ├── FastPWM/            ← FastPWM (aus Repo)
│   └── ...                 ← weitere Repo-Libs
└── src/
    ├── main.cpp            ← nur Verdrahtung (~80 Zeilen)
    ├── hw/                 ← Hardware-Treiber
    │   ├── StepperTMC2209.h/.cpp
    │   ├── LiftMotor.h/.cpp
    │   ├── Revolver.h/.cpp
    │   └── Lid.h/.cpp
    ├── ui/                 ← Alles was der Nutzer sieht
    │   ├── Display.h/.cpp
    │   └── DisplayLayout.h/.cpp
    └── app/                ← Applikationslogik
        ├── RobotConfig.h   ← ALLE Pins, Timeouts, Speeds hier
        ├── StateMachine.h
        └── StateMachine.cpp
```

---

## Abhängigkeiten zwischen Modulen

```
main.cpp
  ├── app/StateMachine   →  hw/LiftMotor
  │                      →  hw/Revolver
  │                      →  hw/Lid
  │                      →  app/RobotConfig
  │
  └── ui/DisplayLayout   →  ui/Display
                         →  app/StateMachine  (für enum State + stateName())
```

**Regel:** `hw/` kennt weder `ui/` noch `app/`. `ui/` kennt `hw/` nicht.

---

## platformio.ini

```ini
[env:nucleo_f446re]
platform  = ststm32
board     = nucleo_f446re
framework = mbed

build_flags =
    -I src
    -I src/hw
    -I src/ui
    -I src/app

monitor_speed = 115200
```

Die vier `-I` Flags sind zwingend damit Includes wie `#include "LiftMotor.h"` aus Unterordnern funktionieren.

---

## Pin-Belegung (RobotConfig.h)

Alle Pins in `src/app/RobotConfig.h`. Makros kommen aus `include/PESBoardPinMap.h`.

| Konstante | Makro | STM32-Pin | Funktion |
|---|---|---|---|
| `LIFT_STEP` | `PB_D0` | PB_2 | Schrittmotor Lift STEP |
| `LIFT_DIR` | `PB_D1` | PC_8 | Schrittmotor Lift DIR |
| `LIFT_EN` | `PB_D2` | PC_6 | Schrittmotor Lift EN |
| `LIFT_TOP` | `PB_A0` | PC_2 | Endschalter Lift oben |
| `LIFT_BOT` | `PB_A1` | PC_3 | Endschalter Lift unten |
| `LIFT_MAGNET` | `PB_D2` | PC_6 | Hubmagnet via invertierender NPN-Treiberstufe (BC547) |
| `REV_STEP` | `PC_6` | PC_6 | Schrittmotor Revolver STEP |
| `REV_DIR` | `PC_8` | PC_8 | Schrittmotor Revolver DIR |
| `REV_EN` | `PB_12` | PB_12 | Schrittmotor Revolver EN |
| `REV_VIAL` | `PB_A2` | PC_5 | Lichtschranke Vial-Position |
| `REV_HOLE` | `PB_A3` | PB_1 | Lichtschranke Loch-Position |
| `LID_PWM` | `PB_PWM_M1` | PB_13 | Deckel DC-Motor PWM |
| `LID_ENCA` | `PB_ENC_A_M1` | PA_6 | Deckel Encoder A |
| `LID_ENCB` | `PB_ENC_B_M1` | PC_7 | Deckel Encoder B |
| `LID_CLOSE` | `PB_A1` | PC_3 | Endschalter Deckel geschlossen (unbenutzt; PC_6 fuer Hubmagnet freigegeben) |
| `DISP_MOSI` | `PB_5` | PB_5 | Display SPI MOSI |
| `DISP_MISO` | `PB_4` | PB_4 | Display SPI MISO |
| `DISP_SCLK` | `PB_3` | PB_3 | Display SPI SCLK |
| `DISP_CS` | `PA_4` | PA_4 | Display Chip-Select |
| `DISP_DC` | `PA_3` | PA_3 | Display Data/Command |
| `DISP_RST` | `PA_2` | PA_2 | Display Reset |

---

## Bekannte Probleme & Fixes (Entwicklungsgeschichte)

### Fix 1: `ThreadFlag` nicht gefunden
`StepperTMC2209.h` fehlte `#include "ThreadFlag.h"`. Die Klasse liegt in `lib/ThreadFlag/`.

### Fix 2: `DigitalIn::read()` nicht `const`
mbed deklariert `DigitalIn::read()` und `DigitalOut::read()` nicht als `const`. Alle `isAtTop()`, `isAtBottom()`, `isGrabbing()` etc. dürfen daher **kein** `const` am Ende haben.

### Fix 3: `RobotConfig.h` ohne PESBoardPinMap
`RobotConfig.h` nutzt `PB_D0`, `PB_PWM_M1` etc., muss daher `#include "PESBoardPinMap.h"` enthalten.

### Fix 4: Include-Pfade fehlen
Ohne `-I src/hw -I src/ui -I src/app` in `build_flags` findet der Compiler `#include "hw/LiftMotor.h"` nicht. Lösung: `platformio.ini` im Projekt-Root mit diesen Flags.

---

## Wichtige Implementierungsdetails

### StepperTMC2209 (src/hw/)
- Eigene Implementierung, **nicht** der `Stepper`-Softwaretreiber aus `lib/Stepper/`
- Ticker → Thread → Timeout Pattern (identisch zu DCMotor aus dem Repo)
- Thread-Priorität: `osPriorityHigh2`
- Destruktor: setzt `m_exitThread = true`, ruft `flags_set` auf, joined den Thread sauber
- `setVelocity(0)` stoppt den Ticker sofort

### LiftMotor (src/hw/)
- Wraps `StepperTMC2209` + 2× `DigitalIn` (Endschalter) + `DigitalOut` (Magnet)
- `moveUp()` = positive Velocity, `moveDown()` = negative Velocity
- Vorzeichen ggf. anpassen je nach Motoreinbaurichtung

### Revolver (src/hw/)
- Positionierung **nur über Lichtschranken**, nicht über Schrittzähler
- `turnCW()` / `turnCCW()` = non-blocking Velocity-Modus
- State Machine pollt `isAtVial()` / `isAtHole()` und ruft `stop()` auf

### Lid (src/hw/)
- `closeLid()`: Velocity-Modus negativ, State Machine stoppt bei `isClosed()`
- `openLid()`: `DCMotor::setRotationRelative(openRotations)` – feste Umdrehungen
- `isOpen()`: geschätzt über `m_motor.getRotation() >= openRotations * 0.9f`

### StateMachine (src/app/)
- **Entry-Pattern:** `m_entry = true` beim Zustandswechsel, erster `update()`-Aufruf im neuen Zustand führt Initialisierung durch und setzt `m_entry = false`
- `m_timerMs` wird nur inkrementiert wenn `!m_entry` (kein Zählen im Entry-Schritt)
- Jeder Zustand hat einen eigenen `handle*()`-Handler
- `transitionTo()` setzt `m_state`, `m_entry = true`, `m_timerMs = 0` und loggt via `printf`
- Timeout in jedem Zustand → `State::ERROR` wenn überschritten
- `ERROR` bleibt aktiv bis `setRunning(false)` aufgerufen wird

### Display (src/ui/)
- ST7735 / ILI9163 kompatibles SPI-Display (JoyIT 1.8", 160×128 Pixel)
- Eingebetteter 5×7 Font (ASCII 32–122), keine externe Font-Datei
- `fillRect()` ist die Basis-Primitive für alle Zeichenoperationen

### DisplayLayout (src/ui/)
- Kennt `StateMachine.h` für `enum State` und `stateName()`
- `update()` wird alle 5 Loop-Iterationen aufgerufen (100 ms) um SPI nicht zu überlasten
- Fortschrittsbalken nur im Zustand `MEASURING` sichtbar

### main.cpp
- Hält alle Hardware-Objekte als `static` Instanzen
- `DebounceIn userBtn` mit `.fall()` Callback für Toggle
- Loop-Timing über `Timer` + `thread_sleep_for(toSleep)`
- LED1: leuchtet wenn running, blinkt schnell bei ERROR

---

## Aktueller Stand / offene Punkte

- [x] Alle Klassen implementiert und modular aufgeteilt
- [x] Kompilierfehler `ThreadFlag`, `const`-Fehler, `PESBoardPinMap` behoben
- [x] `platformio.ini` mit korrekten Include-Pfaden
- [x] **Hubmagnet funktionsfähig** – auf `PC_6` (`PB_D2`) umgezogen (PC_3/PC_8 als Analog-Input/Display-Pin untauglich). Hardware: invertierende NPN-Treiberstufe (BC547): GPIO→1 kΩ→Basis, 10 kΩ Basis-Pull-up auf +5 V, Collector→MOSFET-Gate, Gate-Pull-up 10 kΩ→+12 V, Freilaufdiode parallel zur Spule. **Logik invertiert** (`grab()` = GPIO LOW), zentral in `LiftMotor.cpp`. Verifiziert am Test-Branch `test/hubmagnet-pc3`.
- [x] **Greifer-Ablauf: Puls statt Dauerstrom** – Magnet wird in `GRAB`/`GRAB_AGAIN`/`RELEASE`/`RELEASE_HOME` nur als kurzer Puls (`GRAB_WAIT_MS`) aktiviert, danach AUS. Alle Bewegungen (Lift hoch/runter, Revolver) laufen mit Magnet AUS = Greifer „zu" = kollisionsfrei. Hochfahr-Zustände rufen zusätzlich defensiv `release()` auf. (Bistabiler Latch: Puls greift/löst, hält stromlos.)
- [ ] **Restliche Pin-Belegung noch nicht final** – `RobotConfig.h` ggf. weiter an tatsächliche Verdrahtung anpassen (REV_STEP/DIR, LIFT_EN/REV_EN prüfen)
- [ ] Display-Code noch nicht auf echter Hardware getestet
- [ ] Touch-Controller XPT2046 noch nicht integriert (Klasse existiert im v2-Ordner, aber nicht im aktuellen Build)
- [ ] Kalibrierung: Schrittanzahl für Revolver-Slots (Anzahl Schritte zwischen zwei Slots) noch nicht definiert – aktuell rein sensorbasiert
- [ ] `LID_OPEN_ROTATIONS = 2.5f` muss mechanisch verifiziert werden
- [ ] DC-Motor Parameter (`LID_GEAR_RATIO`, `LID_KN`) müssen mit tatsächlichem Motor abgeglichen werden

---

## Zu beachten beim Weiterentwickeln

1. **Nie** `lib/Stepper/Stepper.h` für die Schrittmotoren verwenden – eigene `StepperTMC2209`-Klasse nutzen
2. Alle Sensor-Methoden (`isAtTop()` etc.) haben **kein** `const` wegen mbed-Einschränkung
3. `RobotConfig.h` ist die einzige Datei für Pins und Konstanten – Magic Numbers im Code vermeiden
4. `PESBoardPinMap.h` muss in jeder Datei inkludiert sein die PES-Board-Makros verwendet (oder via `RobotConfig.h`)
5. Thread-Synchronisation: `StepperTMC2209` verwendet `ThreadFlag` aus `lib/ThreadFlag/` – nicht ersetzen
6. Der DC-Motor für den Deckel nutzt `PB_ENABLE_DCMOTORS` (PB_15) intern in der `DCMotor`-Klasse – muss nicht separat aktiviert werden
