#include <Arduino.h>
#include "valve.h"
#include "relays.h"
#include "states.h"



void turnValveOff() {
    setPowerRelay(false);
    setPolarityRelay1(false);
    setPolarityRelay2(false);
    delay(RELAY_STABILIZATION_DELAY_MS); 
    setPowerRelay(true);
    delay(VALVE_GEAR_TURN_DELAY_MS); 
    setPowerRelay(false);
    stateMachine.valve_on = false;
}

void turnValveOn() {
    setPowerRelay(false);
    setPolarityRelay1(true);
    setPolarityRelay2(true);
    delay(RELAY_STABILIZATION_DELAY_MS); 
    setPowerRelay(true);
    delay(VALVE_GEAR_TURN_DELAY_MS); 
    setPowerRelay(false);
    setPolarityRelay1(false);
    setPolarityRelay2(false);
    stateMachine.valve_on = true;
}