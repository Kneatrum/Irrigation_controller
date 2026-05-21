#ifndef CR0X_VALVES_H
#define CR0X_VALVES_H

#define RELAY_STABILIZATION_DELAY_MS 100
#define VALVE_GEAR_TURN_DELAY_MS 7000
#include <stdint.h>
#include "states.h"

void initializeRelays();
void initializeMotorControlPins();
void initializeCR05StatusPins();
VsetRes_t setCR01(bool new_state);
VsetRes_t setCR02(bool new_state);
VsetRes_t setCR03(bool new_state);
VsetRes_t setCR04(bool new_state);
VsetRes_t setCR05(bool new_state);
void valveInit();

#endif