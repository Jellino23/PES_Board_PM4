#pragma once              // Schutz gegen doppeltes Einlesen
#include "mbed.h"         // Externe Abhängigkeiten die die Klasse braucht

#ifndef M_PIf
    #define M_PIf 3.14159265358979323846f // pi
#endif



class Revolver {         // Klassenname – entspricht dem Dateinamen

public:                   // ← Alles hier ist von aussen sichtbar (main.cpp kann es nutzen)

    Revolver(PinName motorPin, PinName sensorTop, PinName sensorBottom);
    //  ↑ Konstruktor – gleicher Name wie Klasse, kein Rückgabetyp
    //    Parameter sagen: "ich brauche diese Pins um zu funktionieren"

    void turnCW();        // Nur die Unterschrift – kein { } kein Inhalt
    void turnCCW();      // "Es gibt diese Funktion" – mehr nicht
    void stop();
    bool isAtVial();       // bool = gibt true/false zurück
    bool isAtHole();

private:                  // ← Alles hier ist nach aussen versteckt
                          //   main.cpp kann _motor nicht direkt ansprechen

    PwmOut    _motor;         // Die echte Hardware
    DigitalIn _sensorVial;     // _ vorne = Konvention für private Variablen
    DigitalIn _sensorHole;

    static constexpr float SPEED = 0.3f;
    //  ↑ static constexpr = Konstante die zur Kompilierzeit feststeht
    //    gehört zur Klasse, nicht zu einem einzelnen Objekt
};                        // ← Semikolon nicht vergessen!