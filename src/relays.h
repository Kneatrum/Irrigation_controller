#ifndef RELAYS_H
#define RELAYS_H

#define RELAY_STABILIZATION_DELAY_MS 100
#define VALVE_GEAR_TURN_DELAY_MS 5000

void initializeRelays();
void setPowerRelay(bool state);
void setPolarityRelay1(bool state);
void setPolarityRelay2(bool state);

#endif