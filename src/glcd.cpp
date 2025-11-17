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
  if (now - lastBlinkTime >= 500) {
    blinkState = !blinkState;
    lastBlinkTime = now;
  }

  // === Highlighted Title ===
  uint8_t titleHeight = ITEM_HEIGHT + 2;
  u8g2.setDrawColor(1);
  u8g2.drawBox(0, y - (ITEM_HEIGHT - 2), 128, titleHeight);
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

  // === Time line ===
  sprintf(buffer, "%s:%s       %s:%s", startHourStr, startMinStr, stopHourStr, stopMinStr);
  u8g2.drawStr(x, y, buffer);

  // === New line: "Long-press to save" ===
  y += ITEM_HEIGHT + 3;                 // move down a little
  u8g2.setFont(u8g2_font_6x10_tr);       // optional smaller font
  u8g2.drawStr(x, y, "Long-press to save");

  u8g2.sendBuffer();
}

void showSystemTimeSetting(
    RTCTime_t* systemTime,
    const char * const Months[],
    SystemTimeEditingField editingField
) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tr);

  const uint8_t screenW = u8g2.getDisplayWidth();
  const uint8_t screenH = u8g2.getDisplayHeight();

  // === Blink timing ===
  static unsigned long lastBlinkTime = 0;
  static bool blinkState = true;
  unsigned long now = millis();
  if (now - lastBlinkTime >= 500) {
    blinkState = !blinkState;
    lastBlinkTime = now;
  }

  // ============================
  // Title: centered at top
  // ============================
  const char* title = "Set system time";
  uint8_t titleW = u8g2.getStrWidth(title);
  u8g2.drawStr((screenW - titleW) / 2, 12, title);

  // ============================
  // Date + Time in one line
  // ============================
  // Format: DD/MM/YYYY HH:MM:SS
  char dateTime[40];

  // Build each field with blinking support
  char dd[4], mm[4], yyyy[8], HH[4], MM[4], SS[4];

  // DAY
  if (editingField == FIELD_DAY && !blinkState)
    sprintf(dd, "  ");
  else
    sprintf(dd, "%02d", systemTime->day);

  // MONTH
  if (editingField == FIELD_MONTH && !blinkState)
    sprintf(mm, "  ");
  else
    sprintf(mm, "%02d", systemTime->month);

  // YEAR
  if (editingField == FIELD_YEAR && !blinkState)
    sprintf(yyyy, "    ");
  else
    sprintf(yyyy, "20%02d", systemTime->year);

  // HOURS
  if (editingField == FIELD_HOURS && !blinkState)
    sprintf(HH, "  ");
  else
    sprintf(HH, "%02d", systemTime->hour);

  // MINUTES
  if (editingField == FIELD_MINUTES && !blinkState)
    sprintf(MM, "  ");
  else
    sprintf(MM, "%02d", systemTime->minute);

  // SECONDS
  if (editingField == FIELD_SECONDS && !blinkState)
    sprintf(SS, "  ");
  else
    sprintf(SS, "%02d", systemTime->second);

  // Combine into final formatted line
  sprintf(dateTime, "%s/%s/%s  %s:%s:%s", dd, mm, yyyy, HH, MM, SS);

  // Center the date/time horizontally
  uint8_t dtW = u8g2.getStrWidth(dateTime);
  u8g2.drawStr((screenW - dtW) / 2, 32, dateTime);

  // ============================
  // Bottom text
  // ============================
  const char* footer = "Long-press to save";
  uint8_t fw = u8g2.getStrWidth(footer);
  u8g2.drawStr((screenW - fw) / 2, screenH - 4, footer);

  u8g2.sendBuffer();
}

void promptUserSaveNewSchedule(irrigationTime_t schedule, StateMachine sm) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tr);

  const uint8_t screenW = u8g2.getDisplayWidth();
  const uint8_t screenH = u8g2.getDisplayHeight();

  // ===== Title =====
  const char* title = "New schedule";
  uint8_t titleWidth = u8g2.getStrWidth(title);
  uint8_t xCenter = (screenW - titleWidth) / 2;

  uint8_t y = 12;  
  u8g2.drawStr(xCenter, y, title);

  // ===== Start / Stop =====
  y += 16;  // exact spacing under title

  char buffer[20];
  sprintf(buffer, "Start: %02d:%02d", schedule.startHour, schedule.startMinute);
  u8g2.drawStr(5, y, buffer);

  y += 15;
  sprintf(buffer, "Stop : %02d:%02d", schedule.stopHour, schedule.stopMinute);
  u8g2.drawStr(5, y, buffer);

  // ===== Buttons =====
  const char* discardTxt = "DISCARD";
  const char* saveTxt    = "SAVE";

  uint8_t buttonTextY = screenH - 4;     // baseline
  uint8_t buttonBoxTop = screenH - 14;   // top of highlight
  uint8_t buttonBoxH = 14;               // full highlight height

  uint8_t discardX = 5;
  uint8_t saveX = screenW - u8g2.getStrWidth(saveTxt) - 5;

  bool saveActive = sm.USER_CONFIRMS;

  // --- DISCARD ---
  if (!saveActive) {
    uint8_t w = u8g2.getStrWidth(discardTxt) + 6;
    u8g2.drawBox(discardX - 3, buttonBoxTop, w, buttonBoxH);
    u8g2.setDrawColor(0);
    u8g2.drawStr(discardX, buttonTextY, discardTxt);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(discardX, buttonTextY, discardTxt);
  }

  // --- SAVE ---
  if (saveActive) {
    uint8_t w = u8g2.getStrWidth(saveTxt) + 6;
    u8g2.drawBox(saveX - 3, buttonBoxTop, w, buttonBoxH);
    u8g2.setDrawColor(0);
    u8g2.drawStr(saveX, buttonTextY, saveTxt);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(saveX, buttonTextY, saveTxt);
  }

  u8g2.sendBuffer();
}


void promptUserSaveSystemTime(RTCTime_t newSystemTime, StateMachine sm) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tr);

  const uint8_t screenW = u8g2.getDisplayWidth();
  const uint8_t screenH = u8g2.getDisplayHeight();

  // ===== Title =====
  const char* title = "Save system time";
  uint8_t titleWidth = u8g2.getStrWidth(title);
  uint8_t xCenter = (screenW - titleWidth) / 2;

  uint8_t titleY = 12;     // nice top margin
  u8g2.drawStr(xCenter, titleY, title);

  // ===== Centered Date/Time =====
  // Baseline around the visual center of the display
  uint8_t centerY = screenH / 2;  // baseline is okay because text height ~ 13px

  char buffer[30];
  sprintf(buffer, "%02d/%02d/20%02d %02d:%02d:%02d",
          newSystemTime.day,
          newSystemTime.month,
          newSystemTime.year,
          newSystemTime.hour,
          newSystemTime.minute,
          newSystemTime.second);

  u8g2.drawStr(10, centerY, buffer);

  // ===== Buttons =====
  const char* discardTxt = "DISCARD";
  const char* saveTxt    = "SAVE";

  uint8_t buttonTextY = screenH - 4;     // baseline
  uint8_t buttonBoxTop = screenH - 14;   // top of highlight box
  uint8_t buttonBoxH = 14;

  uint8_t discardX = 5;
  uint8_t saveX = screenW - u8g2.getStrWidth(saveTxt) - 5;

  bool saveActive = sm.USER_CONFIRMS;

  // --- DISCARD ---
  if (!saveActive) {
    uint8_t w = u8g2.getStrWidth(discardTxt) + 6;
    u8g2.drawBox(discardX - 3, buttonBoxTop, w, buttonBoxH);
    u8g2.setDrawColor(0);
    u8g2.drawStr(discardX, buttonTextY, discardTxt);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(discardX, buttonTextY, discardTxt);
  }

  // --- SAVE ---
  if (saveActive) {
    uint8_t w = u8g2.getStrWidth(saveTxt) + 6;
    u8g2.drawBox(saveX - 3, buttonBoxTop, w, buttonBoxH);
    u8g2.setDrawColor(0);
    u8g2.drawStr(saveX, buttonTextY, saveTxt);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(saveX, buttonTextY, saveTxt);
  }

  u8g2.sendBuffer();
}
