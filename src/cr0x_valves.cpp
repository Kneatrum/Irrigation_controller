#include <Arduino.h>
#include "cr0x_valves.h"
#include "pinout.h"
#include "eeprom_utils.h"

void initializeRelays() {
    pinMode(CR02_CTRL, OUTPUT);
    pinMode(CR03_CTRL, OUTPUT);
    pinMode(CR04_CTRL, OUTPUT);
    
    // Initialize all relays to OFF state
    digitalWrite(CR02_CTRL, LOW);
    digitalWrite(CR03_CTRL, LOW);
    digitalWrite(CR04_CTRL, LOW);
}

void initializeMotorControlPins(){
    pinMode(CR01_EN, OUTPUT);
    pinMode(CR05_EN, OUTPUT);
    pinMode(CR01_1A_CTRL, OUTPUT);
    pinMode(CR01_2A_CTRL, OUTPUT);
    pinMode(CR05_3A_CTRL, OUTPUT);
    pinMode(CR05_4A_CTRL, OUTPUT);
}

void initializeCR05StatusPins(){
    pinMode(CR05_STATUS_CLOSED, INPUT);
    pinMode(CR05_STATUS_OPEN, INPUT);
}

// --------------------------------Control----------------------------------------
VsetRes_t setCR01(bool new_state){
    // Switch polarity
    digitalWrite(CR01_1A_CTRL, new_state); 
    digitalWrite(CR01_2A_CTRL, !new_state); 
    delay(100); // Small delay for stabilization
    digitalWrite(CR01_EN, HIGH); // Output the two polarities
    delay(VALVE_GEAR_TURN_DELAY_MS); // Give time for the motor to turn
    digitalWrite(CR01_EN, LOW);  // Disable the output

    // Set the outputs to LOW
    digitalWrite(CR01_1A_CTRL, LOW); 
    digitalWrite(CR01_2A_CTRL, LOW);
    return VSET_SUCCESS;
}

VsetRes_t setCR02(bool new_state) {
    digitalWrite(CR02_CTRL, new_state ? HIGH : LOW);
    return VSET_SUCCESS;
}

VsetRes_t setCR03(bool new_state) {
    digitalWrite(CR03_CTRL, new_state ? HIGH : LOW);
    return VSET_SUCCESS;
}

VsetRes_t setCR04(bool new_state) {
    digitalWrite(CR04_CTRL, new_state ? HIGH : LOW);
    return VSET_SUCCESS;
}

VsetRes_t setCR05(bool new_state){
    // Switch polarity
    digitalWrite(CR05_3A_CTRL, new_state);
    digitalWrite(CR05_4A_CTRL, !new_state);
    delay(100); // Delay for stabilization
    digitalWrite(CR05_EN, HIGH); // Output the polarity

    unsigned long cr05_timestamp = millis();
    bool cr05_timeout_error = false;

    if(new_state){
        while(digitalRead(CR05_STATUS_OPEN)){
            if((millis() - cr05_timestamp) > (VALVE_GEAR_TURN_DELAY_MS + 1000)){
                cr05_timeout_error = true;
                break;
            }
        }
    } else {
        while(digitalRead(CR05_STATUS_CLOSED)){
            if((millis() - cr05_timestamp) > (VALVE_GEAR_TURN_DELAY_MS + 1000)){
                cr05_timeout_error = true;
                break;
            }
        }
    }

    // delay(VALVE_GEAR_TURN_DELAY_MS); // Wait for the motor to stop turning
    digitalWrite(CR05_EN, LOW);  // Disable the output
     // Set the outputs to LOW
    digitalWrite(CR05_3A_CTRL, LOW); 
    digitalWrite(CR05_4A_CTRL, LOW);

    if(cr05_timeout_error) return VSET_TIMEOUT;
    else return VSET_SUCCESS;
}


void valveInit(){
    initializeRelays();
    initializeMotorControlPins();
    initializeCR05StatusPins();
}