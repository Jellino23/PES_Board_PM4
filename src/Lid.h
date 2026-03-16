#pragma once              // Schutz gegen doppeltes Einlesen
#include "mbed.h"         // Externe Abhängigkeiten die die Klasse braucht
#include "DCMotor.h"
#include "DigitalIn.h"
#include "DigitalOut.h"

class Lid {         // Klassenname – entspricht dem Dateinamen

public:                   // ← Alles hier ist von aussen sichtbar (main.cpp kann es nutzen)

    Lid(PinName lidMotorPWM, PinName lidMotorEncA, PinName lidMotorEncB, PinName sensorOpen, PinName sensorClose);
    //  ↑ Konstruktor – gleicher Name wie Klasse, kein Rückgabetyp
    //    Parameter sagen: "ich brauche diese Pins um zu funktionieren"

    void openLid();        // Nur die Unterschrift – kein { } kein Inhalt
    void closeLid();      // "Es gibt diese Funktion" – mehr nicht
    void stopLid();
    bool isOpen();       // bool = gibt true/false zurück
    bool isClose();

private:                  // ← Alles hier ist nach aussen versteckt
                          //   main.cpp kann _motor nicht direkt ansprechen

    DCMotor    _lidMotor;         // Die echte Hardware
    DigitalIn _sensorOpen;     // _ vorne = Konvention für private Variablen
    DigitalIn _sensorClose;

    static constexpr float SPEED = 0.3f;
    //  ↑ static constexpr = Konstante die zur Kompilierzeit feststeht
    //    gehört zur Klasse, nicht zu einem einzelnen Objekt
};                        // ← Semikolon nicht vergessen!