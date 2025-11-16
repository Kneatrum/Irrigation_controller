#include "glcd.h"
#include "pinout.h"
#include "states.h"

U8G2_ST7920_128X64_F_HW_SPI u8g2(U8G2_R0, PIN_CS, PIN_RST);


bool updateScreenFlag = false;

bool displayOn = false;


void initLCD() {
  pinMode(BACKLIGHT_PIN, OUTPUT);
  u8g2.begin(); 
  turnOnBacklight();
  displayOn = true;
}

void turnOnBacklight() {
  digitalWrite(BACKLIGHT_PIN, HIGH);
}

void turnOffBacklight() {
  digitalWrite(BACKLIGHT_PIN, LOW);
}

void turnOffDisplay() {
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  u8g2.setPowerSave(1); // Turn off display
}

void turnOnDisplay() {
  u8g2.setPowerSave(0); // Turn on display
}


void drawMenu(StateMachine sm) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tr);
  
  uint8_t displayIndex = 0;  // Track actual display position
  
  for (uint8_t i = 0; i < MENU_ITEMS_COUNT; i++) {
    // Skip "Force stop" if irrigation is not active
    if (i == 2 && !sm.valve_on && !sm.force_stop) {
      continue;
    }
    
    uint8_t y = TOP_MARGIN + displayIndex * ITEM_HEIGHT;
    
    if (displayIndex == sm.activeMenuIndex) {
      // Draw inverted rectangle
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - ITEM_HEIGHT + 2, 128, ITEM_HEIGHT);
      u8g2.setDrawColor(0);  // text inverted
    } else {
      u8g2.setDrawColor(1);
    }

    if(sm.force_stop && i == 2){
      u8g2.drawStr(5, y, "Continue Irrigation");
    } else {
      u8g2.drawStr(5, y, MENU_ITEMS[i]);
    }
    displayIndex++;
  }
  
  u8g2.setDrawColor(1); // restore normal mode
  u8g2.sendBuffer();
}



void showDetailsScreen(RTCTime_t* currentTime, irrigationTime_t* schedule, StateMachine sm) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tr);

  char buffer[20];
  uint8_t y = TOP_MARGIN; // start from same margin as menu
  const uint8_t x = 5;

  // Current Time
  sprintf(buffer, "Time: %02d:%02d:%02d", currentTime->hour, currentTime->minute, currentTime->second);
  u8g2.drawStr(x, y, buffer);
  y += ITEM_HEIGHT;

  // Irrigation Status
  if(sm.force_stop){
    sprintf(buffer, "Irrigation: %s", "Stopped");
  } else {
    sprintf(buffer, "Irrigation: %s", (sm.valve_on) ? "ON" : "OFF");
  }
  u8g2.drawStr(x, y, buffer);
  y += ITEM_HEIGHT;

  // Start Time
  sprintf(buffer, "Start: %02d:%02d", schedule->startHour, schedule->startMinute);
  u8g2.drawStr(x, y, buffer);
  y += ITEM_HEIGHT;

  // Stop Time
  sprintf(buffer, "Stop : %02d:%02d", schedule->stopHour, schedule->stopMinute);
  u8g2.drawStr(x, y, buffer);

  u8g2.sendBuffer();
}


void showIrrigationTimeSetting(irrigationTime_t* schedule, IrrigationTimeEditingField editingField) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tr);

  uint8_t startHour = schedule->startHour;
  uint8_t startMinute = schedule->startMinute;
  uint8_t stopHour = schedule->stopHour;
  uint8_t stopMinute = schedule->stopMinute;

  if( startHour == 255 ) startHour = 0;
  if( startMinute == 255 ) startMinute = 0;
  if( stopHour == 255 ) stopHour = 0;
  if( stopMinute == 255 ) stopMinute = 0;

  char buffer[25];
  uint8_t y = TOP_MARGIN;
  const uint8_t x = 5;

  // === Internal Blink State Management ===
  static unsigned long lastBlinkTime = 0;
  static bool blinkState = true;
  unsigned long now = millis();
  if (now - lastBlinkTime >= 500) {  // toggle every 500 ms
    blinkState = !blinkState;
    lastBlinkTime = now;
  }

  // === Highlighted Title ===
  uint8_t titleHeight = ITEM_HEIGHT + 2;
  u8g2.setDrawColor(1);
  u8g2.drawBox(0, y - (ITEM_HEIGHT - 2), 128, titleHeight); // full bar
  u8g2.setDrawColor(0);
  u8g2.drawStr(x, y, "Set Irrigation Time");
  u8g2.setDrawColor(1);
  y += ITEM_HEIGHT + 4;

  // === Labels ===
  u8g2.drawStr(x, y, "Start       Stop");
  y += ITEM_HEIGHT;

  // === Conditional Blink Display ===
  char startHourStr[3], startMinStr[3], stopHourStr[3], stopMinStr[3];

  if (editingField == IRRIGATION_START_HOUR && !blinkState)
    sprintf(startHourStr, "  ");
  else
    sprintf(startHourStr, "%02d", startHour);

  if (editingField == IRRIGATION_START_MINUTE && !blinkState)
    sprintf(startMinStr, "  ");
  else
    sprintf(startMinStr, "%02d", startMinute);

  if (editingField == IRRIGATION_STOP_HOUR && !blinkState)
    sprintf(stopHourStr, "  ");
  else
    sprintf(stopHourStr, "%02d", stopHour);

  if (editingField == IRRIGATION_STOP_MINUTE && !blinkState)
    sprintf(stopMinStr, "  ");
  else
    sprintf(stopMinStr, "%02d", stopMinute);

  // === Final Line ===
  sprintf(buffer, "%s:%s       %s:%s", startHourStr, startMinStr, stopHourStr, stopMinStr);
  u8g2.drawStr(x, y, buffer);

  u8g2.sendBuffer();
}

void showSystemTimeSetting(RTCTime_t* systemTime, const char * const Months[], SystemTimeEditingField editingField) {
  Serial.println(systemTime->month);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tr);

  uint8_t y = TOP_MARGIN;
  const uint8_t x = 5;

  // === Internal Blink ===
  static unsigned long lastBlinkTime = 0;
  static bool blinkState = true;
  unsigned long now = millis();
  if (now - lastBlinkTime >= 500) {
    blinkState = !blinkState;
    lastBlinkTime = now;
  }

  char buffer[40];

  // === Highlighted Title ===
  uint8_t titleHeight = ITEM_HEIGHT + 2;
  u8g2.setDrawColor(1);
  u8g2.drawBox(0, y - (ITEM_HEIGHT - 2), 128, titleHeight); // full bar
  u8g2.setDrawColor(0);
  u8g2.drawStr(x, y, "Set System Time");
  u8g2.setDrawColor(1);
  y += ITEM_HEIGHT + 4;

  switch(editingField){
    case FIELD_YEAR:
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - (ITEM_HEIGHT ), 128, titleHeight); // full bar
      u8g2.setDrawColor(0);
      u8g2.drawStr(x, y, "Year");
      u8g2.setDrawColor(1);
      y += ITEM_HEIGHT + 2;
      if (blinkState)
        sprintf(buffer, "20%02d", systemTime->year);
      else
        sprintf(buffer, "20  ");
      u8g2.drawStr(x, y, buffer);
    break;
    case FIELD_MONTH:
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - (ITEM_HEIGHT ), 128, titleHeight); // full bar
      u8g2.setDrawColor(0);
      u8g2.drawStr(x, y, "Month");
      u8g2.setDrawColor(1);
      y += ITEM_HEIGHT + 2;
      if (blinkState)
        sprintf(buffer, "%s", Months[systemTime->month]);
      else
        sprintf(buffer, " ");
      u8g2.drawStr(x, y, buffer);
    break;
    case FIELD_DAY:
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - (ITEM_HEIGHT ), 128, titleHeight); // full bar
      u8g2.setDrawColor(0);
      u8g2.drawStr(x, y, "Day of month");
      u8g2.setDrawColor(1);
      y += ITEM_HEIGHT + 2;
      if (blinkState)
        sprintf(buffer, "%02d", systemTime->day);
      else
        sprintf(buffer, "   ");
      u8g2.drawStr(x, y, buffer);
    break;
    case FIELD_HOURS:
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - (ITEM_HEIGHT ), 128, titleHeight); // full bar
      u8g2.setDrawColor(0);
      u8g2.drawStr(x, y, "Hours");
      u8g2.setDrawColor(1);
      y += ITEM_HEIGHT + 2;
      if (blinkState)
        sprintf(buffer, "%02d", systemTime->hour);
      else
        sprintf(buffer, "   ");
      u8g2.drawStr(x, y, buffer);
    break;
    case FIELD_MINUTES:
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - (ITEM_HEIGHT ), 128, titleHeight); // full bar
      u8g2.setDrawColor(0);
      u8g2.drawStr(x, y, "Minutes");
      u8g2.setDrawColor(1);
      y += ITEM_HEIGHT + 2;
      if (blinkState)
        sprintf(buffer, "%02d", systemTime->minute);
      else
        sprintf(buffer, "   ");
      u8g2.drawStr(x, y, buffer);
    break;
    case FIELD_SECONDS:
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - (ITEM_HEIGHT ), 128, titleHeight); // full bar
      u8g2.setDrawColor(0);
      u8g2.drawStr(x, y, "Seconds");
      u8g2.setDrawColor(1);
      y += ITEM_HEIGHT + 2;
      if (blinkState)
        sprintf(buffer, "%02d", systemTime->second);
      else
        sprintf(buffer, "   ");
      u8g2.drawStr(x, y, buffer);
    break;
  }

  u8g2.sendBuffer();
}

void promptUserSaveNewSchedule(irrigationTime_t schedule, StateMachine sm) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tr);

  // --- Title ---
  const char* title = "New schedule";
  uint8_t titleWidth = u8g2.getStrWidth(title);
  uint8_t screenWidth = u8g2.getDisplayWidth();
  uint8_t xCenter = (screenWidth - titleWidth) / 2;
  uint8_t y = TOP_MARGIN;

  u8g2.drawStr(xCenter, y, title);
  y += ITEM_HEIGHT + 2;

  // --- Start Time ---
  char buffer[20];
  sprintf(buffer, "Start: %02d:%02d", schedule.startHour, schedule.startMinute);
  u8g2.drawStr(5, y, buffer);
  y += ITEM_HEIGHT;

  // --- Stop Time ---
  sprintf(buffer, "Stop : %02d:%02d", schedule.stopHour, schedule.stopMinute);
  u8g2.drawStr(5, y, buffer);

  // --- Bottom Buttons ---
  const char* discardTxt = "DISCARD";
  const char* saveTxt     = "SAVE";

  uint8_t bottomY = u8g2.getDisplayHeight() - 5;

  uint8_t discardX = 5;
  uint8_t saveX = screenWidth - u8g2.getStrWidth(saveTxt) - 5;

  // Highlight logic
  bool saveActive = sm.USER_CONFIRMS;

  if (!saveActive) {
    // Highlight DISCARD
    uint8_t w = u8g2.getStrWidth(discardTxt) + 4;
    uint8_t h = 10;
    u8g2.drawBox(discardX - 2, bottomY - 10, w, h);
    u8g2.setDrawColor(0);
    u8g2.drawStr(discardX, bottomY - 2, discardTxt);
    u8g2.setDrawColor(1);
  } else {
    // Draw normal DISCARD
    u8g2.drawStr(discardX, bottomY - 2, discardTxt);
  }

  if (saveActive) {
    // Highlight SAVE
    uint8_t w = u8g2.getStrWidth(saveTxt) + 4;
    uint8_t h = 10;
    u8g2.drawBox(saveX - 2, bottomY - 10, w, h);
    u8g2.setDrawColor(0);
    u8g2.drawStr(saveX, bottomY - 2, saveTxt);
    u8g2.setDrawColor(1);
  } else {
    // Normal SAVE
    u8g2.drawStr(saveX, bottomY - 2, saveTxt);
  }

  u8g2.sendBuffer();
}


void promptUserSaveSystemTime(RTCTime_t newSystemTime, StateMachine sm) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tr);

  // --- Title ---
  const char* title = "Save system time";
  uint8_t titleWidth = u8g2.getStrWidth(title);
  uint8_t screenWidth = u8g2.getDisplayWidth();
  uint8_t xCenter = (screenWidth - titleWidth) / 2;
  uint8_t y = TOP_MARGIN;

  u8g2.drawStr(xCenter, y, title);
  y += ITEM_HEIGHT + 2;

  // --- System Time (Format: DD/MM/20YY HH-MM-SS) ---
  char buffer[30];
  sprintf(buffer, "%02d/%02d/20%02d %02d:%02d:%02d", 
          newSystemTime.day,
          newSystemTime.month,
          newSystemTime.year,
          newSystemTime.hour,
          newSystemTime.minute,
          newSystemTime.second);

  u8g2.drawStr(5, y, buffer);

  // --- Bottom Buttons ---
  const char* discardTxt = "DISCARD";
  const char* saveTxt    = "SAVE";

  uint8_t bottomY = u8g2.getDisplayHeight() - 5;

  uint8_t discardX = 5;
  uint8_t saveX = screenWidth - u8g2.getStrWidth(saveTxt) - 5;

  bool saveActive = sm.USER_CONFIRMS;

  // Highlight DISCARD if saveActive == false
  if (!saveActive) {
    uint8_t w = u8g2.getStrWidth(discardTxt) + 4;
    uint8_t h = 10;
    u8g2.drawBox(discardX - 2, bottomY - 10, w, h);
    u8g2.setDrawColor(0);
    u8g2.drawStr(discardX, bottomY - 2, discardTxt);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(discardX, bottomY - 2, discardTxt);
  }

  // Highlight SAVE if saveActive == true
  if (saveActive) {
    uint8_t w = u8g2.getStrWidth(saveTxt) + 4;
    uint8_t h = 10;
    u8g2.drawBox(saveX - 2, bottomY - 10, w, h);
    u8g2.setDrawColor(0);
    u8g2.drawStr(saveX, bottomY - 2, saveTxt);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(saveX, bottomY - 2, saveTxt);
  }

  u8g2.sendBuffer();
}
