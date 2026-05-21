#include "states.h"
#include "rtc.h"
#include "rotary_enc.h"
#include "eeprom_utils.h"


// Define the menu items array here
menuItem_t *MENU_ITEMS = NULL;


// Optionally expose the number of menu items
const unsigned int MENU_ITEMS_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);

const char *MENU_CLEAR_ERROR = "Clear error";
const char *MENU_SELECT_VALVE_TYPE = "Select Valve Type";
const char *MENU_SELECT_VOLTAGE = "Select Voltage";
const char *MENU_IRRIGATION_SCHEDULES = "Irrigation Schedules";
const char *MENU_SET_SYSTEM_TIME = "Set system time";
// const char *MENU_FORCE_STOP = "Force Stop";
// const char *MENU_CONTINUE_IRRIGATING = "Continue Irrigating";
const char *MENU_MORE_ITEMS = "More";
const char *MENU_LESS_ITEMS = "Less";
const char *MENU_EXIT = "Exit";

const char *SCHEDULE_BROWSING_OPTIONS[]    = {                                nullptr}; // 0 options
const char *SCHEDULE_EN_DEFAULT_OPTIONS[]  = {"Exit", "Disable", "Set Time",  nullptr};
const char *SCHEDULE_DIS_DEFAULT_OPTIONS[] = {"Exit", "Enable",               nullptr};
const char *SCHEDULE_DIS_OPTIONS[]         = {"Enable", "Save",               nullptr};
const char *SCHEDULE_EN_OPTIONS[]          = {"Save", "Disable", "Set Time",  nullptr};
const char *SCHEDULE_SET_TIME_OPTION[]     = {"Long-press to save",           nullptr};

const char **schedule_options_array[] = {
  SCHEDULE_BROWSING_OPTIONS,     // 0
  SCHEDULE_EN_DEFAULT_OPTIONS,   // 1
  SCHEDULE_DIS_DEFAULT_OPTIONS,  // 2
  SCHEDULE_DIS_OPTIONS,          // 3
  SCHEDULE_EN_OPTIONS,           // 4
  SCHEDULE_SET_TIME_OPTION       // 5
};

const char * const VALVE_TYPE_NAMES[] = {
  "CR-01",
  "CR-02",
  "CR-03",
  "CR-04",
  "CR-05",
  "Exit"
};

const char * const VALVE_VOLTAGE_NAMES[] = {
  "5V",
  "6V",
  "9V",
  "12V",
  "24V",
  "Exit"
};

const char *IRRIGATION_SCHEDULES[] = {
  "Schedule 1",
  "Schedule 2",
  "Schedule 3",
  "Schedule 4"
  "Exit"
};


StateMachine stateMachine = {
  .currentState = STATE_INIT,
  .currentConfigSubstate = VALVE_SELECTION,
  .schedule_state = SCHEDULE_BROWSING,
  .currentSubtateOption = VALVE_TYPE_OPTIONS,
  .currentSystemTimeField = FIELD_HOURS,
  .currentIrrigationTimeField = IRRIGATION_START_HOUR,
  .CONFIG_DONE =false,
  .currentNotification = NONE,
  .USER_CONFIRMS = false,
  .activeMenuItem = NULL,
  .CONFIGURE_DETAILS_TIMEOUT = DEFAULT_TIMEOUT_MS,
  .DISPLAY_DETAILS_TIMEOUT = DEFAULT_TIMEOUT_MS,
  .MENU_NAV_STATE_TIMEOUT = DEFAULT_TIMEOUT_MS,
  .CONFIG_STATE_COUNTER = 0,
  .SHOW_DETAILS_COUNTER = 0,
  .MENU_NAV_STATE_COUNTER = 0,
  .force_stop = false,
  .encoder_moved = false,
  .valve_is_selected = false,
  .voltage_is_selected =false,
  .selected_valve = VALVE_NOT_SELECTED,
  .setValve = NULL,
  .selected_voltage = VOLTAGE_NOT_SELECTED,
  .irrigation_schedules { { 0,0,0,0, false, false, false, 0, NULL  }, { 0,0,0,0, false, false, false, 0, NULL }, { 0,0,0,0, false, false, false, 0, NULL }, { 0,0,0,0, false, false, false, 0, NULL } },
  .schedule_index = SCHEDULE_1,
  .is_time_to_irrigate = false,
  .valve_on = false,
  .error_setting_valve = false,
  .active_index_candidate = -1
};


static void resetSubstates(){
  stateMachine.currentConfigSubstate = VALVE_SELECTION;
  stateMachine.currentSystemTimeField = FIELD_HOURS;
  stateMachine.currentIrrigationTimeField = IRRIGATION_START_HOUR;
}

static void resetScheduleState(){
  stateMachine.schedule_state = SCHEDULE_BROWSING;
}

void setIdleState(){
  stateMachine.currentState = STATE_IDLE;
  resetSubstates();
  resetScheduleState();
  // stateMachine.CONFIG_DONE = true;
  stateMachine.USER_CONFIRMS = false;
  stateMachine.currentNotification = NONE;
  // stateMachine.activeMenuIndex = 0;
  stateMachine.CONFIG_STATE_COUNTER = 0;
  stateMachine.SHOW_DETAILS_COUNTER = 0;
  stateMachine.MENU_NAV_STATE_COUNTER = 0;
}

void setShowingDetailsState(){
  stateMachine.currentState = STATE_SHOWING_DETAILS;
  resetSubstates();
  resetScheduleState();
  // stateMachine.CONFIG_DONE = true;
  stateMachine.USER_CONFIRMS = false;
  stateMachine.currentNotification = NONE;
  // stateMachine.activeMenuIndex = 0;
  stateMachine.CONFIG_STATE_COUNTER = 0;
  stateMachine.SHOW_DETAILS_COUNTER = 0;
  stateMachine.MENU_NAV_STATE_COUNTER = 0;
}

void setMenuNavigationState(){
  stateMachine.currentState = STATE_MENU_NAVIGATION;
  resetSubstates();
  resetScheduleState();
  // stateMachine.CONFIG_DONE = true;
  stateMachine.USER_CONFIRMS = false;
  stateMachine.currentNotification = NONE;
  // stateMachine.activeMenuIndex = 0;
  stateMachine.CONFIG_STATE_COUNTER = 0;
  stateMachine.SHOW_DETAILS_COUNTER = 0;
  stateMachine.MENU_NAV_STATE_COUNTER = 0;
}

void setConfiguringState(){
  stateMachine.currentState = STATE_CONFIGURING;
  resetSubstates();
  resetScheduleState();
  // stateMachine.CONFIG_DONE = true;
  stateMachine.USER_CONFIRMS = false;
  stateMachine.currentNotification = NONE;
  // stateMachine.activeMenuIndex = 0;
  stateMachine.CONFIG_STATE_COUNTER = 0;
  stateMachine.SHOW_DETAILS_COUNTER = 0;
  stateMachine.MENU_NAV_STATE_COUNTER = 0;
}

void navigateIrrigationTime(StateMachine* sm) {
  // Implementation for configuring irrigation time
  // This function should handle the logic for setting start and stop times
  // based on the current field being edited in the state machine.
  switch(sm->currentIrrigationTimeField) {
    case IRRIGATION_START_HOUR:
      sm->currentIrrigationTimeField = IRRIGATION_START_MINUTE;
      Serial.println("IRRIGATION_START_MINUTE");
      break;
    case IRRIGATION_START_MINUTE:
      sm->currentIrrigationTimeField = IRRIGATION_STOP_HOUR;
      Serial.println("IRRIGATION_STOP_HOUR");
      break;
    case IRRIGATION_STOP_HOUR:
      sm->currentIrrigationTimeField = IRRIGATION_STOP_MINUTE;
      Serial.println("IRRIGATION_STOP_MINUTE");
      break;
    case IRRIGATION_STOP_MINUTE:
      sm->currentIrrigationTimeField = IRRIGATION_START_HOUR;
      Serial.println("IRRIGATION_START_HOUR");
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

void handleIrrigationTimeField(IrrigationTimeEditingField *field, ScheduleTime_t *irrigationSchedule){
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

void handleValveSelection(ValveType *temporary_valve_type){
  if(clockwiseTurn){
    clockwiseTurn = false;
    if(*temporary_valve_type < VALVE_NOT_SELECTED){
      *temporary_valve_type = static_cast<ValveType>((*temporary_valve_type) + 1);
    } 
  } else {
    counterClockwiseTurn = false;
    if(*temporary_valve_type > CR01){
      *temporary_valve_type = static_cast<ValveType>((*temporary_valve_type) - 1);
    } 
  }
}

void handleVoltageSelection(ValveVoltage *temporary_voltage){
  if (clockwiseTurn) {
    clockwiseTurn = false;
    if(*temporary_voltage < VOLTAGE_NOT_SELECTED){
      *temporary_voltage = static_cast<ValveVoltage>((*temporary_voltage) + 1);
    } 
  } else if (counterClockwiseTurn) {
    counterClockwiseTurn = false;
    if(*temporary_voltage > FIVE_VOLTS){
      *temporary_voltage = static_cast<ValveVoltage>((*temporary_voltage) - 1);
    }
  }
}


void printState(){
  Serial.print(F("Current state: "));
  Serial.println(stateMachine.currentState);
  Serial.print(F("Config substate: "));
  Serial.println(stateMachine.currentConfigSubstate);
  Serial.print(F("Schedule State: "));
  Serial.println(stateMachine.schedule_state);
  Serial.print(F("Schedule Options: "));
  Serial.println(stateMachine.irrigation_schedules[stateMachine.schedule_index].schedule_options[stateMachine.irrigation_schedules[stateMachine.schedule_index].selected_option]);
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
  Serial.print(F("Active menu item: "));
  Serial.println(stateMachine.activeMenuItem);
  Serial.print(F("Force Stop: "));
  Serial.println(stateMachine.force_stop);
  Serial.print(F("Valve on: "));
  Serial.println(stateMachine.valve_on);
  Serial.println();
}


menuItem_t *createMenuItem(const char * item){
  menuItem_t *node = (menuItem_t *)malloc(sizeof(menuItem_t));
  node->data = item;
  node->prev = NULL;
  node->next = NULL;
  return node;
} 

menuItem_t *insertAtBeginnig(menuItem_t **head, const char *item){
  menuItem_t *newNode = createMenuItem(item);
  if(*head == NULL){
    *head = newNode;
    return newNode;
  };
  newNode->next = *head;
  (*head)->prev = newNode;
  *head = newNode; 
  return newNode;
}

menuItem_t *insertAtEnd(menuItem_t **head, const char *data){
  menuItem_t *newNode = createMenuItem(data);
  if(*head == NULL){
    *head = newNode;
    return newNode;
  }
  menuItem_t *current = *head;
  while(current->next != NULL){
    current = current->next;
  }
  current->next = newNode;
  newNode->prev = current;
  return newNode;
}


void traverseForward(menuItem_t *head){
  menuItem_t *current = head;
  Serial.println("Printing Menu Items:");
  while(current != NULL){
    Serial.println(current->data);
    current = current->next;
  }
  Serial.println("Done Printing Menu Items:");
}

void traverseBackward(menuItem_t *tail){
  menuItem_t *current = tail;
  while(current != NULL){
    Serial.println(current->data);
    current = current->prev;
  }
}

menuItem_t *findMenuItem(menuItem_t *head, const char *item){
  menuItem_t *current = head;
  while(current != NULL){
    if(strcmp(current->data, item) == 0){
      return current;
    }
    current = current->next;
  }
  return NULL;
}


menuItem_t *deleteMenuItem(menuItem_t **head, const char *item){
  menuItem_t *current = *head;
  while(current != NULL){
    if(strcmp(current->data, item) == 0){
      if(current->prev != NULL){
        current->prev->next = current->next;
      } else {
        *head = current->next; // Deleting the head
      }
      if(current->next != NULL){
        current->next->prev = current->prev;
      }
      free(current);
      return *head;
    }
    current = current->next;
  }
  return *head; // Item not found, return original head
}

menuItem_t *deleteMenuItemByIndex(menuItem_t **head, int index){
  menuItem_t *current = *head;
  int currentIndex = 0;
  while(current != NULL){
    if(currentIndex == index){
      if(current->prev != NULL){
        current->prev->next = current->next;
      } else {
        *head = current->next; // Deleting the head
      }
      if(current->next != NULL){
        current->next->prev = current->prev;
      }
      free(current);
      return *head;
    }
    current = current->next;
    currentIndex++;
  }
  return *head; // Index out of bounds, return original head
}


menuItem_t *updateMenuItem(menuItem_t *head, const char *oldData, const char *newData){
  menuItem_t *current = head;
  while(current != NULL){
    if(strcmp(current->data, oldData) == 0){
      current->data = newData;
      return head;
    }
    current = current->next;
  }
  return head; // Item not found, return original head
}


menuItem_t *updateMenuItemByIndex(menuItem_t *head, int index, const char *newData){
  menuItem_t *current = head;
  int currentIndex = 0;
  while(current != NULL){
    if(currentIndex == index){
      current->data = newData;
      return head;
    }
    current = current->next;
    currentIndex++;
  }
  return head; // Index out of bounds, return original head
}

int getMenuLength(menuItem_t *head){
  int length = 0;
  menuItem_t *current = head;
  while(current != NULL){
    length++;
    current = current->next;
  }
  return length;
}

int getMenuIndex(menuItem_t *head, const char *item){
  int index = 0;
  menuItem_t *current = head;
  while(current != NULL){
    if(strcmp(current->data, item) == 0){
      return index;
    }
    current = current->next;
    index++;
  }
  return -1; // Item not found
}

menuItem_t *insertAtIndex(menuItem_t **head, int index, const char *data){
  if(index < 0) return *head; // Invalid index
  menuItem_t *newNode = createMenuItem(data);
  if(index == 0){
    newNode->next = *head;
    if(*head != NULL) (*head)->prev = newNode;
    *head = newNode;
    return newNode;
  }
  menuItem_t *current = *head;
  int currentIndex = 0;
  while(current != NULL && currentIndex < index - 1){
    current = current->next;
    currentIndex++;
  }
  if(current == NULL) {
    free(newNode); // Index out of bounds
    return *head;
  }
  newNode->next = current->next;
  newNode->prev = current;
  if(current->next != NULL){
    current->next->prev = newNode;
  }
  current->next = newNode;
  return newNode;
}