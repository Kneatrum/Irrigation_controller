#include "eeprom_utils.h"


int writeEEPROM(uint8_t address, const uint8_t *data, size_t length) {
    if (address < MIN_EEPROM_ADDRESS || address + length - 1 > MAX_EEPROM_ADDRESS) {
        return EEPROM_ERROR_INVALID_ADDRESS;
    }
    for (size_t i = 0; i < length; i++) {
        EEPROM.write(address + i, data[i]);
    }
    return EEPROM_SUCCESS;
}

uint8_t readEEPROM(uint8_t address, size_t length) {
    if (address < MIN_EEPROM_ADDRESS || address + length - 1 > MAX_EEPROM_ADDRESS) {
        return EEPROM_ERROR_INVALID_ADDRESS;
    }

    uint8_t value = 0;
    for (size_t i = 0; i < length; i++) {
        value = EEPROM.read(address + i);
    }
    return value;
}

// --- 32-bit timeouts ---
int writeDisplayDetailsTimeout(uint32_t timeout) {
    uint8_t data[4];
    data[0] = (timeout >> 24) & 0xFF;
    data[1] = (timeout >> 16) & 0xFF;
    data[2] = (timeout >> 8) & 0xFF;
    data[3] = timeout & 0xFF;
    return writeEEPROM(DISPLAY_DETAILS_TIMEOUT_ADDRESS, data, 4);
}

uint32_t readDisplayDetailsTimeout() {
    uint8_t data[4];
    for (size_t i = 0; i < 4; i++) {
        data[i] = EEPROM.read(DISPLAY_DETAILS_TIMEOUT_ADDRESS + i);
    }
    return (static_cast<uint32_t>(data[0]) << 24) |
        (static_cast<uint32_t>(data[1]) << 16) |
        (static_cast<uint32_t>(data[2]) << 8) |
        static_cast<uint32_t>(data[3]);
}

int writeConfigureDetailsTimeout(uint32_t timeout) {
    uint8_t data[4];
    data[0] = (timeout >> 24) & 0xFF;
    data[1] = (timeout >> 16) & 0xFF;
    data[2] = (timeout >> 8) & 0xFF;
    data[3] = timeout & 0xFF;
    return writeEEPROM(CONFIGURE_DETAILS_TIMEOUT_ADDRESS, data, 4);
}

uint32_t readConfigureDetailsTimeout() {
    uint8_t data[4];
    for (size_t i = 0; i < 4; i++) {
        data[i] = EEPROM.read(CONFIGURE_DETAILS_TIMEOUT_ADDRESS + i);
    }
    return (static_cast<uint32_t>(data[0]) << 24) |
        (static_cast<uint32_t>(data[1]) << 16) |
        (static_cast<uint32_t>(data[2]) << 8) |
        static_cast<uint32_t>(data[3]);
}


void writeForceStopStatus(bool force_stop_status){
    EEPROM.write(FORCE_STOP_ADDRESS, force_stop_status);
}

bool readForceStopStatus(){
    uint8_t res = EEPROM.read(FORCE_STOP_ADDRESS);
    if(res == 255 || res == 0) return false;
    return true;
}

ValveType readSelectedValve(){
    uint8_t ret = EEPROM.read(SELECTED_VALVE_ADDRESS);
    Serial.print("Saved valve: ");
    Serial.println(ret);
    ValveType valve_type = static_cast<ValveType>(ret);
    switch(valve_type){
        case CR01:
        case CR02:
        case CR03:
        case CR04:
        case CR05:
            return valve_type;
        default:
            return VALVE_NOT_SELECTED;
    }
}

bool saveSelectedValve(ValveType selectedValve){
    switch(selectedValve){
        case CR01:
        case CR02:
        case CR03:
        case CR04:
        case CR05:
            EEPROM.write(SELECTED_VALVE_ADDRESS, ((uint8_t)selectedValve));
            return true;
        default:
            return false;
    }
}

ValveVoltage readSelectedVoltage(){
    uint8_t  ret = EEPROM.read(SELECTED_VOLTAGE_ADDRESS);
    Serial.print("Saved Voltage: ");
    Serial.println(ret);
    ValveVoltage voltage = static_cast<ValveVoltage>(ret);
    switch(voltage){
        case FIVE_VOLTS:
        case SIX_VOLTS:
        case NINE_VOLTS:
        case TWELVE_VOLTS:
        case TWENTY_FOUR_VOLTS:
            return voltage;
        default:
            return VOLTAGE_NOT_SELECTED;
    }
}

int saveSelectedVoltage(ValveVoltage slected_voltage){
    switch(slected_voltage){
        case FIVE_VOLTS:
        case SIX_VOLTS:
        case NINE_VOLTS:
        case TWELVE_VOLTS:
        case TWENTY_FOUR_VOLTS:
            EEPROM.write(SELECTED_VOLTAGE_ADDRESS, ((uint8_t)slected_voltage));
            return true;
        default:
            return false;
    }
}

void saveValveSelectionState(bool selected){
    uint8_t val = selected? 1: 0; 
    EEPROM.write(VALVE_SELECTED_ADDRESS, val);
}

bool readValveSelectionState(){
    uint8_t val = EEPROM.read(VALVE_SELECTED_ADDRESS);
    if( val == 0 ) return false;
    if( val == 1 ) return true;
    return false; 
}

void saveVoltageSelectionState(bool selected){
    uint8_t val = selected? 1: 0; 
    EEPROM.write(VOLTAGE_SELECTED_ADDRESS, val);
}

bool readVoltageSelectionState(){
    uint8_t val = EEPROM.read(VOLTAGE_SELECTED_ADDRESS);
    if( val == 0 ) return false;
    if( val == 1 ) return true;
    return false;
}

bool readValveState(){
    uint8_t val = EEPROM.read(VALVE_STATE_ADDRESS);
    if( val == 0 ) return false;
    if( val == 1 ) return true;
    return false; 
}

void writeValveState(bool state) {
    uint8_t val = state ? 1 : 0;
    EEPROM.write(VALVE_STATE_ADDRESS, val);
}

void writeStartHour(uint8_t hour, uint8_t schedule) {
    switch(schedule){
        case 0:
            EEPROM.write(SCHEDULE_1_START_HOUR_ADDRESS, hour);
        break;
        case 1:
            EEPROM.write(SCHEDULE_2_START_HOUR_ADDRESS, hour);
        break;
        case 2:
            EEPROM.write(SCHEDULE_3_START_HOUR_ADDRESS, hour);
        break;
        case 3:
            EEPROM.write(SCHEDULE_4_START_HOUR_ADDRESS, hour);
        break;
    }
}

uint8_t readStartHour(uint8_t schedule) {
    switch(schedule){
        case 0:
            return readEEPROM(SCHEDULE_1_START_HOUR_ADDRESS, 1); 
        break;
        case 1:
            return readEEPROM(SCHEDULE_2_START_HOUR_ADDRESS, 1); 
        break;
        case 2:
            return readEEPROM(SCHEDULE_3_START_HOUR_ADDRESS, 1); 
        break;
        case 3:
            return readEEPROM(SCHEDULE_4_START_HOUR_ADDRESS, 1); 
        break;
        default:
            return EEPROM_ERROR;
    }
}

void writeStartMinute(uint8_t minute, uint8_t schedule) { 
    switch(schedule){
        case 0:
            EEPROM.write(SCHEDULE_1_START_MINUTE_ADDRESS, minute);
        break;
        case 1:
            EEPROM.write(SCHEDULE_2_START_MINUTE_ADDRESS, minute);
        break;
        case 2:
            EEPROM.write(SCHEDULE_3_START_MINUTE_ADDRESS, minute);
        break;
        case 3:
            EEPROM.write(SCHEDULE_4_START_MINUTE_ADDRESS, minute);
        break;
    }
}

uint8_t readStartMinute(uint8_t schedule) { 
    // return readEEPROM(START_MINUTE_ADDRESS, 1); 
    switch(schedule){
        case 0:
            return readEEPROM(SCHEDULE_1_START_MINUTE_ADDRESS, 1); 
        break;
        case 1:
            return readEEPROM(SCHEDULE_2_START_MINUTE_ADDRESS, 1); 
        break;
        case 2:
            return readEEPROM(SCHEDULE_3_START_MINUTE_ADDRESS, 1); 
        break;
        case 3:
            return readEEPROM(SCHEDULE_4_START_MINUTE_ADDRESS, 1); 
        break;
        default:
            return EEPROM_ERROR;
    }
}

void writeStopHour(uint8_t hour, uint8_t schedule) { 
    switch(schedule){
        case 0:
            EEPROM.write(SCHEDULE_1_STOP_HOUR_ADDRESS, hour);
        break;
        case 1:
            EEPROM.write(SCHEDULE_2_STOP_HOUR_ADDRESS, hour);
        break;
        case 2:
            EEPROM.write(SCHEDULE_3_STOP_HOUR_ADDRESS, hour);
        break;
        case 3:
            EEPROM.write(SCHEDULE_4_STOP_HOUR_ADDRESS, hour);
        break;
    }
}

uint8_t readStopHour(uint8_t schedule) { 
    // return readEEPROM(STOP_HOUR_ADDRESS, 1); 
    switch(schedule){
        case 0:
            return readEEPROM(SCHEDULE_1_STOP_HOUR_ADDRESS, 1); 
        break;
        case 1:
            return readEEPROM(SCHEDULE_2_STOP_HOUR_ADDRESS, 1); 
        break;
        case 2:
            return readEEPROM(SCHEDULE_3_STOP_HOUR_ADDRESS, 1); 
        break;
        case 3:
            return readEEPROM(SCHEDULE_4_STOP_HOUR_ADDRESS, 1); 
        break;
        default:
            return EEPROM_ERROR;
    }
}

void writeStopMinute(uint8_t minute, uint8_t schedule) { 
    switch(schedule){
        case 0:
            EEPROM.write(SCHEDULE_1_STOP_MINUTE_ADDRESS, minute); 
        break;
        case 1:
            EEPROM.write(SCHEDULE_2_STOP_MINUTE_ADDRESS, minute); 
        break;
        case 2:
            EEPROM.write(SCHEDULE_3_STOP_MINUTE_ADDRESS, minute); 
        break;
        case 3:
            EEPROM.write(SCHEDULE_4_STOP_MINUTE_ADDRESS, minute); 
        break;
    }
}

uint8_t readStopMinute(uint8_t schedule) { 
    // return readEEPROM(STOP_MINUTE_ADDRESS, 1); 
    switch(schedule){
        case 0:
            return readEEPROM(SCHEDULE_1_STOP_MINUTE_ADDRESS, 1); 
        break;
        case 1:
            return readEEPROM(SCHEDULE_2_STOP_MINUTE_ADDRESS, 1); 
        break;
        case 2:
            return readEEPROM(SCHEDULE_3_STOP_MINUTE_ADDRESS, 1); 
        break;
        case 3:
            return readEEPROM(SCHEDULE_4_STOP_MINUTE_ADDRESS, 1); 
        break;
        default:
            return EEPROM_ERROR;
    }
}

bool writeEnableStatus(uint8_t schedule, bool enable){
    uint8_t enable_status = enable? 1 : 0;
    switch(schedule){
        case 0:
            EEPROM.write(SCHEDULE_1_EN_ADDRESS, enable_status);
            return true;
        break;
        case 1:
            EEPROM.write(SCHEDULE_2_EN_ADDRESS, enable_status);
            return true;
        break;
        case 2:
            EEPROM.write(SCHEDULE_3_EN_ADDRESS, enable_status);
            return true;
        break;
        case 3:
            EEPROM.write(SCHEDULE_4_EN_ADDRESS, enable_status);
            return true;
        break;
        default:
            return false;
    }
}

bool readEnableStatus(uint8_t schedule){
    uint8_t res = 255;
    switch(schedule){
        case 0:
            res = EEPROM.read(SCHEDULE_1_EN_ADDRESS);
            if(res == 0) return false;
            if(res > 1) return false;
            return true;
        break;
        case 1:
            res = EEPROM.read(SCHEDULE_2_EN_ADDRESS);
            if(res == 0) return false;
            if(res > 1) return false;
            return true;
        break;
        case 2:
            res = EEPROM.read(SCHEDULE_3_EN_ADDRESS);
            if(res == 0) return false;
            if(res > 1) return false;
            return true;
        break;
        case 3:
            res = EEPROM.read(SCHEDULE_4_EN_ADDRESS);
            if(res == 0) return false;
            if(res > 1) return false;
            return true;
        break;
        default:
            return EEPROM_ERROR;
    }
}