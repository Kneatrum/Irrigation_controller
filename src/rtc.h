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
} irrigationTime_t;


extern irrigationTime_t irrigationSchedule;

void initializeRTC();

// Get timestamp from RTC and set system time
unsigned long getRTCTimestamp();

int setSystemTime(RTCTime_t time);

RTCTime_t getRTCTime();

irrigationTime_t readIrrigationTime();
bool isIrrigationScheduleSet();
int writeIrrigationTime(const irrigationTime_t *time);
bool isSecondsBeforeIrrigationEvent(
    irrigationTime_t *irrigationTime,
    uint16_t secondsBefore,
    bool checkStartEvent   // true = check start time, false = check stop time
) ;
bool isWithinIrrigationTime(irrigationTime_t *irrigationTime);

#endif