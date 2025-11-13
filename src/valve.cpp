#include <Arduino.h>
#include "valve.h"
#include "relays.h"

bool VALVE_STATE = false;

void turnValveOff() {
    setPowerRelay(false);
    setPolarityRelay1(false);
    setPolarityRelay2(false);
    delay(RELAY_STABILIZATION_DELAY_MS); 
    setPowerRelay(true);
    delay(VALVE_GEAR_TURN_DELAY_MS); 
    setPowerRelay(false);
    VALVE_STATE = false;
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
    VALVE_STATE = true;
}