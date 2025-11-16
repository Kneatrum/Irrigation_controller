#ifndef STATES_H
#define STATES_H

#include <stdint.h>
#include "rtc.h"

#define DEFAULT_TIMEOUT_MS 90000 // 90 seconds

typedef enum {
    STATE_IDLE,
    STATE_SHOWING_DETAILS,
    STATE_MENU_NAVIGATION,
    STATE_CONFIGURING
} SystemState;

typedef enum {
    IRRIGATION_TIME_NAVIGATION,
    SYSTEM_TIME_NAVIGATION
} ConfigSubstate;

typedef enum {
    IRRIGATION_START_HOUR,
    IRRIGATION_START_MINUTE,
    IRRIGATION_STOP_HOUR,
    IRRIGATION_STOP_MINUTE
} IrrigationTimeEditingField;

typedef enum {
    FIELD_SECONDS,
    FIELD_MINUTES,
    FIELD_HOURS,
    FIELD_DAY,
    FIELD_MONTH,
    FIELD_YEAR
} SystemTimeEditingField;

typedef enum {
    SET_IRRIGATION_TIME,
    SET_SYSTEM_TIME,
    FORCE_STOP_IRRIGATION,
    EXIT_CONFIGURATION
} MenuItems;

typedef enum {
    NONE,
    SAVE_IRRIGATION_SCHEDULE,
    SAVE_SYSTEM_TIME,
    DISPLAY_RELAY_OPERATION,
} Notification;

typedef struct {
    SystemState currentState;
    ConfigSubstate currentConfigSubstate;
    SystemTimeEditingField currentSystemTimeField;
    IrrigationTimeEditingField currentIrrigationTimeField;
    bool CONFIG_DONE;
    Notification currentNotification;
    bool USER_CONFIRMS;
    uint8_t activeMenuIndex;
    uint32_t CONFIGURE_DETAILS_TIMEOUT;
    uint32_t DISPLAY_DETAILS_TIMEOUT;
    uint32_t MENU_NAV_STATE_TIMEOUT;
    unsigned long CONFIG_STATE_COUNTER;
    unsigned long SHOW_DETAILS_COUNTER;
    unsigned long MENU_NAV_STATE_COUNTER;
    bool force_stop;
    bool valve_on;
} StateMachine;


extern StateMachine stateMachine;
extern MenuItems menuItems;


/**
 * @brief Menu items displayed in configuration mode.
 * 
 * Defined in states.c
 */
extern const char * const MENU_ITEMS[];
extern const unsigned int MENU_ITEMS_COUNT;

void setIdleState();
void setShowingDetailsState();
void setMenuNavigationState();
void setConfiguringState();

void configureIrrigationTime(StateMachine* sm, irrigationTime_t* schedule);
void configureSystemTime(StateMachine* sm, RTCTime_t* newTime) ;
void handleIrrigationTimeField(IrrigationTimeEditingField *field, irrigationTime_t *irrigationSchedule);
void handleSystemTimeField(SystemTimeEditingField * field, RTCTime_t * newSystemTime);
void printState();


#endif
