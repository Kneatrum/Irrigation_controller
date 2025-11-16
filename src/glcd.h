#ifndef GLCD_H
#define GLCD_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include "rtc.h"
#include "states.h"

extern U8G2_ST7920_128X64_F_HW_SPI u8g2;

static const uint8_t ITEM_HEIGHT = 14;  // pixel height per line
static const uint8_t TOP_MARGIN = 12;   // top padding

extern bool updateScreenFlag;

extern bool displayOn;

void initLCD();
void turnOnBacklight();
void turnOffBacklight();
void drawMenu(uint8_t selectedIndex, bool irrigationActive);
void turnOffDisplay();
void turnOnDisplay();
void showDetailsScreen(RTCTime_t* currentTime, irrigationTime_t* schedule, bool* valveState);
void showIrrigationTimeSetting(irrigationTime_t* schedule, IrrigationTimeEditingField editingField);
void showSystemTimeSetting(systemTime_t* systemTime, const char * const Months[], SystemTimeEditingField editingField);
void promptUserSaveNewSchedule(irrigationTime_t schedule, StateMachine sm);
void promptUserSaveSystemTime(RTCTime_t newSystemTime, StateMachine sm);
#endif