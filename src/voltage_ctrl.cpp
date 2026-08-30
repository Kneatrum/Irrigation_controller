#include "voltage_ctrl.h"
#include "pinout.h"
#include "eeprom_utils.h"


float maxADCvsSystemVoltageFactor = (r1 + r2) / r2;
float resolution = maxADCVoltage / maxRes;




void tmux1208Init(){
    pinMode(TMUX_1208_EN_PIN, OUTPUT);
    pinMode(TMUX_1208_A0_PIN, OUTPUT);
    pinMode(TMUX_1208_A1_PIN, OUTPUT);
    pinMode(TMUX_1208_A2_PIN, OUTPUT);

    digitalWrite(TMUX_1208_A0_PIN, LOW);
    digitalWrite(TMUX_1208_A1_PIN, LOW);
    digitalWrite(TMUX_1208_A2_PIN, LOW);
    /*
     When set to low, the lowest voltage at 
     the V_DRIVE output is below 2v, 
     which is safe enough
    */
   
    digitalWrite(TMUX_1208_EN_PIN, LOW); 
}

void setMuxEnable(bool state){
    digitalWrite(TMUX_1208_EN_PIN, state? HIGH : LOW);
}

void clearAddressBits(){
    setMuxEnable(false);
    digitalWrite(TMUX_1208_A0_PIN, LOW);
    digitalWrite(TMUX_1208_A1_PIN, LOW);
    digitalWrite(TMUX_1208_A2_PIN, LOW);
}

void set5V(){ // S1 on PCB
    digitalWrite(TMUX_1208_A0_PIN, LOW);
    digitalWrite(TMUX_1208_A1_PIN, LOW);
    digitalWrite(TMUX_1208_A2_PIN, LOW);
}

void set6V(){ // S2 on PCB
    digitalWrite(TMUX_1208_A0_PIN, HIGH);
    digitalWrite(TMUX_1208_A1_PIN, LOW);
    digitalWrite(TMUX_1208_A2_PIN, LOW);
}

void set9V(){ // S3 on PCB
    digitalWrite(TMUX_1208_A0_PIN, LOW);
    digitalWrite(TMUX_1208_A1_PIN, HIGH);
    digitalWrite(TMUX_1208_A2_PIN, LOW);
}

void set12V(){ // S4 on PCB
    digitalWrite(TMUX_1208_A0_PIN, HIGH);
    digitalWrite(TMUX_1208_A1_PIN, HIGH);
    digitalWrite(TMUX_1208_A2_PIN, LOW);
}

void set24V(){ // S5 on PCB
    digitalWrite(TMUX_1208_A0_PIN, LOW);
    digitalWrite(TMUX_1208_A1_PIN, LOW);
    digitalWrite(TMUX_1208_A2_PIN, HIGH);
}

void turnOffValveVoltage(){
    // digitalWrite(LM2596_EN, LOW); Implement this on the hardware side in the next version
    clearAddressBits();
    setMuxEnable(false);
}

float readValveVoltage(){
    int adc_out = analogRead(VALVE_V_MEASURE_PIN);
    return resolution * adc_out * maxADCvsSystemVoltageFactor;
}

float readInputVoltage(){
    int adc_out = analogRead(VIN_MEASURE_PIN);
    return resolution * adc_out * maxADCvsSystemVoltageFactor;
}

void setVoltage(ValveVoltage new_voltage, VsetRes_t *status){
    float input_voltage = readInputVoltage();
    uint8_t INT_INPUT_V = ( (uint8_t)round(input_voltage) );
    float valve_voltage;
    float temp_dif = 0.0;
    delay(100);

    switch(new_voltage)
    {
        case FIVE_VOLTS:
            clearAddressBits();
            set5V();
            setMuxEnable(true);
            delay(100);
            valve_voltage = readValveVoltage();


            if((INT_INPUT_V - INT_FIVE_VOLTS) < MIN_VALVE_VOLTAGE_VS_POWER_SUPPLY_DIFFERENCE ){
                *status = VSET_SUPPLY_VOLTAGE_TOO_LOW;
                turnOffValveVoltage();
                break;
            }

            temp_dif = abs(valve_voltage - INT_FIVE_VOLTS);
            
            if( temp_dif <= VOLTAGE_ERROR){
                *status = VSET_SUCCESS;
            }  else {
                turnOffValveVoltage();
                *status = VSET_ERROR;
            }
        break;

        case SIX_VOLTS:
            clearAddressBits();
            set6V();
            setMuxEnable(true);
            delay(100);
            valve_voltage = readValveVoltage();

            if((INT_INPUT_V - INT_SIX_VOLTS) < MIN_VALVE_VOLTAGE_VS_POWER_SUPPLY_DIFFERENCE ){
                *status = VSET_SUPPLY_VOLTAGE_TOO_LOW;
                turnOffValveVoltage();
                break;
            }

            temp_dif = abs(valve_voltage - INT_SIX_VOLTS);

            if(temp_dif <= VOLTAGE_ERROR){
                *status = VSET_SUCCESS; 
            } else {
                turnOffValveVoltage();
                *status = VSET_ERROR; 
            } 
        break;

        case NINE_VOLTS:
            clearAddressBits();
            set9V();
            setMuxEnable(true);
            delay(100);
            valve_voltage = readValveVoltage();

            if((INT_INPUT_V - INT_NINE_VOLTS) < MIN_VALVE_VOLTAGE_VS_POWER_SUPPLY_DIFFERENCE ){
                *status = VSET_SUPPLY_VOLTAGE_TOO_LOW;
                turnOffValveVoltage();
                break;
            }

            temp_dif = abs(valve_voltage - INT_NINE_VOLTS);

            if(abs(valve_voltage - INT_NINE_VOLTS) <= VOLTAGE_ERROR){
                *status = VSET_SUCCESS; 
            } else {
                turnOffValveVoltage();
                *status = VSET_ERROR;
            }
        break;

        case TWELVE_VOLTS:
            clearAddressBits();
            set12V();
            setMuxEnable(true);
            delay(100);
            valve_voltage = readValveVoltage();

            if((INT_INPUT_V - INT_TWELVE_VOLTS) < MIN_VALVE_VOLTAGE_VS_POWER_SUPPLY_DIFFERENCE ){
                *status = VSET_SUPPLY_VOLTAGE_TOO_LOW;
                turnOffValveVoltage();
                break;
            }

            temp_dif = abs(valve_voltage - INT_TWELVE_VOLTS);

            if(temp_dif <= VOLTAGE_ERROR){
                *status = VSET_SUCCESS; 
            } else {
                turnOffValveVoltage();
                *status = VSET_ERROR;
            }
        break;

        case TWENTY_FOUR_VOLTS:
            clearAddressBits();
            set24V();
            setMuxEnable(true);
            delay(100);
            float valve_voltage = readValveVoltage();
            
            if((INT_INPUT_V - INT_TWENTY_FOUR_VOLTS) < MIN_VALVE_VOLTAGE_VS_POWER_SUPPLY_DIFFERENCE ){
                *status = VSET_SUPPLY_VOLTAGE_TOO_LOW;
                turnOffValveVoltage();
                break;
            }

            temp_dif = abs(valve_voltage - INT_TWENTY_FOUR_VOLTS);

            if(temp_dif <= VOLTAGE_ERROR) {
                *status = VSET_SUCCESS;
            } else {
                *status = VSET_ERROR;
                turnOffValveVoltage();
            }
        break;
    }
    
}