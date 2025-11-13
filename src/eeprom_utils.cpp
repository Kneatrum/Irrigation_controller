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
