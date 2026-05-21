#ifndef EEPROM_UTILS_H
#define EEPROM_UTILS_H

#include <Arduino.h>
#include <EEPROM.h>
#include "voltage_ctrl.h"

#define EEPROM_SIZE 512
#define MAX_EEPROM_ADDRESS (EEPROM_SIZE - 1)
#define MIN_EEPROM_ADDRESS 0
#define EEPROM_ERROR_INVALID_ADDRESS -1
#define EEPROM_ERROR_INVALID_LENGTH -2
#define EEPROM_OPERATION_NOT_ALLOWED -3
#define EEPROM_SUCCESS 0
#define EEPROM_ERROR 1

#define SCHEDULE_1_START_HOUR_ADDRESS 0
#define SCHEDULE_1_START_MINUTE_ADDRESS 1
#define SCHEDULE_1_STOP_HOUR_ADDRESS 2
#define SCHEDULE_1_STOP_MINUTE_ADDRESS 3
#define SCHEDULE_1_ENABLED_STATUS_ADDRESS 4

#define SCHEDULE_2_START_HOUR_ADDRESS 5
#define SCHEDULE_2_START_MINUTE_ADDRESS 6
#define SCHEDULE_2_STOP_HOUR_ADDRESS 7
#define SCHEDULE_2_STOP_MINUTE_ADDRESS 8
#define SCHEDULE_2_ENABLED_STATUS_ADDRESS 9

#define SCHEDULE_3_START_HOUR_ADDRESS 10
#define SCHEDULE_3_START_MINUTE_ADDRESS 11
#define SCHEDULE_3_STOP_HOUR_ADDRESS 12
#define SCHEDULE_3_STOP_MINUTE_ADDRESS 13
#define SCHEDULE_3_ENABLED_STATUS_ADDRESS 14

#define SCHEDULE_4_START_HOUR_ADDRESS 15
#define SCHEDULE_4_START_MINUTE_ADDRESS 16
#define SCHEDULE_4_STOP_HOUR_ADDRESS 17
#define SCHEDULE_4_STOP_MINUTE_ADDRESS 18
#define SCHEDULE_4_ENABLED_STATUS_ADDRESS 19

#define VALVE_STATE_ADDRESS 20
#define DISPLAY_DETAILS_TIMEOUT_ADDRESS 21
#define CONFIGURE_DETAILS_TIMEOUT_ADDRESS 22
#define FORCE_STOP_ADDRESS 23

#define SELECTED_VALVE_ADDRESS 24
#define SELECTED_VOLTAGE_ADDRESS 25

#define VALVE_SELECTED_ADDRESS 26
#define VOLTAGE_SELECTED_ADDRESS 27

#define SCHEDULE_1_EN_ADDRESS 28
#define SCHEDULE_2_EN_ADDRESS 29
#define SCHEDULE_3_EN_ADDRESS 30
#define SCHEDULE_4_EN_ADDRESS 31


// Low-level EEPROM operations
int writeEEPROM(uint8_t address, const uint8_t *data, size_t length);
uint8_t readEEPROM(uint8_t address, size_t length);

// Inline single-byte helpers
void writeStartHour(uint8_t hour, uint8_t schedule);
uint8_t readStartHour(uint8_t schedule);

void writeStartMinute(uint8_t minute, uint8_t schedule);
uint8_t readStartMinute(uint8_t schedule);

void writeStopHour(uint8_t hour, uint8_t schedule);
uint8_t readStopHour(uint8_t schedule);

void writeStopMinute(uint8_t minute, uint8_t schedule);
uint8_t readStopMinute(uint8_t schedule);

void writeValveState(bool state);

bool readValveState();

// Prototypes for multi-byte data
int writeDisplayDetailsTimeout(uint32_t timeout);
uint32_t readDisplayDetailsTimeout();

int writeConfigureDetailsTimeout(uint32_t timeout);
uint32_t readConfigureDetailsTimeout();

void writeForceStopStatus(bool force_stop_status);

bool readForceStopStatus();

ValveType readSelectedValve();
bool saveSelectedValve(ValveType selectedValve);

ValveVoltage readSelectedVoltage();
int saveSelectedVoltage(ValveVoltage slected_voltage);
void saveValveSelectionState(bool selected);
void saveVoltageSelectionState(bool selected);
bool readVoltageSelectionState();
bool readValveSelectionState();
bool writeEnableStatus(uint8_t schedule, bool enable);
bool readEnableStatus(uint8_t schedule);

#endif
