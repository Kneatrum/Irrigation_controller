#ifndef VOLTAGE_CTRL_H
#define VOLTAGE_CTRL_H
#include <stdint.h>
#include "states.h"

#define r1  100
#define r2  10
#define maxADCVoltage  3.3
#define maxRes 1023
#define maxSystemVoltage 36.0

#define VOLTAGE_ERROR  1.0

#define INT_FIVE_VOLTS 5
#define INT_SIX_VOLTS 6
#define INT_NINE_VOLTS 9
#define INT_TWELVE_VOLTS 12
#define INT_TWENTY_FOUR_VOLTS 24

void tmux1208Init();
void setMuxEnable(bool state);
void clearAddressBits();
void set5V();
void set6V();
void set9V();
void set12V();
void set24V();
void turnOffValveVoltage();
float readValveVoltage();
float readInputVoltage();
void setVoltage(ValveVoltage new_voltage, bool *status);

#endif