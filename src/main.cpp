#include <Arduino.h>
#include "eeprom_utils.h"
#include "rtc.h"
#include "glcd.h"
#include "relays.h"
#include "states.h"
#include "pinout.h"
#include "rotary_enc.h"
#include "valve.h"


bool irrigationScheduleSet = false;

const uint8_t ONE_SECOND_INTERVAL = 1;
const uint8_t TWO_SECONDS_INTERVAL = 2;
const uint8_t FIVE_SECONDS_INTERVAL = 5;

RTCTime_t currentTime;
systemTime_t newSystemTime;
bool one_second_flag = false;
bool five_seconds_flag = false;
uint8_t seconds_counter = 0;


void setup() {

  Serial.begin(115200);
  while(!Serial);

  initializeRelays();
  initializeRotaryEncoder();
  attachRotaryEncoderInterrupts(updateEncoder, handleButton);
  initializeRTC();
  initLCD();

  turnValveOff();

  uint32_t display_timeout =  readDisplayDetailsTimeout();
  uint32_t configure_timeout = readConfigureDetailsTimeout();

  if (display_timeout == 0xFFFFFFFF) {
    stateMachine.DISPLAY_DETAILS_TIMEOUT = DEFAULT_TIMEOUT_MS;
  } else {
    stateMachine.DISPLAY_DETAILS_TIMEOUT = display_timeout;
  } 

  if (configure_timeout == 0xFFFFFFFF) {
    stateMachine.CONFIGURE_DETAILS_TIMEOUT = DEFAULT_TIMEOUT_MS;
    stateMachine.MENU_NAV_STATE_TIMEOUT = DEFAULT_TIMEOUT_MS; // Temporary solution
  } else {
    stateMachine.DISPLAY_DETAILS_TIMEOUT = configure_timeout;
  } 
  
  
  irrigationScheduleSet = isIrrigationScheduleSet();

  if(irrigationScheduleSet) {
    stateMachine.CONFIG_DONE = true;
    irrigationSchedule = readIrrigationTime();
    stateMachine.valve_on = readValveState();
    stateMachine.force_stop = readForceStopStatus();
    setShowingDetailsState();
  } else {
    setMenuNavigationState();
  }

  currentTime = getRTCTime();
  lastTimestamp = getRTCTimestamp();
  
}


void loop() {

  currentTimestamp = getRTCTimestamp();

  if (currentTimestamp - lastTimestamp >= ONE_SECOND_INTERVAL) {
    lastTimestamp = currentTimestamp;
    if(stateMachine.currentState != STATE_CONFIGURING && stateMachine.currentConfigSubstate != SYSTEM_TIME_NAVIGATION){
      currentTime = getRTCTime();
    }
    // one_second_flag = true;
    seconds_counter++;
    updateScreenFlag = true;

    if (seconds_counter >= FIVE_SECONDS_INTERVAL) {
      seconds_counter = 0;
      five_seconds_flag = true;
    }

    // Auto-exit details screen after timeout
    if(stateMachine.currentState == STATE_SHOWING_DETAILS) {
      stateMachine.SHOW_DETAILS_COUNTER += 1000;
      if (stateMachine.SHOW_DETAILS_COUNTER >= stateMachine.DISPLAY_DETAILS_TIMEOUT) {
        setIdleState();
      }
    }

    // Auto-exit configuring state after timeout
    if(stateMachine.currentState == STATE_CONFIGURING && stateMachine.CONFIG_DONE) {
      stateMachine.CONFIG_STATE_COUNTER += 1000;
      if (stateMachine.CONFIG_STATE_COUNTER >= stateMachine.CONFIGURE_DETAILS_TIMEOUT) {       
        setShowingDetailsState();
      }
    }

    if(stateMachine.currentState == STATE_MENU_NAVIGATION && stateMachine.CONFIG_DONE){
      stateMachine.MENU_NAV_STATE_COUNTER += 1000;
      if(stateMachine.currentIrrigationTimeField >= stateMachine.MENU_NAV_STATE_TIMEOUT){
        setShowingDetailsState();
      }
    }
  }


  if(five_seconds_flag && stateMachine.CONFIG_DONE && stateMachine.currentState != STATE_CONFIGURING) {
    five_seconds_flag = false;
    bool withinIrrigationTime = isWithinIrrigationTime(&irrigationSchedule);

    if(stateMachine.force_stop && !withinIrrigationTime) {
      stateMachine.force_stop = false;
      writeForceStopStatus(stateMachine.force_stop);
    }

    if(isSecondsBeforeIrrigationEvent(&irrigationSchedule,10, !stateMachine.valve_on)){
      setShowingDetailsState();
    }

    if(withinIrrigationTime){
      if(stateMachine.force_stop){
        if(stateMachine.valve_on){
          turnValveOff();
          writeValveState(stateMachine.valve_on);
        }
      } else {
        if(!stateMachine.valve_on && !stateMachine.force_stop){
          turnValveOn();
          writeValveState(stateMachine.valve_on);
        }
      } 
      
    } else {
      if(stateMachine.valve_on){
        turnValveOff();
        stateMachine.valve_on = false;
        writeValveState(stateMachine.valve_on);
      }
    }
  }
  
 
  if(encoderMoved) {
    encoderMoved = false;
    switch(stateMachine.currentState)
    {
      case STATE_IDLE:
      break;
      case STATE_SHOWING_DETAILS:
      break;
      case STATE_MENU_NAVIGATION: {
        // Handle menu navigation
        uint8_t visibleMenuCount = stateMachine.valve_on || stateMachine.force_stop ? MENU_ITEMS_COUNT : (MENU_ITEMS_COUNT - 1);
        if (clockwiseTurn) {
          stateMachine.activeMenuIndex = (stateMachine.activeMenuIndex + 1) % visibleMenuCount;
        } else {
          if (stateMachine.activeMenuIndex == 0) {
            stateMachine.activeMenuIndex = visibleMenuCount - 1;
          } else {
            stateMachine.activeMenuIndex--;
          }
        }
        updateScreenFlag = true;
        break;
      }
      case STATE_CONFIGURING:
        stateMachine.CONFIG_STATE_COUNTER = 0;
        if (stateMachine.currentConfigSubstate == IRRIGATION_TIME_NAVIGATION) {
          if(stateMachine.currentNotification == SAVE_IRRIGATION_SCHEDULE){
            stateMachine.USER_CONFIRMS = !stateMachine.USER_CONFIRMS;
            updateScreenFlag = true;
          } else {
            handleIrrigationTimeField(&stateMachine.currentIrrigationTimeField, &irrigationSchedule);
          }
        } else if (stateMachine.currentConfigSubstate == SYSTEM_TIME_NAVIGATION) {
          if(stateMachine.currentNotification == SAVE_SYSTEM_TIME){
            stateMachine.USER_CONFIRMS = !stateMachine.USER_CONFIRMS;
            updateScreenFlag = true;
          } else {
            handleSystemTimeField(&stateMachine.currentSystemTimeField, &currentTime);
          }
        }
      break;
      default:
        break;
    }
  }


  if(updateScreenFlag){
    updateScreenFlag = false;
    switch(stateMachine.currentState) {
      case STATE_IDLE:
        break;
      case STATE_SHOWING_DETAILS:
        showDetailsScreen(&currentTime, &irrigationSchedule, stateMachine);
        break;
      case STATE_MENU_NAVIGATION:
        drawMenu(stateMachine);
        break;
      case STATE_CONFIGURING:
          if(stateMachine.currentConfigSubstate == IRRIGATION_TIME_NAVIGATION) {
            if(stateMachine.currentNotification == SAVE_IRRIGATION_SCHEDULE){
              promptUserSaveNewSchedule(irrigationSchedule, stateMachine);
            } else {
              showIrrigationTimeSetting(&irrigationSchedule, stateMachine.currentIrrigationTimeField);
            }
          } else if (stateMachine.currentConfigSubstate == SYSTEM_TIME_NAVIGATION) {
            if(stateMachine.currentNotification == SAVE_SYSTEM_TIME){
              promptUserSaveSystemTime(currentTime, stateMachine);
            } else {
              showSystemTimeSetting(&currentTime, monthsOfYear, stateMachine.currentSystemTimeField);
            }
          }
        
        break;
      default:
        // Unknown state, reset to IDLE
        stateMachine.currentState = STATE_IDLE;
        break;
    }
  }

  // Turn off display in IDLE state
  if(stateMachine.currentState == STATE_IDLE) {
    if(displayOn) {
      turnOffDisplay();
      turnOffBacklight();
      displayOn = false;
    }
  } else  {
    if(!displayOn) {
      turnOnDisplay();
      turnOnBacklight();
      displayOn = true;
    }
  }

  if (shortPressFlag) {
    shortPressFlag = false;
    stateMachine.CONFIG_STATE_COUNTER = 0;
    stateMachine.SHOW_DETAILS_COUNTER = 0;
    stateMachine.MENU_NAV_STATE_COUNTER = 0;
    switch (stateMachine.currentState)
    {
      case STATE_IDLE:
        stateMachine.currentState = STATE_SHOWING_DETAILS;
        break;
      case STATE_SHOWING_DETAILS:
        stateMachine.currentState = STATE_MENU_NAVIGATION;
        break;
      case STATE_MENU_NAVIGATION:{
        // Handle menu navigation button press
        uint8_t menuIndex = stateMachine.activeMenuIndex;
        if(!stateMachine.force_stop){
          if ((!stateMachine.valve_on ) && menuIndex >= FORCE_STOP_IRRIGATION) {
            menuIndex++;    // jump over FORCE_STOP_IRRIGATION
          }
        }
        switch (menuIndex) {
          case SET_IRRIGATION_TIME:
            stateMachine.currentState = STATE_CONFIGURING;
            stateMachine.currentConfigSubstate = IRRIGATION_TIME_NAVIGATION;
            stateMachine.currentIrrigationTimeField = IRRIGATION_START_HOUR;
            break;
          case SET_SYSTEM_TIME:
            stateMachine.currentState = STATE_CONFIGURING;
            stateMachine.currentConfigSubstate = SYSTEM_TIME_NAVIGATION;
            stateMachine.currentSystemTimeField = FIELD_YEAR;
            break;
          case FORCE_STOP_IRRIGATION:
            if (stateMachine.valve_on) {
              stateMachine.force_stop = true;
              writeForceStopStatus(stateMachine.force_stop);
            } else if(stateMachine.force_stop){
              stateMachine.force_stop = false;
              writeForceStopStatus(stateMachine.force_stop);
            }
            break;
          case EXIT_CONFIGURATION:
            setShowingDetailsState();
            break;
          default:
            break;
        }
        break;
      }
      case STATE_CONFIGURING:
        if(stateMachine.currentConfigSubstate == IRRIGATION_TIME_NAVIGATION){
          if(stateMachine.currentNotification == SAVE_IRRIGATION_SCHEDULE){
            if(stateMachine.USER_CONFIRMS){
              if (writeIrrigationTime(&irrigationSchedule) == EEPROM_SUCCESS){
                // Show success message but i am not doing that.
              }
            }
            // Go back to STATE_SHOWING_DETAILS regardless of the outcome.
            setShowingDetailsState();
          } else {
            configureIrrigationTime(&stateMachine, &irrigationSchedule);
          }
        } else if(stateMachine.currentConfigSubstate == SYSTEM_TIME_NAVIGATION){
          if(stateMachine.currentNotification == SAVE_SYSTEM_TIME){
            if(stateMachine.USER_CONFIRMS){
              setSystemTime(currentTime);
            }
            setShowingDetailsState();
          } else {
            configureSystemTime(&stateMachine, &currentTime);
          }
        }
        break;
    
    default:
      break;
    }
  }

  if( longPressFlag){
    longPressFlag = false;
    switch(stateMachine.currentState){
      case STATE_IDLE:
      break;
      case STATE_SHOWING_DETAILS:
      break;
      case STATE_MENU_NAVIGATION:
      break;
      case STATE_CONFIGURING:
        if(stateMachine.currentConfigSubstate == IRRIGATION_TIME_NAVIGATION){
          stateMachine.currentNotification = SAVE_IRRIGATION_SCHEDULE;
        } else if(stateMachine.currentConfigSubstate == SYSTEM_TIME_NAVIGATION){
          stateMachine.currentNotification = SAVE_SYSTEM_TIME;
        }
      break;
    }
  }

}