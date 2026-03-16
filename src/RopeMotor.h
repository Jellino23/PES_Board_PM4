#pragma once              // Schutz gegen doppeltes Einlesen
#include "mbed.h"         // Externe Abhängigkeiten die die Klasse braucht

class RopeMotor {         // Klassenname – entspricht dem Dateinamen

public:                   // ← Alles hier ist von aussen sichtbar (main.cpp kann es nutzen)

    RopeMotor(PinName motorPin, PinName sensorTop, PinName sensorBottom);
    //  ↑ Konstruktor – gleicher Name wie Klasse, kein Rückgabetyp
    //    Parameter sagen: "ich brauche diese Pins um zu funktionieren"

    void moveUp();        // Nur die Unterschrift – kein { } kein Inhalt
    void moveDown();      // "Es gibt diese Funktion" – mehr nicht
    void stop();
    bool isAtTop();       // bool = gibt true/false zurück
    bool isAtBottom();

private:                  // ← Alles hier ist nach aussen versteckt
                          //   main.cpp kann _motor nicht direkt ansprechen

    PwmOut    _motor;         // Die echte Hardware
    DigitalIn _sensorTop;     // _ vorne = Konvention für private Variablen
    DigitalIn _sensorBottom;

    static constexpr float SPEED = 0.3f;
    //  ↑ static constexpr = Konstante die zur Kompilierzeit feststeht
    //    gehört zur Klasse, nicht zu einem einzelnen Objekt
};                        // ← Semikolon nicht vergessen!