#include "Revolver.h"

// Konstruktor
Revolver::Revolver(PinName motorPin, PinName sensorVial, PinName sensorHole)
    : _motor(motorPin), _sensorVial(sensorVial), _sensorHole(sensorHole)
{
    _sensorVial.mode(PullUp);
    _sensorHole.mode(PullUp);
    _motor.period(0.02f);
    stop();
}

// Funktionen – immer mit KlassenName:: davor
void Revolver::turnCW() {
    _motor.write(0.5f + SPEED * 0.5f);
}

void Revolver::turnCCW() {
    _motor.write(0.5f - SPEED * 0.5f);
}

void Revolver::stop() {
    _motor.write(0.5f);
}

bool Revolver::isAtVial() {
    return _sensorVial.read() == 0;
}

bool Revolver::isAtHole() {
    return _sensorHole.read() == 0;
}
