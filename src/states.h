#ifndef STATES_H
#define STATES_H

#include <stdint.h>
#include "rtc.h"

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

typedef struct {
    SystemState currentState;
    ConfigSubstate currentConfigSubstate;
    SystemTimeEditingField currentSystemTimeField;
    IrrigationTimeEditingField currentIrrigationTimeField;
    bool CONFIG_DONE;
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
extern uint8_t activeMenuIndex;

void setIdleState();
void setShowingDetailsState();
void setMenuNavigationState();
void setConfiguringState();

void configureIrrigationTime(StateMachine* sm, irrigationTime_t* schedule);
void configureSystemTime(StateMachine* sm, systemTime_t* newTime);
void handleIrrigationTimeField(IrrigationTimeEditingField *field, irrigationTime_t *irrigationSchedule);
void handleSystemTimeField(SystemTimeEditingField *field, systemTime_t *newSystemTime);


#endif
