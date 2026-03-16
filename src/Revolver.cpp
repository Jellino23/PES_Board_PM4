#include "Revolver.h"

// Konstruktor
Revolver::Revolver(PinName servoRevolverPin, PinName sensorVial, PinName sensorHole)
    : _servoRevolver(servoRevolverPin), _sensorVial(sensorVial), _sensorHole(sensorHole)
{
    _sensorVial.mode(PullUp);
    _sensorHole.mode(PullUp);
    if (!_servoRevolver.isEnabled())
        _servoRevolver.enable(homing());
    stop();
}

// Funktionen – immer mit KlassenName:: davor
void Revolver::turnCW() {
    _servoRevolver.write(0.5f + SPEED * 0.5f);
}

void Revolver::turnCCW() {
    _servoRevolver.write(0.5f - SPEED * 0.5f);
}

void Revolver::stop() {
    _servoRevolver.write(0.5f);
}

bool Revolver::isAtVial() {
    return _sensorVial.read() == 0;
}

bool Revolver::isAtHole() {
    return _sensorHole.read() == 0;
}
