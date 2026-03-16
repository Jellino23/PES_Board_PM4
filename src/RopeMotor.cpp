#include "RopeMotor.h"    // Eigenen Header einbinden – kennt damit alle Deklarationen
                          // kein #pragma once nötig

// Konstruktor
RopeMotor::RopeMotor(PinName motorPin, PinName sensorTop, PinName sensorBottom)
//  ↑ RopeMotor:: sagt "diese Funktion gehört zur Klasse RopeMotor"
    : _motor(motorPin), _sensorTop(sensorTop), _sensorBottom(sensorBottom)
//    ↑ Initialisierungsliste – Hardware-Objekte werden hier mit den Pins erstellt
//      muss so gemacht werden, PwmOut/DigitalIn können nicht "leer" erstellt werden
{
    // Alles was beim Erstellen des Objekts passieren soll
    _sensorTop.mode(PullUp);
    _sensorBottom.mode(PullUp);
    _motor.period(0.02f);
    stop();               // eigene Funktion aufrufen ist erlaubt
}

// Funktionen – immer mit KlassenName:: davor
void RopeMotor::moveUp() {
    _motor.write(0.5f + SPEED * 0.5f);
}

void RopeMotor::moveDown() {
    _motor.write(0.5f - SPEED * 0.5f);
}

void RopeMotor::stop() {
    _motor.write(0.5f);
}

bool RopeMotor::isAtTop() {
    return _sensorTop.read() == 0;   // gibt true zurück wenn Sensor aktiv
}

bool RopeMotor::isAtBottom() {
    return _sensorBottom.read() == 0;
}
