#ifndef GLCD_H
#define GLCD_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include "rtc.h"
#include "states.h"

extern U8G2_ST7920_128X64_F_SW_SPI u8g2;

#define VISIBLE_ITEMS   ((64 - TOP_MARGIN) / ITEM_HEIGHT)   // e.g. ~4 items on 64px screen
#define SCROLL_SPEED    20 

static const uint8_t ITEM_HEIGHT = 14;  // pixel height per line
static const uint8_t TOP_MARGIN = 12;   // top padding

extern bool updateScreenFlag;

extern bool displayOn;

void showInitialisationMessage(StateMachine sm);
void initLCD();
void turnOnBacklight();
void turnOffBacklight();
void drawMenu(StateMachine sm, menuItem_t *items);
void turnOffDisplay();
void turnOnDisplay();
void showDetailsScreen(RTCTime_t currentTime, StateMachine sm);
void showIrrigationTimeSetting(ScheduleTime_t* schedule, IrrigationTimeEditingField editingField);
void showSystemTimeSetting(RTCTime_t* systemTime, const char * const Months[], SystemTimeEditingField editingField);
void promptUserSaveNewSchedule(ScheduleTime_t schedule, StateMachine sm);
void promptUserSaveSystemTime(RTCTime_t newSystemTime, StateMachine sm);
void promptUserSaveNewValveType(StateMachine sm, ValveType *temporary_valve_type);
void promptUserSaveNewVoltage(StateMachine sm, ValveVoltage *temporary_voltage);
void showVoltage(const char *const *valve_voltage_names, ValveVoltage *temporary_valve_voltage) ;
void showValveType(const char *const *valve_type_names, ValveType *temporary_valve_type);

void showSchedules(StateMachine sm);
void showMessage(const char *text);
#endif