#include "states.h"
#include "rtc.h"
#include "rotary_enc.h"
#include "eeprom_utils.h"


// Define the menu items array here
const char * const MENU_ITEMS[] = {
  "Set Irrigation Time",
  "Set System Time",
  "Force stop",  // Should only appear if irrigation is active
  "Exit"
};


// Optionally expose the number of menu items
const unsigned int MENU_ITEMS_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);


StateMachine stateMachine = {
  .currentState = STATE_IDLE,
  .currentConfigSubstate = IRRIGATION_TIME_NAVIGATION,
  .currentSystemTimeField = FIELD_HOURS,
  .currentIrrigationTimeField = IRRIGATION_START_HOUR,
  .CONFIG_DONE =false,
  .currentNotification = NONE,
  .USER_CONFIRMS = false,
  .activeMenuIndex = 0,
  .CONFIGURE_DETAILS_TIMEOUT = DEFAULT_TIMEOUT_MS,
  .DISPLAY_DETAILS_TIMEOUT = DEFAULT_TIMEOUT_MS,
  .MENU_NAV_STATE_TIMEOUT = DEFAULT_TIMEOUT_MS,
  .CONFIG_STATE_COUNTER = 0,
  .SHOW_DETAILS_COUNTER = 0,
  .MENU_NAV_STATE_COUNTER = 0,
  .force_stop = false,
  .valve_on = false
};


static void resetSubstates(){
  stateMachine.currentConfigSubstate = IRRIGATION_TIME_NAVIGATION;
  stateMachine.currentSystemTimeField = FIELD_HOURS;
  stateMachine.currentIrrigationTimeField = IRRIGATION_START_HOUR;
}

void setIdleState(){
  stateMachine.currentState = STATE_IDLE;
  resetSubstates();
  stateMachine.CONFIG_DONE = true;
  stateMachine.USER_CONFIRMS = false;
  stateMachine.currentNotification = NONE;
  stateMachine.activeMenuIndex = 0;
  stateMachine.CONFIG_STATE_COUNTER = 0;
  stateMachine.SHOW_DETAILS_COUNTER = 0;
  stateMachine.MENU_NAV_STATE_COUNTER = 0;
}

void setShowingDetailsState(){
  stateMachine.currentState = STATE_SHOWING_DETAILS;
  resetSubstates();
  stateMachine.CONFIG_DONE = true;
  stateMachine.USER_CONFIRMS = false;
  stateMachine.currentNotification = NONE;
  stateMachine.activeMenuIndex = 0;
  stateMachine.CONFIG_STATE_COUNTER = 0;
  stateMachine.SHOW_DETAILS_COUNTER = 0;
  stateMachine.MENU_NAV_STATE_COUNTER = 0;
}

void setMenuNavigationState(){
  stateMachine.currentState = STATE_MENU_NAVIGATION;
  resetSubstates();
  stateMachine.CONFIG_DONE = true;
  stateMachine.USER_CONFIRMS = false;
  stateMachine.currentNotification = NONE;
  stateMachine.activeMenuIndex = 0;
  stateMachine.CONFIG_STATE_COUNTER = 0;
  stateMachine.SHOW_DETAILS_COUNTER = 0;
  stateMachine.MENU_NAV_STATE_COUNTER = 0;
}

void setConfiguringState(){
  stateMachine.currentState = STATE_CONFIGURING;
  resetSubstates();
  stateMachine.CONFIG_DONE = true;
  stateMachine.USER_CONFIRMS = false;
  stateMachine.currentNotification = NONE;
  stateMachine.activeMenuIndex = 0;
  stateMachine.CONFIG_STATE_COUNTER = 0;
  stateMachine.SHOW_DETAILS_COUNTER = 0;
  stateMachine.MENU_NAV_STATE_COUNTER = 0;
}

void configureIrrigationTime(StateMachine* sm, irrigationTime_t* schedule) {
  // Implementation for configuring irrigation time
  // This function should handle the logic for setting start and stop times
  // based on the current field being edited in the state machine.
  switch(sm->currentIrrigationTimeField) {
    case IRRIGATION_START_HOUR:
      sm->currentIrrigationTimeField = IRRIGATION_START_MINUTE;
      break;
    case IRRIGATION_START_MINUTE:
      sm->currentIrrigationTimeField = IRRIGATION_STOP_HOUR;
      break;
    case IRRIGATION_STOP_HOUR:
      sm->currentIrrigationTimeField = IRRIGATION_STOP_MINUTE;
      break;
    case IRRIGATION_STOP_MINUTE:
      sm->currentIrrigationTimeField = IRRIGATION_START_HOUR;
      break;
    default:
      break;
  }

}


void configureSystemTime(StateMachine* sm, RTCTime_t* newTime) {
  // Implementation for configuring system time
  // This function should handle the logic for setting the system time
  // based on the current field being edited in the state machine.
  switch(sm->currentSystemTimeField) {
    case FIELD_YEAR:
      sm->currentSystemTimeField = FIELD_MONTH;
      break;
    case FIELD_MONTH:
      sm->currentSystemTimeField = FIELD_DAY;
    break;
    case FIELD_DAY:
      sm->currentSystemTimeField = FIELD_HOURS;
    break;
    case FIELD_HOURS:
      sm->currentSystemTimeField = FIELD_MINUTES;
    break;
    case FIELD_MINUTES:
      sm->currentSystemTimeField = FIELD_SECONDS;
    break;
    case FIELD_SECONDS:
      sm->currentSystemTimeField = FIELD_YEAR;
      break;
    default:
      break;
  }
}

void handleIrrigationTimeField(IrrigationTimeEditingField *field, irrigationTime_t *irrigationSchedule){
  switch(*field) {
    case IRRIGATION_START_HOUR:
      if (clockwiseTurn) {
        clockwiseTurn = false;
        irrigationSchedule->startHour = (irrigationSchedule->startHour + 1) % 24;
      } else {
        counterClockwiseTurn = false;
        irrigationSchedule->startHour = (irrigationSchedule->startHour == 0) ? 23 : (irrigationSchedule->startHour - 1);
      }
      break;
    case IRRIGATION_START_MINUTE:
      if (clockwiseTurn) {
        clockwiseTurn = false;
        irrigationSchedule->startMinute = (irrigationSchedule->startMinute + 1) % 60;
      } else {
        counterClockwiseTurn = false;
        irrigationSchedule->startMinute = (irrigationSchedule->startMinute == 0) ? 59 : (irrigationSchedule->startMinute - 1);
      }
      break;
    case IRRIGATION_STOP_HOUR:
      if (clockwiseTurn) {
        clockwiseTurn = false;
        irrigationSchedule->stopHour = (irrigationSchedule->stopHour + 1) % 24;
      } else {
        counterClockwiseTurn = false;
        irrigationSchedule->stopHour = (irrigationSchedule->stopHour == 0) ? 23 : (irrigationSchedule->stopHour - 1);
      }
      break;
    case IRRIGATION_STOP_MINUTE:
      if (clockwiseTurn) {
        clockwiseTurn = false;
        irrigationSchedule->stopMinute = (irrigationSchedule->stopMinute + 1) % 60;
      } else {
        counterClockwiseTurn = false;
        irrigationSchedule->stopMinute = (irrigationSchedule->stopMinute == 0) ? 59 : (irrigationSchedule->stopMinute - 1);
      }
      break;
    default:
      break;
  }
}

void handleSystemTimeField(SystemTimeEditingField * field, RTCTime_t * newSystemTime) {
  switch(*field) {
    case FIELD_YEAR:
      if (clockwiseTurn) {
        clockwiseTurn = false;
        newSystemTime->year++;
        if(newSystemTime->year >= 100) newSystemTime->year = 0;
      } else {
        counterClockwiseTurn = false;
        newSystemTime->year--;
        if(newSystemTime->year >= 255) newSystemTime->year = 99;
      }
      break;
    case FIELD_MONTH:
      if (clockwiseTurn) {
        clockwiseTurn = false;
        newSystemTime->month = (newSystemTime->month + 1) % 12;
      } else {
        counterClockwiseTurn = false;
        newSystemTime->month = (newSystemTime->month == 0) ? 11 : (newSystemTime->month - 1);
      }
      break;
    case FIELD_DAY: {
      uint8_t max_days = daysInMonth(newSystemTime->year, newSystemTime->month + 1); 

      // Ensure day is within 1..max_days
      if (newSystemTime->day < 1) newSystemTime->day = 1;
      if (newSystemTime->day > max_days) newSystemTime->day = max_days;

      if (clockwiseTurn) {
        clockwiseTurn = false;
        // Increment with wrap
        newSystemTime->day++;
        if (newSystemTime->day > max_days) newSystemTime->day = 1;
      } else {
        counterClockwiseTurn = false;
        // Decrement with wrap
        if (newSystemTime->day <= 1) {
          newSystemTime->day = max_days;
        } else {
          newSystemTime->day--;
        }
      }
      break;
    }
    case FIELD_HOURS:
      if (clockwiseTurn) {
        clockwiseTurn = false;
        newSystemTime->hour = (newSystemTime->hour + 1) % 24;
      } else {
        counterClockwiseTurn = false;
        newSystemTime->hour = (newSystemTime->hour == 0) ? 23 : (newSystemTime->hour - 1);
      }
      break;
    case FIELD_MINUTES:
      if (clockwiseTurn) {
        clockwiseTurn = false;
        newSystemTime->minute = (newSystemTime->minute + 1) % 60;
      } else {
        counterClockwiseTurn = false;
        newSystemTime->minute = (newSystemTime->minute == 0) ? 59 : (newSystemTime->minute - 1);
      }
      break;
    case FIELD_SECONDS:
      if (clockwiseTurn) {
        clockwiseTurn = false;
        newSystemTime->second = (newSystemTime->second + 1) % 60;
      } else {
        counterClockwiseTurn = false;
        newSystemTime->second = (newSystemTime->second == 0) ? 59 : (newSystemTime->second - 1);
      }
      break;
    default:
      break;
  }      

}


void printState(){
  Serial.print(F("Current state: "));
  Serial.println(stateMachine.currentState);
  Serial.print(F("Config substate: "));
  Serial.println(stateMachine.currentConfigSubstate);
  Serial.print(F("System Time Field: "));
  Serial.println(stateMachine.currentSystemTimeField);
  Serial.print(F("Irrigation Time field: "));
  Serial.println(stateMachine.currentIrrigationTimeField);
  Serial.print(F("Config done: "));
  Serial.println(stateMachine.CONFIG_DONE);
  Serial.print(F("Current Notification: "));
  Serial.println(stateMachine.currentNotification);
  Serial.print(F("User confirms: "));
  Serial.println(stateMachine.USER_CONFIRMS);
  Serial.print(F("Active menu index: "));
  Serial.println(stateMachine.activeMenuIndex);
  Serial.print(F("Force Stop: "));
  Serial.println(stateMachine.force_stop);
  Serial.print(F("Valve on: "));
  Serial.println(stateMachine.valve_on);
  Serial.println();
}
