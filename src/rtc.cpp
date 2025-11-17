#include "rtc.h"
#include "eeprom_utils.h"

unsigned long lastTimestamp;
unsigned long currentTimestamp;
RTC_DS3231 rtc;

irrigationTime_t irrigationSchedule;

const char * const monthsOfYear[12] = {
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
};


void initializeRTC() {
    if (!rtc.begin()) {
        // Handle RTC initialization failure
        while (1);
    }

    if (rtc.lostPower()) {
        // Set RTC to a default date and time if it has lost power
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

}

unsigned long getRTCTimestamp() {
    DateTime now = rtc.now();
    return now.unixtime();;
}

int setSystemTime(RTCTime_t time) {
    rtc.adjust(DateTime(
        time.year,
        time.month,
        time.day,
        time.hour,
        time.minute,
        time.second
    ));

    return 0; // Success
}

RTCTime_t getRTCTime() {
    DateTime now = rtc.now();
    RTCTime_t rtcTime;
    rtcTime.year = (uint8_t)(now.year() - 2000);
    rtcTime.month = now.month();
    rtcTime.day = now.day();
    rtcTime.hour = now.hour();
    rtcTime.minute = now.minute();
    rtcTime.second = now.second();
    return rtcTime;
}

// --- Struct operations ---
irrigationTime_t readIrrigationTime() {
    irrigationTime_t time;
    time.startHour   = readStartHour();
    time.startMinute = readStartMinute();
    time.stopHour    = readStopHour();
    time.stopMinute  = readStopMinute();
    return time;
}

bool isIrrigationScheduleSet(){
    irrigationTime_t time;
    time.startHour   = readStartHour();
    time.startMinute = readStartMinute();
    time.stopHour    = readStopHour();
    time.stopMinute  = readStopMinute();
    if( time.startHour == 255 || time.startMinute == 255 || time.stopHour == 255 || time.stopMinute == 255 )  return false;
    return true;
}

int writeIrrigationTime(const irrigationTime_t *time) {
    int result;
    if ((result = writeStartHour(time->startHour))   != EEPROM_SUCCESS) return result;
    if ((result = writeStartMinute(time->startMinute)) != EEPROM_SUCCESS) return result;
    if ((result = writeStopHour(time->stopHour))     != EEPROM_SUCCESS) return result;
    if ((result = writeStopMinute(time->stopMinute)) != EEPROM_SUCCESS) return result;
    return EEPROM_SUCCESS;
}


bool isSecondsBeforeIrrigationEvent(
    irrigationTime_t *irrigationTime,
    uint16_t secondsBefore,
    bool checkStartEvent   // true = check start time, false = check stop time
) {
    DateTime now = rtc.now();
    unsigned long currentTimestamp = now.unixtime();

    // Select start or stop time based on user choice
    uint8_t targetHour   = checkStartEvent ? irrigationTime->startHour  : irrigationTime->stopHour;
    uint8_t targetMinute = checkStartEvent ? irrigationTime->startMinute : irrigationTime->stopMinute;

    // Build today's event time
    DateTime eventTime(
        now.year(), 
        now.month(), 
        now.day(),
        targetHour,
        targetMinute,
        0
    );

    unsigned long eventTimestamp = eventTime.unixtime();

    // If event time already passed, move it to next day
    if (eventTimestamp < currentTimestamp) {
        eventTimestamp += 86400;  // add 24 hours
    }

    // Check if we are within [eventTimestamp - secondsBefore, eventTimestamp)
    return (currentTimestamp >= (eventTimestamp - secondsBefore) &&
            currentTimestamp < eventTimestamp);
}



bool isWithinIrrigationTime(irrigationTime_t *irrigationTime) {
    DateTime now = rtc.now();
    unsigned long currentTimestamp = now.unixtime();


    // if( irrigationTime->startHour == 255 || 
    //     irrigationTime->startMinute == 255 ||
    //     irrigationTime->stopHour == 255 ||
    //     irrigationTime->stopMinute == 255){
    //     return false;
    // }
    
    // Create DateTime objects for start and stop times today
    DateTime startTime( now.year(), 
                        now.month(), 
                        now.day(), 
                        irrigationTime->startHour, 
                        irrigationTime->startMinute, 
                        0);

    DateTime stopTime(  now.year(), 
                        now.month(), 
                        now.day(), 
                        irrigationTime->stopHour, 
                        irrigationTime->stopMinute, 
                        0);
    
    unsigned long startTimestamp = startTime.unixtime();
    unsigned long stopTimestamp = stopTime.unixtime();
    
    // Handle case where stop time is past midnight (next day)
    if (stopTimestamp <= startTimestamp) {
        stopTimestamp += 86400; // Add 24 hours in seconds
    }
    
    // Check if current time is within the range
    if (startTimestamp <= stopTimestamp) {
        // Normal case: start and stop on same day
        return (currentTimestamp >= startTimestamp && currentTimestamp <= stopTimestamp);
    } else {
        // This shouldn't happen with the above adjustment, but kept for safety
        return (currentTimestamp >= startTimestamp || currentTimestamp <= stopTimestamp);
    }
}

uint8_t daysInMonth(uint8_t year_suffix, uint8_t month) {
  switch (month) {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
      return 31;
    case 4: case 6: case 9: case 11:
      return 30;
    case 2:{
      // Leap year check
      uint16_t year = 2000 + year_suffix;
      if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
        return 29;
      else
        return 28;
    }
    default:
      return 0;  // invalid month
  }
}



