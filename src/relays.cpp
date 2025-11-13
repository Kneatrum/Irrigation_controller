#include <Arduino.h>
#include "relays.h"
#include "pinout.h"

void initializeRelays() {
    pinMode(POWER_RELAY_PIN, OUTPUT);
    pinMode(POLARITY_RELAY_1_PIN, OUTPUT);
    pinMode(POLARITY_RELAY_2_PIN, OUTPUT);
    
    // Initialize all relays to OFF state
    digitalWrite(POWER_RELAY_PIN, LOW);
    digitalWrite(POLARITY_RELAY_1_PIN, LOW);
    digitalWrite(POLARITY_RELAY_2_PIN, LOW);
}

void setPowerRelay(bool state) {
    digitalWrite(POWER_RELAY_PIN, state ? HIGH : LOW);
}

void setPolarityRelay1(bool state) {
    digitalWrite(POLARITY_RELAY_1_PIN, state ? HIGH : LOW);
}

void setPolarityRelay2(bool state) {
    digitalWrite(POLARITY_RELAY_2_PIN, state ? HIGH : LOW);
}