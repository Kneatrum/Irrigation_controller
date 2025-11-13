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

uint8_t activeMenuIndex = 0;

StateMachine stateMachine = {
  .currentState = STATE_IDLE,
  .currentConfigSubstate = IRRIGATION_TIME_NAVIGATION,
  .currentSystemTimeField = FIELD_HOURS,
  .currentIrrigationTimeField = IRRIGATION_START_HOUR
};


static void resetSubstates(){
  stateMachine.currentConfigSubstate = IRRIGATION_TIME_NAVIGATION;
  stateMachine.currentSystemTimeField = FIELD_HOURS;
  stateMachine.currentIrrigationTimeField = IRRIGATION_START_HOUR;
}

void setIdleState(){
  stateMachine.currentState = STATE_IDLE;
  resetSubstates();
}

void setShowingDetailsState(){
  stateMachine.currentState = STATE_SHOWING_DETAILS;
  resetSubstates();
}

void setMenuNavigationState(){
  stateMachine.currentState = STATE_MENU_NAVIGATION;
  resetSubstates();
}

void setConfiguringState(){
  stateMachine.currentState = STATE_CONFIGURING;
  resetSubstates();
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
      // Set notification that configuration is done
      // Write irrigation time to EEPROM
      if (writeIrrigationTime(schedule) == EEPROM_SUCCESS){
        sm->currentState = STATE_SHOWING_DETAILS;
        sm->currentIrrigationTimeField = IRRIGATION_START_HOUR;
        sm->CONFIG_DONE = true;
      }
      break;
    default:
      break;
  }

}


void configureSystemTime(StateMachine* sm, systemTime_t* newTime) {
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
      // Set notification that configuration is done
      // Write system time to RTC
      // writeSystemTime(newTime);
      setSystemTime(newTime);
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

void handleSystemTimeField(SystemTimeEditingField * field, systemTime_t * newSystemTime) {
  switch(*field) {
    case FIELD_YEAR:
      if (clockwiseTurn) {
        clockwiseTurn = false;
        newSystemTime->year++;
      } else {
        counterClockwiseTurn = false;
        newSystemTime->year--;
      }
      break;
    case FIELD_MONTH:
      if (clockwiseTurn) {
        clockwiseTurn = false;
        newSystemTime->month = (newSystemTime->month % 12) + 1;
      } else {
        counterClockwiseTurn = false;
        newSystemTime->month = (newSystemTime->month == 1) ? 12 : (newSystemTime->month - 1);
      }
      break;
    case FIELD_DAY:
      if (clockwiseTurn) {
        clockwiseTurn = false;
        newSystemTime->day = (newSystemTime->day % 31) + 1;
      } else {
        counterClockwiseTurn = false;
        newSystemTime->day = (newSystemTime->day == 1) ? 31 : (newSystemTime->day - 1);
      }
      break;
    case FIELD_HOURS:
      if (clockwiseTurn) {
        clockwiseTurn = false;
        newSystemTime->hours = (newSystemTime->hours + 1) % 24;
      } else {
        counterClockwiseTurn = false;
        newSystemTime->hours = (newSystemTime->hours == 0) ? 23 : (newSystemTime->hours - 1);
      }
      break;
    case FIELD_MINUTES:
      if (clockwiseTurn) {
        clockwiseTurn = false;
        newSystemTime->minutes = (newSystemTime->minutes + 1) % 60;
      } else {
        counterClockwiseTurn = false;
        newSystemTime->minutes = (newSystemTime->minutes == 0) ? 59 : (newSystemTime->minutes - 1);
      }
      break;
    case FIELD_SECONDS:
      if (clockwiseTurn) {
        clockwiseTurn = false;
        newSystemTime->seconds = (newSystemTime->seconds + 1) % 60;
      } else {
        counterClockwiseTurn = false;
        newSystemTime->seconds = (newSystemTime->seconds == 0) ? 59 : (newSystemTime->seconds - 1);
      }
      break;
    default:
      break;
  }      

}
