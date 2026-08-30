#include <Arduino.h>
#include "eeprom_utils.h"
#include "rtc.h"
#include "glcd.h"
#include "cr0x_valves.h"
#include "states.h"
#include "pinout.h"
#include "rotary_enc.h"
#include "voltage_ctrl.h"

const uint8_t ONE_SECOND_INTERVAL = 1;
const uint8_t TWO_SECONDS_INTERVAL = 2;
const uint8_t FIVE_SECONDS_INTERVAL = 5;

RTCTime_t currentTime;
systemTime_t newSystemTime;
bool one_second_flag = false;
bool five_seconds_flag = false;
uint8_t seconds_counter = 0;

ValveType TEMPORARY_VALVE_TYPE = VALVE_NOT_SELECTED;
ValveVoltage TEMPORARY_VALVE_VOLTAGE = VOLTAGE_NOT_SELECTED;

void populateMenu(StateMachine sm){
  if(!sm.CONFIG_DONE){

    if(!sm.valve_is_selected){
      if(!MENU_ITEMS) MENU_ITEMS = createMenuItem(MENU_SELECT_VALVE_TYPE);
      else insertAtEnd(&MENU_ITEMS, MENU_SELECT_VALVE_TYPE);
    }

    if(!sm.voltage_is_selected){
      if(!MENU_ITEMS) MENU_ITEMS = createMenuItem(MENU_SELECT_VOLTAGE);
      else insertAtEnd(&MENU_ITEMS, MENU_SELECT_VOLTAGE);
    }

  } 
   
  if(!MENU_ITEMS) MENU_ITEMS = createMenuItem(MENU_IRRIGATION_SCHEDULES);
  else insertAtEnd(&MENU_ITEMS, MENU_IRRIGATION_SCHEDULES);
  
  if(!MENU_ITEMS) MENU_ITEMS = createMenuItem(MENU_SET_SYSTEM_TIME);
  else insertAtEnd(&MENU_ITEMS, MENU_SET_SYSTEM_TIME);

  // if(sm.valve_on) insertAtEnd(&MENU_ITEMS, MENU_FORCE_STOP);
  // if(sm.force_stop) insertAtEnd(&MENU_ITEMS, MENU_CONTINUE_IRRIGATING);
  
  if(sm.voltage_is_selected || sm.valve_is_selected){
    insertAtEnd(&MENU_ITEMS, MENU_MORE_ITEMS);
  }

  insertAtEnd(&MENU_ITEMS, MENU_EXIT);
}

void updateState(StateMachine *sm){
  bool is_valve_selected = readValveSelectionState();
  bool is_voltage_selected = readVoltageSelectionState();
  ValveVoltage selectedVoltage =  readSelectedVoltage();
  ValveType selectedValve = readSelectedValve();

  for(int i = 0; i < MAX_IRRIGATION_SCHEDULES; i++){
    if(isIrrigationScheduleSet(i)){
      sm->irrigation_schedules[i].time_is_set = true;
      sm->irrigation_schedules[i].schedule_time = readIrrigationSchedule(i);  
      sm->irrigation_schedules[i].enabled = readEnableStatus(i);
    }
  }
 
  bool is_force_stopped = readForceStopStatus();
  bool valve_state = readValveState();

  sm->valve_is_selected = is_valve_selected;
  sm->voltage_is_selected = is_voltage_selected;
  sm->selected_voltage = selectedVoltage;
  sm->selected_valve = selectedValve;
  sm->force_stop = is_force_stopped;
  sm->valve_on = valve_state;

  if(sm->valve_is_selected && sm->voltage_is_selected && 
    (sm->irrigation_schedules[SCHEDULE_1].time_is_set || 
     sm->irrigation_schedules[SCHEDULE_2].time_is_set ||
     sm->irrigation_schedules[SCHEDULE_3].time_is_set ||
     sm->irrigation_schedules[SCHEDULE_4].time_is_set)){
     sm->CONFIG_DONE = true;
  } 

  if(sm->valve_is_selected){
    switch(selectedValve){
      case CR01:
        sm->setValve = &setCR01;
        break;
      case CR02:
        sm->setValve = &setCR02;
        break;
      case CR03:
        sm->setValve = &setCR03;
        break;
      case CR04:
        sm->setValve = &setCR04;
        break;
      case CR05:
        sm->setValve = &setCR05;
        break;
      case VALVE_NOT_SELECTED:
      default:
        sm->setValve = NULL;
        is_valve_selected = false;
        sm->valve_is_selected = is_valve_selected;
        saveValveSelectionState(false);
    }
  }

}

void setActiveIrrigationSchedule(StateMachine *sm){
  for(int i = 0; i < MAX_IRRIGATION_SCHEDULES; i++){
    if(sm->irrigation_schedules[i].enabled){
      if(isWithinIrrigationTime(sm->irrigation_schedules[i].schedule_time) && !stateMachine.error_setting_valve){
        sm->is_time_to_irrigate = true;
        sm->active_index_candidate = i;
        break; // There should only be one active irrigation schedule
      } else {
        if(sm->is_time_to_irrigate) sm->is_time_to_irrigate = false;
        // sm->active_index_candidate = -1;
      }
    } else {
      // If the schedule is disabled this effectively turns the valve off if it's on.
      if( sm->is_time_to_irrigate ){
        sm->is_time_to_irrigate = false;
      }
      // if( sm->active_index_candidate > -1){
      //   sm->active_index_candidate = -1;
      // }
    }
  }
}

bool showDetailsBeforeValveEvent(StateMachine sm){
  bool show = false;
  // Serial.println(!stateMachine.valve_on? "check start time": "check stop time");
  for(int i = 0; i < MAX_IRRIGATION_SCHEDULES; i++){
    if(sm.irrigation_schedules[i].enabled){
      if(isSecondsBeforeIrrigationEvent(stateMachine.irrigation_schedules[i].schedule_time,10, !stateMachine.valve_on)){
        show = true;
      }
    }
  }
  return show;
}

uint16_t toMinutes(uint8_t hour, uint8_t minute) {
  return (uint16_t)hour * 60 + minute;
}

bool isTimeDifferenceMoreThan5Minutes(ScheduleTime_t t) {
    uint16_t start = toMinutes(t.startHour,  t.startMinute);
    uint16_t stop  = toMinutes(t.stopHour,   t.stopMinute);

    uint16_t diff = (stop >= start)
                  ? (stop - start)           // normal case
                  : (1440 - start + stop);   // midnight crossing

    return diff >= 5;
}

bool isConflictingTimeSchedules(StateMachine sm) {
  ScheduleTime_t newTime = sm.irrigation_schedules[sm.schedule_index].schedule_time;
  uint16_t newStart = toMinutes(newTime.startHour,  newTime.startMinute);
  uint16_t newStop  = toMinutes(newTime.stopHour,   newTime.stopMinute);

  for (int i = 0; i < MAX_IRRIGATION_SCHEDULES; i++) {
    if (i == sm.schedule_index)                    continue; // don't compare against itself
    if (!sm.irrigation_schedules[i].enabled)       continue;
    if (!sm.irrigation_schedules[i].time_is_set)   continue;

    uint16_t existStart = toMinutes(sm.irrigation_schedules[i].schedule_time.startHour,
                                    sm.irrigation_schedules[i].schedule_time.startMinute);
    uint16_t existStop  = toMinutes(sm.irrigation_schedules[i].schedule_time.stopHour,
                                    sm.irrigation_schedules[i].schedule_time.stopMinute);

    // Standard interval overlap: A overlaps B if A starts before B ends AND B starts before A ends
    if (newStart < existStop && existStart < newStop) {
      return true;
    }
  }
  return false;
}


void setup() {
  Serial.setRx(PA10);
  Serial.setTx(PA9);
  Serial.begin(115200);

  tmux1208Init();
  initLCD();
  valveInit();
  initializeRotaryEncoder();
  attachRotaryEncoderInterrupts(updateEncoder, handleButton);
  initializeRTC();
  updateState(&stateMachine);
  populateMenu(stateMachine);

  stateMachine.DISPLAY_DETAILS_TIMEOUT = DEFAULT_TIMEOUT_MS;
  stateMachine.CONFIGURE_DETAILS_TIMEOUT = DEFAULT_TIMEOUT_MS;

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
        stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_BROWSING];
        stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 0;
        stateMachine.irrigation_schedules[stateMachine.schedule_index].enabled = readEnableStatus(stateMachine.schedule_index);
        stateMachine.schedule_index = SCHEDULE_1;
        setShowingDetailsState();
      }
    }

    if(stateMachine.currentState == STATE_MENU_NAVIGATION && stateMachine.CONFIG_DONE){
      stateMachine.MENU_NAV_STATE_COUNTER += 1000;
      if(stateMachine.MENU_NAV_STATE_COUNTER >= stateMachine.MENU_NAV_STATE_TIMEOUT){
        setShowingDetailsState();
      }
    }
    // Serial.print("Current state: ");
    
    switch(stateMachine.currentState) {
      case STATE_INIT:
        // Serial.println("INIT");
        break;
      case STATE_IDLE:
        // Serial.println("IDLE");
        break;
      case STATE_SHOWING_DETAILS:
        // Serial.println("SHOWING_DETAILS");
        // Serial.print("Display timeout ");
        // Serial.println(stateMachine.SHOW_DETAILS_COUNTER);
        break;
      case STATE_MENU_NAVIGATION:
        // Serial.println("MENU_NAVIGATION");
        break;
      case STATE_CONFIGURING:
        // Serial.print("Config Timer ");
        // Serial.println(stateMachine.CONFIG_STATE_COUNTER);
        switch(stateMachine.currentConfigSubstate){
          case VALVE_SELECTION:
            // Serial.println("CONFIGURING");
            switch(stateMachine.currentNotification){
              case SAVE_VALVE_TYPE:
                // Serial.println("\tSaving valve type");
              break;
              default:
                // Serial.println("\tValve selection");
                // printState();
              break;
            }
          break;
          case VOLTAGE_SELECTION:
            Serial.println("CONFIGURING");
            switch(stateMachine.currentNotification){
              case SAVE_VOLTAGE:
                Serial.println("\tSaving voltage");
              break;
              default:
                Serial.println("\tVoltage selection");
                // printState();
              break;
            }
          case SCHEDULE_SELECTION:
          Serial.println("CONFIGURING");
            switch (stateMachine.schedule_state)
            {
              case SCHEDULE_BROWSING:
                Serial.println("\tBrowsing");
                // printState();
              break;
              case SCHEDULE_EN_DEFAULT:
                Serial.println("\tEnabled By Default");
              break;
              case SCHEDULE_DIS_DEFAULT:
                Serial.println("\tDisabled By Default");
              break;
              case SCHEDULE_DISABLED:
                Serial.println("\tDisabled");
              break;
              case SCHEDULE_ENABLED:
                Serial.println("\tEnabled");
              break;
              case SCHEDULE_SET_TIME:
                Serial.println("\tSet time");
              break;
            }
          break;
        }
        break;
      default:
        Serial.println("UNKNOWN");
        break;
    }
    float valveV = readValveVoltage();
    float inVoltage =  readInputVoltage();
  }


  if(five_seconds_flag && stateMachine.CONFIG_DONE && stateMachine.currentState != STATE_CONFIGURING) {
    five_seconds_flag = false;
    setActiveIrrigationSchedule(&stateMachine);

    Serial.print("Is time to irrigate: ");
    Serial.println(stateMachine.is_time_to_irrigate);
    Serial.print("Active status: ");
    Serial.println(stateMachine.irrigation_schedules[stateMachine.schedule_index].active);
    Serial.print("Active candidate: ");
    Serial.println( stateMachine.active_index_candidate);
    Serial.println("***");

    if(stateMachine.is_time_to_irrigate){
      if(!stateMachine.valve_on){
        showMessage("Turning valve on");
        VsetRes_t res = stateMachine.setValve(true);
        if (res == VSET_SUCCESS ){
          showMessage("Success");
          stateMachine.valve_on =  true;
          stateMachine.irrigation_schedules[stateMachine.active_index_candidate].active = true;
          writeValveState(stateMachine.valve_on);
          delay(1000);
        } else if(res == VSET_TIMEOUT){
          showMessage("Timeout in turning valve on");
          stateMachine.error_setting_valve = true;
          if( !findMenuItem(MENU_ITEMS, MENU_CLEAR_ERROR) ){
            insertAtBeginnig(&MENU_ITEMS, MENU_CLEAR_ERROR);
            stateMachine.activeMenuItem = MENU_ITEMS->data;
          }
          delay(3000);
        } else if(res == VSET_ERROR){
          showMessage("Error setting valve");
          stateMachine.error_setting_valve = true;
          if( !findMenuItem(MENU_ITEMS, MENU_CLEAR_ERROR) ){
            insertAtBeginnig(&MENU_ITEMS, MENU_CLEAR_ERROR);
            stateMachine.activeMenuItem = MENU_ITEMS->data;
          }
          delay(3000);
        }
      }
    } else {

      if(stateMachine.valve_on){
        showMessage("Turning valve off");
        VsetRes_t res = stateMachine.setValve(false);
        if(res == VSET_SUCCESS ){
          showMessage("Success");
          stateMachine.valve_on = false;
          writeValveState(stateMachine.valve_on);
          delay(1000);
        } else if (res == VSET_TIMEOUT){
          showMessage("Timeout in setting valve");
          delay(3000);
        } else if(res == VSET_ERROR){
          showMessage("Error in turning valve off");
          delay(3000);
        }
      }

      if(stateMachine.irrigation_schedules[stateMachine.active_index_candidate].active){
        stateMachine.irrigation_schedules[stateMachine.active_index_candidate].active = false;
        stateMachine.active_index_candidate = -1;
      }

    }

    if(showDetailsBeforeValveEvent(stateMachine)){
      setShowingDetailsState();
    }
  }
  
 
  if(encoderMoved) {
    encoderMoved = false;
    switch(stateMachine.currentState)
    {
      case STATE_INIT:
      break;
      case STATE_IDLE:
      break;
      case STATE_SHOWING_DETAILS:
       switch(stateMachine.schedule_index){
        case SCHEDULE_1:
          if(clockwiseTurn) stateMachine.schedule_index = SCHEDULE_2;
          stateMachine.SHOW_DETAILS_COUNTER = 0;
          updateScreenFlag = true;
        break;
        case SCHEDULE_2:
          if(clockwiseTurn) stateMachine.schedule_index = SCHEDULE_3;
          else stateMachine.schedule_index = SCHEDULE_1;
          stateMachine.SHOW_DETAILS_COUNTER = 0;
          updateScreenFlag = true;
        break;
        case SCHEDULE_3:
          if(clockwiseTurn) stateMachine.schedule_index = SCHEDULE_4;
          else stateMachine.schedule_index = SCHEDULE_2;
          stateMachine.SHOW_DETAILS_COUNTER = 0;
          updateScreenFlag = true;
        break;
        case SCHEDULE_4:
          if(counterClockwiseTurn) stateMachine.schedule_index = SCHEDULE_3;
          stateMachine.SHOW_DETAILS_COUNTER = 0;
          updateScreenFlag = true;
        break;
      }
      break;
      case STATE_MENU_NAVIGATION: {
        menuItem_t *currentMenuItem = findMenuItem(MENU_ITEMS, stateMachine.activeMenuItem);

        if (!currentMenuItem) {
          currentMenuItem = MENU_ITEMS;
        }

        if (clockwiseTurn) {
          // Only move forward if a next item exists
          if (currentMenuItem->next != nullptr) {
            currentMenuItem = currentMenuItem->next;
            stateMachine.activeMenuItem = currentMenuItem->data;
          }
        } else {
          // Only move backward if a prev item exists
          if (currentMenuItem->prev != nullptr) {
            currentMenuItem = currentMenuItem->prev;
            stateMachine.activeMenuItem = currentMenuItem->data;
          }
        }
        updateScreenFlag = true;
        break;
      }
      case STATE_CONFIGURING:
        stateMachine.CONFIG_STATE_COUNTER = 0;
        if(stateMachine.currentConfigSubstate == VALVE_SELECTION){
          if(stateMachine.currentNotification == SAVE_VALVE_TYPE){
            if(clockwiseTurn) stateMachine.USER_CONFIRMS = true;
            else stateMachine.USER_CONFIRMS = false;
          } else {
            handleValveSelection(&TEMPORARY_VALVE_TYPE);
          }
          updateScreenFlag = true;
        } else if(stateMachine.currentConfigSubstate == VOLTAGE_SELECTION){
          if(stateMachine.currentNotification == SAVE_VOLTAGE){
            if(clockwiseTurn) stateMachine.USER_CONFIRMS = true;
            else stateMachine.USER_CONFIRMS = false;
          } else {
            handleVoltageSelection(&TEMPORARY_VALVE_VOLTAGE);
          }
          updateScreenFlag = true;
        } else if (stateMachine.currentConfigSubstate == SCHEDULE_SELECTION) {
         
          switch(stateMachine.schedule_state){
            case SCHEDULE_BROWSING:
              switch(stateMachine.schedule_index){
                case SCHEDULE_1:
                  if(clockwiseTurn) stateMachine.schedule_index = SCHEDULE_2;
                  updateScreenFlag = true;
                break;
                case SCHEDULE_2:
                  if(clockwiseTurn) stateMachine.schedule_index = SCHEDULE_3;
                  else stateMachine.schedule_index = SCHEDULE_1;
                  updateScreenFlag = true;
                break;
                case SCHEDULE_3:
                  if(clockwiseTurn) stateMachine.schedule_index = SCHEDULE_4;
                  else stateMachine.schedule_index = SCHEDULE_2;
                  updateScreenFlag = true;
                break;
                case SCHEDULE_4:
                  if(clockwiseTurn) stateMachine.schedule_index = SCHEDULE_EXIT;
                  else stateMachine.schedule_index = SCHEDULE_3;
                  updateScreenFlag = true;
                break;
                case SCHEDULE_EXIT:
                  if(counterClockwiseTurn) stateMachine.schedule_index = SCHEDULE_4;
                  updateScreenFlag = true;
                break;
              }
            break;
            case SCHEDULE_EN_DEFAULT:
              if(clockwiseTurn){
                if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option < 2)
                stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option++;
                updateScreenFlag = true;
              } else {
                if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option > 0)
                stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option--;
                updateScreenFlag = true;
              }
            break;
            case SCHEDULE_DIS_DEFAULT:
              if(clockwiseTurn){
                if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option < 1)
                stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option++;
                updateScreenFlag = true;
              } else {
                if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option > 0)
                stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option--;
                updateScreenFlag = true;
              }
            break;
            case SCHEDULE_DISABLED:
              if(clockwiseTurn){
                if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option < 1)
                stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option++;
              } else {
                if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option > 0)
                stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option--;
              }
              updateScreenFlag = true;
            break;
            case SCHEDULE_ENABLED:
              if(clockwiseTurn){
                if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option < 2){
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option++;
                }
              } else {
                if( stateMachine.irrigation_schedules[stateMachine.schedule_index].enabled && 
                    (!isTimeDifferenceMoreThan5Minutes(stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_time)) ||
                    isConflictingTimeSchedules(stateMachine))
                    {
                       // This is like not allowing the user to click on option 0 (save) if there is a time conflict. We want to force the user to adjust the time first before saving the schedule.
                      if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option > 1){
                        stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option--;
                      }
                } else {
                  if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option > 0)
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option--;
                }
              }
              updateScreenFlag = true;
            break;
            case SCHEDULE_SET_TIME:
              stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 0;
              updateScreenFlag = true;
               if(stateMachine.currentNotification == SAVE_IRRIGATION_SCHEDULE){
                if(clockwiseTurn) stateMachine.USER_CONFIRMS = true;
                else stateMachine.USER_CONFIRMS = false;
              } else {
                handleIrrigationTimeField(&stateMachine.currentIrrigationTimeField, &stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_time);
              }
              updateScreenFlag = true;
            break;
          }
        } else if (stateMachine.currentConfigSubstate == SYSTEM_TIME_NAVIGATION) {
          if(stateMachine.currentNotification == SAVE_SYSTEM_TIME){
            if(clockwiseTurn) stateMachine.USER_CONFIRMS = true;
            else stateMachine.USER_CONFIRMS = false;
          } else {
            handleSystemTimeField(&stateMachine.currentSystemTimeField, &currentTime);
          }
          updateScreenFlag = true;
        }
      break;
      default:
        break;
    }
  }


  if(updateScreenFlag){
    updateScreenFlag = false;
    switch(stateMachine.currentState) {
      case STATE_INIT:
        // Serial.println(stateMachine.valve_on? "INIT Valve is on": "INIT Valve is off");
        showInitialisationMessage(stateMachine);
        stateMachine.activeMenuItem = MENU_ITEMS->data;
        // Serial.println(stateMachine.activeMenuItem);
        // traverseForward(MENU_ITEMS);

        if(stateMachine.valve_is_selected) {
          // Serial.println("Valve is selected");
          TEMPORARY_VALVE_TYPE = stateMachine.selected_valve;
        } else {
          // Serial.println("Valve is not selected");
          TEMPORARY_VALVE_TYPE = CR01;
        }

        if(stateMachine.voltage_is_selected){
          // Serial.println("Voltage is selected");
          TEMPORARY_VALVE_VOLTAGE = stateMachine.selected_voltage;
          setMuxEnable(true); // Turn on voltage
        } else {
          // Serial.println("Voltage is not selected");
          TEMPORARY_VALVE_VOLTAGE = FIVE_VOLTS;
        }

        if(stateMachine.voltage_is_selected && stateMachine.valve_is_selected){
          if(stateMachine.valve_on == true){
            showMessage("Turning valve on");
            // stateMachine.setValve(true);
            VsetRes_t res = stateMachine.setValve(true);
            if(res == VSET_SUCCESS ){
              showMessage("Success setting voltage");
              delay(3000);
            } else if (res == VSET_TIMEOUT){
              showMessage("Timeout in setting voltage");
              delay(3000);
            } else if(res == VSET_ERROR){
              showMessage("Error setting voltage");
              delay(3000);
            }
          } else {
            showMessage("Initialising");
            // stateMachine.setValve(false);
            VsetRes_t res = stateMachine.setValve(false);
            if(res == VSET_SUCCESS ){
              // showMessage("Success setting voltage");
              // delay(3000);
            } else if (res == VSET_TIMEOUT){
              showMessage("Timeout in setting voltage");
              delay(3000);
            } else if(res == VSET_ERROR){
              showMessage("Error setting voltage");
              delay(3000);
            }
          }
        }

        if(stateMachine.CONFIG_DONE){
          setShowingDetailsState();
        } else {
          setMenuNavigationState();
        }
        break;
      case STATE_IDLE:
        break;
      case STATE_SHOWING_DETAILS:
        showDetailsScreen(currentTime, stateMachine);
        break;
      case STATE_MENU_NAVIGATION:
        drawMenu(stateMachine, MENU_ITEMS);
        break;
      case STATE_CONFIGURING:
          if(stateMachine.currentConfigSubstate == VALVE_SELECTION){
            if(stateMachine.currentNotification == SAVE_VALVE_TYPE){
              promptUserSaveNewValveType(stateMachine, &TEMPORARY_VALVE_TYPE);
            } else {
              showValveType(VALVE_TYPE_NAMES, &TEMPORARY_VALVE_TYPE);
            }
          } else if(stateMachine.currentConfigSubstate == VOLTAGE_SELECTION){
            if(stateMachine.currentNotification == SAVE_VOLTAGE){
              promptUserSaveNewVoltage(stateMachine, &TEMPORARY_VALVE_VOLTAGE);
            } else {
              showVoltage(VALVE_VOLTAGE_NAMES, &TEMPORARY_VALVE_VOLTAGE);
            }
          } else if(stateMachine.currentConfigSubstate == SCHEDULE_SELECTION) {
            if(stateMachine.currentNotification == SCHEDULE_CONFLICT){
              showMessage("Schedule conflict! Please adjust the time.");
              delay(3000); // Show message for 3 seconds
              stateMachine.schedule_state = SCHEDULE_ENABLED;
              stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_ENABLED];
              stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 2; // Set the current option to Set Time to force user to adjust time before saving schedule again
              stateMachine.currentNotification = NONE; // Clear notification after showing message
            } else if(stateMachine.currentNotification == TIME_DIFFERENCE_TOO_SHORT){
              showMessage("Invalid schedule! Stop time must be at least 5 minutes after start time.");
              delay(3000); // Show message for 3 seconds
              stateMachine.schedule_state = SCHEDULE_ENABLED;
              stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_ENABLED];
              stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 2; // Set the current option to Set Time to force user to adjust time before saving schedule again
              stateMachine.currentNotification = NONE; // Clear notification after showing message
            } else if(stateMachine.currentNotification == SAVE_IRRIGATION_SCHEDULE){
              promptUserSaveNewSchedule(stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_time, stateMachine);
            } else {
              // showIrrigationTimeSetting(&stateMachine.irrigation_schedule, stateMachine.currentIrrigationTimeField);
              showSchedules(stateMachine);
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
      case STATE_INIT:
        break;
      case STATE_IDLE:
        stateMachine.currentState = STATE_SHOWING_DETAILS;
        updateScreenFlag = true;
        break;
      case STATE_SHOWING_DETAILS:
        stateMachine.currentState = STATE_MENU_NAVIGATION;
        updateScreenFlag = true;
        break;
      case STATE_MENU_NAVIGATION:{
        // Handle menu navigation button press
        const char *menuIndex = stateMachine.activeMenuItem;

        if(strcmp(menuIndex, MENU_CLEAR_ERROR) == 0){
          if(stateMachine.error_setting_valve) stateMachine.error_setting_valve = false;
          if(findMenuItem(MENU_ITEMS, MENU_CLEAR_ERROR)){
            deleteMenuItem(&MENU_ITEMS, MENU_CLEAR_ERROR);
          }
          stateMachine.activeMenuItem = MENU_ITEMS->data;
          updateScreenFlag = true;
          setShowingDetailsState();
        } else if(strcmp(menuIndex, MENU_SELECT_VALVE_TYPE) == 0){
          stateMachine.currentState = STATE_CONFIGURING;
          stateMachine.currentConfigSubstate = VALVE_SELECTION;
          updateScreenFlag = true;
        } else if(strcmp(menuIndex, MENU_SELECT_VOLTAGE) == 0) {
          stateMachine.currentState = STATE_CONFIGURING;
          stateMachine.currentConfigSubstate = VOLTAGE_SELECTION;
          updateScreenFlag = true;
        } else if(strcmp(menuIndex, MENU_IRRIGATION_SCHEDULES) == 0) {
          stateMachine.currentState = STATE_CONFIGURING;
          stateMachine.currentConfigSubstate = SCHEDULE_SELECTION;
          updateScreenFlag = true;
        } else if(strcmp(menuIndex, MENU_SET_SYSTEM_TIME) == 0) {
          stateMachine.currentState = STATE_CONFIGURING;
          stateMachine.currentConfigSubstate = SYSTEM_TIME_NAVIGATION;
          stateMachine.currentSystemTimeField = FIELD_YEAR;
          updateScreenFlag = true;
        } else if(strcmp(menuIndex, MENU_LESS_ITEMS) == 0) {  // User clicks on Less
          int index_of_less = getMenuIndex(MENU_ITEMS, MENU_LESS_ITEMS);
          if(index_of_less != -1){
            updateMenuItemByIndex(MENU_ITEMS, index_of_less, MENU_MORE_ITEMS);
          }

          int index_of_select_valve = getMenuIndex(MENU_ITEMS, MENU_SELECT_VALVE_TYPE);
          if(index_of_select_valve != -1){
            deleteMenuItemByIndex(&MENU_ITEMS, index_of_select_valve);
          }
          
          int index_of_select_voltage = getMenuIndex(MENU_ITEMS, MENU_SELECT_VOLTAGE);
          if(index_of_select_voltage != -1){
            MENU_ITEMS = deleteMenuItemByIndex(&MENU_ITEMS, index_of_select_voltage);
          }

          stateMachine.activeMenuItem = MENU_ITEMS->data;
          updateScreenFlag = true;

        } else if(strcmp(menuIndex, MENU_MORE_ITEMS) == 0) {  // User clicks on MORE
          int  index_of_more = getMenuIndex(MENU_ITEMS, MENU_MORE_ITEMS);
          int  index_of_select_voltage = getMenuIndex(MENU_ITEMS, MENU_SELECT_VOLTAGE);
          int  index_of_select_valve = getMenuIndex(MENU_ITEMS, MENU_SELECT_VALVE_TYPE);
          MENU_ITEMS = updateMenuItemByIndex(MENU_ITEMS, index_of_more, MENU_LESS_ITEMS);
          
          if(index_of_select_voltage == -1) {
            MENU_ITEMS = insertAtBeginnig(&MENU_ITEMS, MENU_SELECT_VOLTAGE);
          }

          if(index_of_select_valve == -1) {
            MENU_ITEMS = insertAtBeginnig(&MENU_ITEMS, MENU_SELECT_VALVE_TYPE);
          }
          
          stateMachine.activeMenuItem = MENU_ITEMS->data;
          updateScreenFlag = true;
        } else if(strcmp(menuIndex, MENU_EXIT) == 0) { // User clicks on Exit
          stateMachine.activeMenuItem = MENU_ITEMS->data;
          setShowingDetailsState();
          updateScreenFlag = true;
        }
      }
      break;
      case STATE_CONFIGURING:
        if(stateMachine.currentConfigSubstate == VALVE_SELECTION){
          if(stateMachine.currentNotification == SAVE_VALVE_TYPE){
            if(stateMachine.USER_CONFIRMS){
              if(saveSelectedValve(TEMPORARY_VALVE_TYPE)){
                saveValveSelectionState(true);
                switch(TEMPORARY_VALVE_TYPE){
                  case 0:
                    stateMachine.selected_valve = CR01;
                    stateMachine.setValve = &setCR01;
                  break;
                  case 1:
                    stateMachine.selected_valve = CR02;
                    stateMachine.setValve = &setCR02;
                  break;
                  case 2:
                    stateMachine.selected_valve = CR03;
                    stateMachine.setValve = &setCR03;
                  break;
                  case 3:
                    stateMachine.selected_valve = CR04;
                    stateMachine.setValve = &setCR04;
                  break;
                  case 4:
                    stateMachine.selected_valve = CR05;
                    stateMachine.setValve = &setCR05;
                  break;
                }
                
                stateMachine.selected_valve = TEMPORARY_VALVE_TYPE;
                stateMachine.valve_is_selected = true;

                deleteMenuItem(&MENU_ITEMS, MENU_SELECT_VALVE_TYPE);

                if(!findMenuItem(MENU_ITEMS, MENU_MORE_ITEMS)){  // If the "More" option is not present, add it.
                  int exitIndex = getMenuIndex(MENU_ITEMS, MENU_EXIT);
                  insertAtIndex(&MENU_ITEMS, exitIndex, MENU_MORE_ITEMS);
                }

                // If any of MENU_SELECT_VALVE_TYPE or MENU_SELECT_VOLTAGE is not in the linked list
                // Remove the LESS option from the linked list
                if( !findMenuItem(MENU_ITEMS, MENU_SELECT_VALVE_TYPE) || !findMenuItem(MENU_ITEMS, MENU_SELECT_VOLTAGE)){
                  if(findMenuItem(MENU_ITEMS, MENU_LESS_ITEMS)){
                    int less_items_index = getMenuIndex(MENU_ITEMS,MENU_LESS_ITEMS);
                    deleteMenuItem(&MENU_ITEMS, MENU_LESS_ITEMS);
                  }
                }

                stateMachine.activeMenuItem = MENU_ITEMS->data;

                if(stateMachine.voltage_is_selected && stateMachine.irrigation_schedules[stateMachine.schedule_index].time_is_set){
                  if(!stateMachine.CONFIG_DONE){
                    stateMachine.CONFIG_DONE = true;
                    // showMessage("Turning valve off");
                    // VsetRes_t res = stateMachine.setValve(false); 
                    // if(res == VSET_SUCCESS ){
                    //   showMessage("Success setting valve");
                    //   delay(3000);
                    //   setShowingDetailsState();
                    // } else if (res == VSET_TIMEOUT){
                    //   showMessage("Timeout in setting valve");
                    //   delay(3000);
                    // } else if(res == VSET_ERROR){
                    //   showMessage("Error setting valve");
                    //   delay(3000);
                    // }
                  }
                } else {
                  setMenuNavigationState();
                }
                updateScreenFlag = true;
              }
            } else {
              // Serial.println("User has not confirmed"); 
              // stateMachine.activeMenuItem = MENU_ITEMS->data;
              setMenuNavigationState();
              updateScreenFlag = true;
            } 
          } else {
            if(TEMPORARY_VALVE_TYPE == VALVE_NOT_SELECTED /*Exit*/){  
              // Serial.println("This is the Exit option on the menu");
              stateMachine.activeMenuItem = MENU_ITEMS->data;
              setMenuNavigationState();
            }
            
            stateMachine.currentNotification = SAVE_VALVE_TYPE;
            updateScreenFlag = true;

          }
        } else if(stateMachine.currentConfigSubstate == VOLTAGE_SELECTION){
          if(stateMachine.currentNotification == SAVE_VOLTAGE){
            if(stateMachine.USER_CONFIRMS){
              VsetRes_t status;
              setVoltage(TEMPORARY_VALVE_VOLTAGE, &status);
              if(status == VSET_SUCCESS){
                saveSelectedVoltage(TEMPORARY_VALVE_VOLTAGE);
                saveVoltageSelectionState(true);

                stateMachine.selected_voltage = TEMPORARY_VALVE_VOLTAGE;
                stateMachine.voltage_is_selected = true;
                MENU_ITEMS = deleteMenuItem(&MENU_ITEMS, MENU_SELECT_VOLTAGE);

                if(!findMenuItem(MENU_ITEMS, MENU_MORE_ITEMS)){ // If the "More" option does not exist
                  int exitIndex = getMenuIndex(MENU_ITEMS, MENU_EXIT);
                  insertAtIndex(&MENU_ITEMS, exitIndex, MENU_MORE_ITEMS); // Add "More" option.
                }

                // If any of MENU_SELECT_VALVE_TYPE or MENU_SELECT_VOLTAGE is not in the linked list
                // Remove the LESS option from the linked list
                if( !findMenuItem(MENU_ITEMS, MENU_SELECT_VALVE_TYPE) || !findMenuItem(MENU_ITEMS, MENU_SELECT_VOLTAGE)){
                  if(findMenuItem(MENU_ITEMS, MENU_LESS_ITEMS)){
                    int less_items_index = getMenuIndex(MENU_ITEMS,MENU_LESS_ITEMS);
                    deleteMenuItem(&MENU_ITEMS, MENU_LESS_ITEMS);
                  }
                }

                stateMachine.activeMenuItem = MENU_ITEMS->data;

                if(stateMachine.valve_is_selected && stateMachine.irrigation_schedules[stateMachine.schedule_index].time_is_set){
                  setShowingDetailsState();
                  if(!stateMachine.CONFIG_DONE){
                    stateMachine.CONFIG_DONE = true;
                    showMessage("Turning valve off");
                    delay(3000);
                    VsetRes_t res = stateMachine.setValve(false);
                    if(res == VSET_SUCCESS ){
                      showMessage("Success setting valve");
                      delay(3000);
                      updateScreenFlag = true;
                    } else if (res == VSET_TIMEOUT){
                      showMessage("Timeout in setting valve");
                      delay(3000);
                      updateScreenFlag = true;
                    } else if(res == VSET_ERROR){
                      showMessage("Error setting valve");
                      delay(3000);
                      updateScreenFlag = true;
                    }
                  }
                } else {
                  setMenuNavigationState();
                  updateScreenFlag = true;
                }
              } else if(status == VSET_TIMEOUT){
                showMessage("Timeout in setting voltage");
                delay(3000);
                setMenuNavigationState();
                updateScreenFlag = true;
              } else if(status == VSET_ERROR){
                showMessage("Error setting voltage");
                delay(3000);
                setMenuNavigationState();
                updateScreenFlag = true;
              } else if(status == VSET_SUPPLY_VOLTAGE_TOO_LOW){
                showMessage("Supply voltage too low");
                delay(3000);
                setMenuNavigationState();
                updateScreenFlag = true;
              }
              // Check make sure that CONFIG_DONE is set to true before allowing a transition to another state
              // setShowingDetailsState();
            } else {
             // User selects DISCARD
              setMenuNavigationState();
              updateScreenFlag = true;
            }
          } else {
            if(TEMPORARY_VALVE_VOLTAGE == VOLTAGE_NOT_SELECTED){
              // Serial.println("This is the Exit option on the menu");
              stateMachine.activeMenuItem = MENU_ITEMS->data;
              setMenuNavigationState();
            }
            // Serial.println("User has selected voltage. Set the notification to SAVE_VOLTAGE to prompt user to confirm their selection on the next short press");
            stateMachine.currentNotification = SAVE_VOLTAGE;
            updateScreenFlag = true;
          }
        } else if(stateMachine.currentConfigSubstate == SCHEDULE_SELECTION){
            // Serial.println("In schedule selection substate");
            switch(stateMachine.schedule_state){
              // bool schedule_enabled = stateMachine.irrigation_schedules[stateMachine.schedule_index].enabled;
              // uint8_t selected_option =  stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option;
              case SCHEDULE_BROWSING:
                // Serial.println("Browsing");
                if(stateMachine.schedule_index == SCHEDULE_EXIT){
                  // Serial.println("Browsing 1");
                  stateMachine.currentState = STATE_MENU_NAVIGATION;
                  // stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 0;  // This caused a bug let it remain there for me to study it later
                  setMenuNavigationState();
                  // Prepare for next time we get into schedule navigation
                  stateMachine.schedule_index = SCHEDULE_1;
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_BROWSING];
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 0;

                  stateMachine.activeMenuItem = MENU_ITEMS->data;
                  stateMachine.currentConfigSubstate = SCHEDULE_SELECTION;
                  updateScreenFlag = true;
                } else {
                  if(stateMachine.irrigation_schedules[stateMachine.schedule_index].enabled){
                    stateMachine.schedule_state = SCHEDULE_EN_DEFAULT;
                    stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_EN_DEFAULT];
                    stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 0;
                  } else {
                    stateMachine.schedule_state = SCHEDULE_DIS_DEFAULT;
                    stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_DIS_DEFAULT];
                    stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 0;
                  }
                  updateScreenFlag = true;
                }
              break;
              case SCHEDULE_EN_DEFAULT:
                if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option == 0){
                  stateMachine.schedule_state = SCHEDULE_BROWSING;
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_BROWSING];
                  updateScreenFlag = true;
                } else if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option == 1){
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].enabled = false;
                  stateMachine.schedule_state = SCHEDULE_DISABLED;
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_DISABLED];
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 0;
                  updateScreenFlag = true;
                } else if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option == 2){
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_SET_TIME];
                  stateMachine.schedule_state = SCHEDULE_SET_TIME;
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 0;
                  updateScreenFlag = true;
                }
              break;
              case SCHEDULE_DIS_DEFAULT:
                if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option == 0){ // EXIT
                  stateMachine.schedule_state = SCHEDULE_BROWSING;
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_BROWSING];
                  updateScreenFlag = true;
                } else if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option == 1){  // ENABLE
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].enabled = true;
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_ENABLED];
                  stateMachine.schedule_state = SCHEDULE_ENABLED;
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 2; // Highlight set time option to prompt user to set time after enabling the schedule. This is because a schedule with no time set is not useful at all.
                  updateScreenFlag = true;
                }
              break;
              case SCHEDULE_DISABLED:
                if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option == 0){
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].enabled = true;
                  stateMachine.schedule_state = SCHEDULE_EN_DEFAULT;
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_EN_DEFAULT];
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 0;
                  updateScreenFlag = true;
                } else if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option == 1){
                  stateMachine.schedule_state = SCHEDULE_BROWSING;
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_BROWSING];
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 0;
                  // saveIrrigationScheduleToEEPROM
                  updateScreenFlag = true;
                }
              break;
              case SCHEDULE_ENABLED:
                if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option == 0){  // Save
                  stateMachine.schedule_state = SCHEDULE_BROWSING;
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_BROWSING];
                  uint8_t schedule_id = (uint8_t)(stateMachine.schedule_index);
                  writeEnableStatus(schedule_id, true);
                  if(!stateMachine.irrigation_schedules[stateMachine.schedule_index].time_is_set){
                    // isConflictingTimeSchedules(StateMachine sm)
                    // isTimeDifferenceMoreThan5Minutes(ScheduleTime_t t)
                    writeIrrigationTime(stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_time, schedule_id);
                    stateMachine.irrigation_schedules[stateMachine.schedule_index].time_is_set = true;
                  }
                  updateScreenFlag = true;
                } else if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option == 1){ // Disable
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].enabled = false;
                  stateMachine.schedule_state = SCHEDULE_DIS_DEFAULT;
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_DIS_DEFAULT];
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 0;
                  updateScreenFlag = true;
                } else if(stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option == 2){ // Set time
                  stateMachine.schedule_state = SCHEDULE_SET_TIME;
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_SET_TIME];
                  stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 0;
                  updateScreenFlag = true;
                }
              break;
              case SCHEDULE_SET_TIME: 
                if(stateMachine.currentNotification == SAVE_IRRIGATION_SCHEDULE){
                  if(stateMachine.USER_CONFIRMS){  // CONFIRMS
                    uint8_t schedule_id =  (uint8_t)(stateMachine.schedule_index);
                    writeIrrigationTime(stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_time, schedule_id);
                    bool enable_status_eeprom = readEnableStatus(schedule_id);
                    bool enable_ram = stateMachine.irrigation_schedules[stateMachine.schedule_index].enabled;

                    // If the enable status in EEPROM is not enabled while that in RAM is enabled, update EEPROM with what is in RAM
                    if( enable_status_eeprom != enable_ram ){
                      writeEnableStatus(schedule_id, enable_ram);
                    }

                    // Turn off notification
                    stateMachine.currentNotification =  NONE;
                    // Serial.println("Irrigation time saved successfully");
                    stateMachine.currentIrrigationTimeField = IRRIGATION_START_HOUR;  // Next time we start setting the IRRIGATION_START_HOUR
                    stateMachine.irrigation_schedules[stateMachine.schedule_index].time_is_set = true;
                    // Exiting. Prepare for the next time we come back to this state
                    stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_BROWSING];
                    stateMachine.schedule_state = SCHEDULE_BROWSING;
                    stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 0;
                    stateMachine.schedule_index = SCHEDULE_1;
                    // Repeat for the new schedule index
                    stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 0;
                    // Make sure we are pointing to the first element in the linked list menu
                    stateMachine.activeMenuItem = MENU_ITEMS->data;

                    if(stateMachine.valve_is_selected && stateMachine.voltage_is_selected){
                      // Serial.println("Configuration is done");
                      if(!stateMachine.CONFIG_DONE){
                        stateMachine.CONFIG_DONE = true;
                        setShowingDetailsState();
                        showMessage("Turning valve off");
                        VsetRes_t res = stateMachine.setValve(false);
                        if(res == VSET_SUCCESS ){
                          showMessage("Success setting valve");
                          delay(3000);
                        } else if (res == VSET_TIMEOUT){
                          showMessage("Timeout in setting valve");
                          delay(3000);
                        } else if(res == VSET_ERROR){
                          showMessage("Error setting valve");
                          delay(3000);
                        }
                      }
                    } else{
                      // Serial.println("Return to Menu navigation");
                      setMenuNavigationState();
                    } 
                    updateScreenFlag = true;
                  } else {  // DISCARD
                    // stateMachine.currentIrrigationTimeField = IRRIGATION_START_HOUR;  
                    if(stateMachine.irrigation_schedules[stateMachine.schedule_index].time_is_set){
                      // Retrieve the old settings
                      stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_time = readIrrigationSchedule(stateMachine.schedule_index);
                    } else {
                      stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_time.startHour = 0;
                      stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_time.startMinute = 0;
                      stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_time.stopHour = 0;
                      stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_time.stopMinute = 0;
                    }
                    stateMachine.schedule_state = SCHEDULE_BROWSING;
                    stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option = 0;
                    stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options = schedule_options_array[SCHEDULE_BROWSING];
                    stateMachine.schedule_index = SCHEDULE_1;
                    stateMachine.activeMenuItem = MENU_ITEMS->data;
                    setMenuNavigationState();
                    updateScreenFlag = true;
                  }
                  // Go back to STATE_SHOWING_DETAILS regardless of the outcome.
                } else {
                  // stateMachine.currentScheduleSubstate = 
                  // configureIrrigationTime(&stateMachine, &stateMachine.irrigation_schedule);
                  navigateIrrigationTime(&stateMachine);
                  updateScreenFlag = true;
                }  
              break;
            }       
        } else if(stateMachine.currentConfigSubstate == SYSTEM_TIME_NAVIGATION){
          if(stateMachine.currentNotification == SAVE_SYSTEM_TIME){
            if(stateMachine.USER_CONFIRMS){
              setSystemTime(currentTime);
              stateMachine.activeMenuItem = MENU_ITEMS->data;
              // stateMachine.currentState = 
              // stateMachine.currentConfigSubstate = 
              bool at_least_one_time_schedule_is_set =  false;

              for(int i = 0; i < MAX_IRRIGATION_SCHEDULES; i++ ){
                if((isIrrigationScheduleSet(i))){
                  at_least_one_time_schedule_is_set = true;
                }
              }

              if(at_least_one_time_schedule_is_set && stateMachine.valve_is_selected && stateMachine.voltage_is_selected){
                if(!stateMachine.CONFIG_DONE){
                  stateMachine.CONFIG_DONE = true;
                  setShowingDetailsState();
                  Serial.println("Turn off valve first");
                  showMessage("Turning valve off");
                  VsetRes_t res = stateMachine.setValve(false);
                  if(res == VSET_SUCCESS ){
                    showMessage("Success setting valve");
                    delay(3000);
                  } else if (res == VSET_TIMEOUT){
                    showMessage("Timeout in setting valve");
                    delay(3000);
                  } else if(res == VSET_ERROR){
                    showMessage("Error setting valve");
                    delay(3000);
                  }
                }
              } else {
                setMenuNavigationState();
              }
              updateScreenFlag = true;
            } else { // CANCEL
              setMenuNavigationState();
              updateScreenFlag = true;
            }
          } else {
            configureSystemTime(&stateMachine, &currentTime);
            updateScreenFlag = true;
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
        if(stateMachine.currentConfigSubstate == VALVE_SELECTION){
          // stateMachine.currentNotification = SAVE_VALVE_TYPE;
        } else if(stateMachine.currentConfigSubstate == VOLTAGE_SELECTION){
          // stateMachine.currentNotification = SAVE_VOLTAGE;
        } else if(stateMachine.currentConfigSubstate == SCHEDULE_SELECTION){
          if(stateMachine.schedule_state == SCHEDULE_SET_TIME){
            ScheduleTime_t t = stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_time;
            if (!isTimeDifferenceMoreThan5Minutes(t)) {
              stateMachine.currentNotification = TIME_DIFFERENCE_TOO_SHORT; // Clear notification after showing message
            } else if (isConflictingTimeSchedules(stateMachine)) {
              stateMachine.currentNotification = SCHEDULE_CONFLICT; // Clear notification after showing message
            } else {
              stateMachine.currentNotification = SAVE_IRRIGATION_SCHEDULE;
            }
          }
        } else if(stateMachine.currentConfigSubstate == SYSTEM_TIME_NAVIGATION){
          stateMachine.currentNotification = SAVE_SYSTEM_TIME;
        }
      break;
    }
  }

}