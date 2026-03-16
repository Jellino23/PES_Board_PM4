#include "Lid.h"

const float gear_ratio_lidMotor = 100.0f; //Übersetzungsverhältnis
const float kn_lidMotor = 140.0f / 12.0f; //Motorkonstante [rpm/V]
const float voltage_max = 12.0f;

Lid::Lid(PinName lidMotorPWM, PinName lidMotorEncA, PinName lidMotorEncB, PinName sensorOpen, PinName sensorClose)
    : _lidMotor(lidMotorPWM, lidMotorEncA, lidMotorEncB, gear_ratio_lidMotor, kn_lidMotor, voltage_max), _sensorOpen(sensorOpen), _sensorClose(sensorClose) 


void Lid::openLid() {
    _lidMotor.setVelocity(SPEED);
}

void Lid::closeLid() {
    _lidMotor.setVelocity(0.0f);
}

void Lid::stopLid() {
    _lidMotor.setVelocity(0.0f);
}

bool Lid::isOpen() {
    return _sensorOpen.read() == 0;
}

bool Lid::isClose() {
    return _sensorClose.read() == 0;
}

