#include "Lid.h"

Lid::Lid(PinName motorPin, PinName sensorTop, PinName sensorBottom)
    : _motor(motorPin), _sensorOpen(sensorTop), _sensorClose(sensorBottom) {
}

void Lid::openLid() {
    _motor.write(SPEED);
}

void Lid::closeLid() {
    _motor.write(0.0f);
}

void Lid::stopLid() {
    _motor.write(0.0f);
}

bool Lid::isOpen() {
    return _sensorOpen.read();
}

bool Lid::isClose() {
    return _sensorClose.read();
}