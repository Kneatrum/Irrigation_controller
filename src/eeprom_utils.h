#ifndef EEPROM_UTILS_H
#define EEPROM_UTILS_H

#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_SIZE 512
#define MAX_EEPROM_ADDRESS (EEPROM_SIZE - 1)
#define MIN_EEPROM_ADDRESS 0
#define EEPROM_ERROR_INVALID_ADDRESS -1
#define EEPROM_ERROR_INVALID_LENGTH -2
#define EEPROM_SUCCESS 0

#define START_HOUR_ADDRESS 0
#define START_MINUTE_ADDRESS 1
#define STOP_HOUR_ADDRESS 2
#define STOP_MINUTE_ADDRESS 3

#define VALVE_STATE_ADDRESS 8
#define DISPLAY_DETAILS_TIMEOUT_ADDRESS 9
#define CONFIGURE_DETAILS_TIMEOUT_ADDRESS 10
#define FORCE_STOP_ADDRESS 20


// Low-level EEPROM operations
int writeEEPROM(uint8_t address, const uint8_t *data, size_t length);
uint8_t readEEPROM(uint8_t address, size_t length);

// Inline single-byte helpers
inline int writeStartHour(uint8_t value) { return writeEEPROM(START_HOUR_ADDRESS, &value, 1); }
inline uint8_t readStartHour() { return readEEPROM(START_HOUR_ADDRESS, 1); }

inline int writeStartMinute(uint8_t value) { return writeEEPROM(START_MINUTE_ADDRESS, &value, 1); }
inline uint8_t readStartMinute() { return readEEPROM(START_MINUTE_ADDRESS, 1); }

inline int writeStopHour(uint8_t value) { return writeEEPROM(STOP_HOUR_ADDRESS, &value, 1); }
inline uint8_t readStopHour() { return readEEPROM(STOP_HOUR_ADDRESS, 1); }

inline int writeStopMinute(uint8_t value) { return writeEEPROM(STOP_MINUTE_ADDRESS, &value, 1); }
inline uint8_t readStopMinute() { return readEEPROM(STOP_MINUTE_ADDRESS, 1); }

inline int writeValveState(bool state) {
    uint8_t val = state ? 1 : 0;
    return writeEEPROM(VALVE_STATE_ADDRESS, &val, 1);
}

inline bool readValveState() { return readEEPROM(VALVE_STATE_ADDRESS, 1) != 0; }

// Prototypes for multi-byte data
int writeDisplayDetailsTimeout(uint32_t timeout);
uint32_t readDisplayDetailsTimeout();

int writeConfigureDetailsTimeout(uint32_t timeout);
uint32_t readConfigureDetailsTimeout();

int writeForceStopStatus(bool force_stop_status);

bool readForceStopStatus();

#endif
