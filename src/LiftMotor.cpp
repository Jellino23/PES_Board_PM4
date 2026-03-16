#include "LiftMotor.h"    // Eigenen Header einbinden – kennt damit alle Deklarationen
                          // kein #pragma once nötig

// Konstruktor
LiftMotor::LiftMotor(PinName servoLiftPin, PinName sensorTop, PinName sensorBottom)
//  ↑ LiftMotor:: sagt "diese Funktion gehört zur Klasse LiftMotor"
    : _servoLift(servoLiftPin), _sensorTop(sensorTop), _sensorBottom(sensorBottom)
//    ↑ Initialisierungsliste – Hardware-Objekte werden hier mit den Pins erstellt
//      muss so gemacht werden, PwmOut/DigitalIn können nicht "leer" erstellt werden
{
    // Alles was beim Erstellen des Objekts passieren soll
    _sensorTop.mode(PullUp);
    _sensorBottom.mode(PullUp);
    _servoLift.period(0.02f);
    stop();               // eigene Funktion aufrufen ist erlaubt
}

// Funktionen – immer mit KlassenName:: davor
void LiftMotor::moveUp() {
    _servoLift.write(0.5f + SPEED * 0.5f);
}

void LiftMotor::moveDown() {
    _motor.write(0.5f - SPEED * 0.5f);
}

void LiftMotor::stop() {
    _motor.write(0.5f);
}

bool LiftMotor::isAtTop() {
    return _sensorTop.read() == 0;   // gibt true zurück wenn Sensor aktiv
}

bool LiftMotor::isAtBottom() {
    return _sensorBottom.read() == 0;
}
