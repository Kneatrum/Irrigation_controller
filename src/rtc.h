#ifndef RTC_H
#define RTC_H

#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"

extern unsigned long lastTimestamp;
extern unsigned long currentTimestamp;
extern RTC_DS3231 rtc;
extern const char * const monthsOfYear[12];


typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} systemTime_t ;

typedef struct {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} RTCTime_t;


typedef struct {
    uint8_t startHour;
    uint8_t startMinute;
    uint8_t stopHour;
    uint8_t stopMinute;
} ScheduleTime_t;

typedef struct {
    ScheduleTime_t schedule_time;
    bool time_is_set;
    bool enabled; // Meaning it has no effect even if it's time for irrigation
    bool active; // Meaning it's time for irrigation
    uint8_t selected_option;
    const char ** schedule_options;
} IrrigationSchedule_t;


// extern IrrigationSchedule_t irrigationSchedule;

void initializeRTC();

// Get timestamp from RTC and set system time
unsigned long getRTCTimestamp();

int setSystemTime(RTCTime_t time);

RTCTime_t getRTCTime();

ScheduleTime_t readIrrigationSchedule(uint8_t schedule);
bool isIrrigationScheduleSet(uint8_t schedule);
void writeIrrigationTime(ScheduleTime_t time, uint8_t schedule);
bool isSecondsBeforeIrrigationEvent(
    ScheduleTime_t irrigationTime,
    uint16_t secondsBefore,
    bool checkStartEvent   // true = check start time, false = check stop time
) ;
bool isWithinIrrigationTime(ScheduleTime_t irrigationTime);
uint8_t daysInMonth(uint8_t year_suffix, uint8_t month);


#endif