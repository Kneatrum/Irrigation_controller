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
  Serial.print("Irrigation schedule set: ");
  Serial.println(irrigationScheduleSet);

  if(irrigationScheduleSet) {
    stateMachine.CONFIG_DONE = true;
    irrigationSchedule = readIrrigationTime();
    VALVE_STATE = readValveState();
    Serial.print("Valve state: ");
    Serial.println(VALVE_STATE);
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
    currentTime = getRTCTime();
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

    if(isSecondsBeforeIrrigationEvent(&irrigationSchedule,10, !VALVE_STATE)){
      setShowingDetailsState();
    }

    if (withinIrrigationTime && !VALVE_STATE) {
      Serial.println("TURNING ON RELAYS");
      turnValveOn();
      VALVE_STATE = true;
      writeValveState(VALVE_STATE);
    } else if (!withinIrrigationTime && VALVE_STATE) {
      Serial.println("TURNING OFF RELAYS");
      turnValveOff();
      VALVE_STATE = false;
      writeValveState(VALVE_STATE);
    }
  }
  
 
  if(encoderMoved) {
    encoderMoved = false;
    Serial.println(stateMachine.currentState);
    switch(stateMachine.currentState)
    {
      case STATE_IDLE:
      break;
      case STATE_SHOWING_DETAILS:
      break;
      case STATE_MENU_NAVIGATION: {
        // Handle menu navigation
        uint8_t visibleMenuCount = VALVE_STATE ? MENU_ITEMS_COUNT : (MENU_ITEMS_COUNT - 1);
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
          handleSystemTimeField(&stateMachine.currentSystemTimeField, &newSystemTime);
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
        showDetailsScreen(&currentTime, &irrigationSchedule, &VALVE_STATE);
        break;
      case STATE_MENU_NAVIGATION:
        drawMenu(stateMachine.activeMenuIndex, VALVE_STATE);
        break;
      case STATE_CONFIGURING:
          if(stateMachine.currentConfigSubstate == IRRIGATION_TIME_NAVIGATION) {
            if(stateMachine.currentNotification == SAVE_IRRIGATION_SCHEDULE){
              promptUserSaveNewSchedule(irrigationSchedule, stateMachine);
            } else {
              showIrrigationTimeSetting(&irrigationSchedule, stateMachine.currentIrrigationTimeField);
            }
          } else if (stateMachine.currentConfigSubstate == SYSTEM_TIME_NAVIGATION) {
            showSystemTimeSetting(&newSystemTime, monthsOfYear, stateMachine.currentSystemTimeField);
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
    CONFIG_STATE_COUNTER = 0;
    SHOW_DETAILS_COUNTER = 0;
    switch (stateMachine.currentState)
    {
      case STATE_IDLE:
        stateMachine.currentState = STATE_SHOWING_DETAILS;
        break;
      case STATE_SHOWING_DETAILS:
        stateMachine.currentState = STATE_MENU_NAVIGATION;
        break;
      case STATE_MENU_NAVIGATION:
        // Handle menu navigation button press
        switch (activeMenuIndex) {
          case SET_IRRIGATION_TIME:
            stateMachine.currentState = STATE_CONFIGURING;
            stateMachine.currentConfigSubstate = IRRIGATION_TIME_NAVIGATION;
            stateMachine.currentIrrigationTimeField = IRRIGATION_START_HOUR;
            Serial.println("SET_IRRIGATION_TIME");
            break;
          case SET_SYSTEM_TIME:
            stateMachine.currentState = STATE_CONFIGURING;
            stateMachine.currentConfigSubstate = SYSTEM_TIME_NAVIGATION;
            stateMachine.currentSystemTimeField = FIELD_YEAR;
            Serial.println("SET_SYSTEM_TIME");
            break;
          case FORCE_STOP_IRRIGATION:
            if (VALVE_STATE) {
              turnValveOff();
              VALVE_STATE = false;
              writeValveState(VALVE_STATE);
            }
            Serial.println("FORCE_STOP_IRRIGATION");
            break;
          case EXIT_CONFIGURATION:
            stateMachine.currentState = STATE_SHOWING_DETAILS;
            SHOW_DETAILS_COUNTER = 0;
            Serial.println("EXIT_CONFIGURATION");
            setShowingDetailsState();
            break;
          default:
            break;
        }
        break;
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
          configureSystemTime(&stateMachine, &newSystemTime);
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
          
        }
      break;
    }
  }

}