#ifndef STATES_H
#define STATES_H

#include <stdint.h>
#include "rtc.h"
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_TIMEOUT_MS 90000 // 90 seconds
#define MAX_IRRIGATION_SCHEDULES 4

typedef struct menuItem_t {
    const char *data;
    struct menuItem_t *prev;
    struct menuItem_t *next;
} menuItem_t;

typedef enum {
    VSET_SUCCESS,
    VSET_SUPPLY_VOLTAGE_TOO_LOW,
    VSET_TIMEOUT,
    VSET_ERROR
} VsetRes_t;

typedef enum {
    STATE_INIT,
    STATE_IDLE,
    STATE_SHOWING_DETAILS,
    STATE_MENU_NAVIGATION,
    STATE_CONFIGURING
} SystemState;

typedef enum {
    VALVE_SELECTION,
    VOLTAGE_SELECTION,
    SCHEDULE_SELECTION,
    SYSTEM_TIME_NAVIGATION
} ConfigSubstate;

typedef enum {
    SCHEDULE_BROWSING,
    SCHEDULE_EN_DEFAULT,
    SCHEDULE_DIS_DEFAULT,
    SCHEDULE_DISABLED,
    SCHEDULE_ENABLED,
    SCHEDULE_SET_TIME
} ScheduleState_t;

typedef enum {
    VALVE_TYPE_OPTIONS,
    VOLTAGE_OPTIONS,
    SCHEDULE_OPTIONS,
    SYSTEM_TIME_OPTIONS
} SubstateOptions;

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
    SELECT_VALVE,
    SELECT_VOLTAGE,
    SET_IRRIGATION_TIME,
    SET_SYSTEM_TIME,
    FORCE_STOP_IRRIGATION,
    EXIT_CONFIGURATION
} MenuItems;

typedef enum {
    NONE,
    SAVE_IRRIGATION_SCHEDULE,
    SCHEDULE_CONFLICT,
    TIME_DIFFERENCE_TOO_SHORT,
    SAVE_SYSTEM_TIME,
    DISPLAY_RELAY_OPERATION,
    SAVE_VALVE_TYPE,
    SAVE_VOLTAGE
} Notification;

typedef enum {
    CR01,
    CR02,
    CR03,
    CR04,
    CR05,
    VALVE_NOT_SELECTED
} ValveType;

typedef enum {
    SCHEDULE_1,
    SCHEDULE_2,
    SCHEDULE_3,
    SCHEDULE_4,
    SCHEDULE_EXIT
} ScheduleIndex_t;

// typedef enum {
//     EXIT_OPTION,
//     DISABLE_OPTION,
//     SET_TIME_OPTION,
//     ENABLE_OPTION,
//     DISABLE_OPTION,
//     SAVE_AND_EXIT_OPTION,
// } ScheduleOption_t;

extern const char * const VALVE_TYPE_NAMES[];

typedef enum {
    FIVE_VOLTS,
    SIX_VOLTS,
    NINE_VOLTS,
    TWELVE_VOLTS,
    TWENTY_FOUR_VOLTS,
    VOLTAGE_NOT_SELECTED
} ValveVoltage;

extern const char * const VALVE_VOLTAGE_NAMES[];

extern const char *IRRIGATION_SCHEDULES[];

typedef enum {
    SCH_EN_DEF_EXIT,
    SCH_EN_DEF_DISABLE,
    SCH_EN_DEF_SET_TIME
} SchEnDef_t;  // {"Exit", "Disable", "Set Time"}

typedef enum {
    SCH_DIS_DEF_EXIT,
    SCH_DIS_DEF_ENABLE
} SchDisDef_t; //  {"Exit", "Enable"}

typedef enum {
    SCH_DIS_ENABLE,
    SCH_DIS_SAVE_N_EXIT,
} SchDis_t;   // {"Enable", "Save & Exit"}

typedef enum {
    SCH_EN_SAVE_N_EXIT,
    SCH_EN_DISABLE,
    SCH_EN_SET_TIME
} SchEn_t;   // {"Save & Exit", "Disable", "Set Time"}

// typedef struct {
//     SchEnDef_t sch_enabled_default;
//     SchDisDef_t sch_disabled_default;
//     SchDis_t sch_disabled;
//     SchEn_t sch_enabled;
// } ScheduleOptionsddd_t;

typedef struct {
    SystemState currentState;
    ConfigSubstate currentConfigSubstate;
    ScheduleState_t schedule_state;
    SubstateOptions currentSubtateOption;
    SystemTimeEditingField currentSystemTimeField;
    IrrigationTimeEditingField currentIrrigationTimeField;
    bool CONFIG_DONE;
    Notification currentNotification;
    bool USER_CONFIRMS;
    const char *activeMenuItem;
    uint32_t CONFIGURE_DETAILS_TIMEOUT;
    uint32_t DISPLAY_DETAILS_TIMEOUT;
    uint32_t MENU_NAV_STATE_TIMEOUT;
    unsigned long CONFIG_STATE_COUNTER;
    unsigned long SHOW_DETAILS_COUNTER;
    unsigned long MENU_NAV_STATE_COUNTER;
    bool force_stop;
    bool encoder_moved;
    bool valve_is_selected;
    bool voltage_is_selected;
    ValveType selected_valve;
    VsetRes_t (*setValve)(bool new_state);
    ValveVoltage selected_voltage;
    IrrigationSchedule_t irrigation_schedules[MAX_IRRIGATION_SCHEDULES];
    ScheduleIndex_t schedule_index;
    bool is_time_to_irrigate;
    bool valve_on;
    bool error_setting_valve;
    int active_index_candidate;
} StateMachine;


extern StateMachine stateMachine;
extern MenuItems menuItems;


/**
 * @brief Menu items displayed in configuration mode.
 * 
 * Defined in states.c
 */
extern menuItem_t *MENU_ITEMS;
extern const char *MENU_CLEAR_ERROR;
extern const char *MENU_SELECT_VALVE_TYPE;
extern const char *MENU_SELECT_VOLTAGE;
extern const char *MENU_IRRIGATION_SCHEDULES;
extern const char *MENU_SET_SYSTEM_TIME;
// extern const char *MENU_FORCE_STOP;
// extern const char *MENU_CONTINUE_IRRIGATING;
extern const char *MENU_MORE_ITEMS;
extern const char *MENU_LESS_ITEMS;
extern const char *MENU_EXIT;
extern const unsigned int MENU_ITEMS_COUNT;

extern const char *SCHEDULE_EN_DEFAULT_OPTIONS[];
extern const char *SCHEDULE_DIS_DEFAULT_OPTIONS[];
extern const char *SCHEDULE_DIS_OPTIONS[];
extern const char *SCHEDULE_EN_OPTIONS[];
extern const char *SCHEDULE_SET_TIME_OPTION[];

extern const char **schedule_options_array[];

void setIdleState();
void setShowingDetailsState();
void setMenuNavigationState();
void setConfiguringState();

void navigateIrrigationTime(StateMachine* sm);
void configureSystemTime(StateMachine* sm, RTCTime_t* newTime) ;
void handleIrrigationTimeField(IrrigationTimeEditingField *field, ScheduleTime_t *irrigationSchedule);
void handleSystemTimeField(SystemTimeEditingField * field, RTCTime_t * newSystemTime);
void handleValveSelection(ValveType *temporary_valve_type);
void handleVoltageSelection(ValveVoltage *temporary_voltage);
void printState();
menuItem_t *createMenuItem(const char * item);
menuItem_t *insertAtBeginnig(menuItem_t **head, const char *item);
menuItem_t *insertAtEnd(menuItem_t **head, const char *data);
void traverseForward(menuItem_t *head);
void traverseBackward(menuItem_t *tail);
menuItem_t *findMenuItem(menuItem_t *head, const char *item);
menuItem_t *deleteMenuItem(menuItem_t **head, const char *item);
menuItem_t *deleteMenuItemByIndex(menuItem_t **head, int index);
menuItem_t *updateMenuItem(menuItem_t *head, const char *oldData, const char *newData);
menuItem_t *updateMenuItemByIndex(menuItem_t *head, int index, const char *newData);
int getMenuLength(menuItem_t *head);
int getMenuIndex(menuItem_t *head, const char *item);
menuItem_t *insertAtIndex(menuItem_t **head, int index, const char *data);


#endif
