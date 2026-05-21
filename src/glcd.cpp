#include "glcd.h"
#include "pinout.h"
#include "states.h"

// U8G2_ST7920_128X64_F_HW_SPI u8g2(U8G2_R0, PIN_CS, PIN_RST);
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, PIN_SCK, PIN_MOSI, PIN_CS, PIN_RST);


bool updateScreenFlag = false;

bool displayOn = false;

void showInitialisationMessage(StateMachine sm){
  turnOnBacklight();
  displayOn = true;
  showMessage("Initialising"); 
}

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


void drawMenu(StateMachine sm, menuItem_t *items) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tr);

  uint8_t menu_length = getMenuLength(items);

  // ── 1. Find the index of the currently active item ──────────────────────────
  uint8_t activeIndex = 0;
  menuItem_t *tempItems = items;
  for (uint8_t i = 0; i < menu_length; i++) {
    if (strcmp(tempItems->data, sm.activeMenuItem) == 0) {
      activeIndex = i;
      break;
    }
    tempItems = tempItems->next;
  }

  // ── 2. Determine target scroll window ────────────────────────────────────────
  // Hard-coded to 4: the number of items that comfortably fit the screen
  const uint8_t COMFORTABLE_VISIBLE = 4;

  static uint8_t topIndex = 0;

  // Maximum topIndex that keeps the last item anchored at the bottom — no empty space
  uint8_t maxTopIndex = (menu_length > COMFORTABLE_VISIBLE)
                          ? (menu_length - COMFORTABLE_VISIBLE)
                          : 0;

  if (activeIndex < topIndex) {
    // Active item scrolled above view — pull window up
    topIndex = activeIndex;
  } else if (activeIndex >= topIndex + COMFORTABLE_VISIBLE) {
    // Active item scrolled below view — push down, clamped so last item stays at bottom
    uint8_t newTop = activeIndex - COMFORTABLE_VISIBLE + 1;
    topIndex = (newTop <= maxTopIndex) ? newTop : maxTopIndex;  // <= not 
  }

  // ── 3. Animate: smoothly move currentScrollY toward the target in pixels ─────
  static int16_t currentScrollY = 0;
  int16_t targetScrollY = topIndex * ITEM_HEIGHT;

  if (currentScrollY < targetScrollY) {
    currentScrollY = min((int16_t)(currentScrollY + SCROLL_SPEED), targetScrollY);
  } else if (currentScrollY > targetScrollY) {
    currentScrollY = max((int16_t)(currentScrollY - SCROLL_SPEED), targetScrollY);
  }

  // ── 4. Draw ──────────────────────────────────────────────────────────────────
  tempItems = items;
  for (uint8_t i = 0; i < menu_length; i++) {
    int16_t y = TOP_MARGIN + (int16_t)(i * ITEM_HEIGHT) - currentScrollY;

    if (y < -ITEM_HEIGHT || y > 64 + ITEM_HEIGHT) {
      tempItems = tempItems->next;
      continue;
    }

    if (strcmp(tempItems->data, sm.activeMenuItem) == 0) {
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - ITEM_HEIGHT + 2, 128, ITEM_HEIGHT);
      u8g2.setDrawColor(0);
    } else {
      u8g2.setDrawColor(1);
    }

    u8g2.drawStr(5, y, tempItems->data);
    tempItems = tempItems->next;
  }

  u8g2.sendBuffer();
}



void showDetailsScreen(RTCTime_t currentTime, StateMachine sm) {
  u8g2.clearBuffer();
  char buf[20];
  char stopBuf[20];

  // ── Layout (128 × 64 px) ──────────────────────────────────────────────────
  const uint8_t SCREEN_W = 128;
  const uint8_t SCREEN_H = 64;
  const uint8_t TOP_H    = 30;
  const uint8_t GAP      = 2;
  const uint8_t BOT_Y    = TOP_H + GAP;
  const uint8_t BOT_H    = SCREEN_H - BOT_Y;
  const uint8_t V_DIV    = 79;

  // ── Outer rectangle frames ────────────────────────────────────────────────
  u8g2.setDrawColor(1);
  u8g2.drawFrame(0, 0, SCREEN_W, TOP_H);
  u8g2.drawFrame(0, BOT_Y, SCREEN_W, BOT_H);

  // ═══════════════════════ TOP RECTANGLE ═══════════════════════════════════

  // Vertical divider
  u8g2.drawVLine(V_DIV, 1, TOP_H - 2);

  // TIME
  u8g2.setFont(u8g2_font_7x13B_tr);
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
  currentTime.hour, currentTime.minute, currentTime.second);

  uint8_t timeW = u8g2.getStrWidth(buf);
  uint8_t tx    = 1 + (V_DIV - 1 - timeW) / 2;

  u8g2.drawStr(tx, 14, buf);

  // ON / OFF
  const char *statusStr = sm.valve_on ? "ON" : "OFF";

  u8g2.setFont(u8g2_font_logisoso20_tf);

  uint8_t sw = u8g2.getStrWidth(statusStr);

  // true usable width inside right cell
  uint8_t rightInnerX = V_DIV + 1;
  uint8_t rightInnerW = SCREEN_W - rightInnerX - 1;

  uint8_t sx = rightInnerX + (rightInnerW - sw) / 2;

  // vertically centered better than previous version
  uint8_t sy = 26;

  u8g2.drawStr(sx, sy, statusStr);

  // TAGS
  const char *valveStr;
  switch (sm.selected_valve) {
    case CR01: valveStr = "CR01"; break;
    case CR02: valveStr = "CR02"; break;
    case CR03: valveStr = "CR03"; break;
    case CR04: valveStr = "CR04"; break;
    case CR05: valveStr = "CR05"; break;
    default:   valveStr = "----"; break;
  }

  const char *voltStr;
  switch (sm.selected_voltage) {
    case FIVE_VOLTS:        voltStr = "5V";  break;
    case SIX_VOLTS:         voltStr = "6V";  break;
    case NINE_VOLTS:        voltStr = "9V";  break;
    case TWELVE_VOLTS:      voltStr = "12V"; break;
    case TWENTY_FOUR_VOLTS: voltStr = "24V"; break;
    default:                voltStr = "--";  break;
  }

  u8g2.setFont(u8g2_font_5x8_tr);

  const uint8_t TAG_H   = 10;
  const uint8_t TAG_PAD = 3; // slightly larger padding
  const uint8_t TAG_Y   = TOP_H - TAG_H; // touches outer border directly

  // "VALVE"
  uint8_t t1w = u8g2.getStrWidth("VALVE") + TAG_PAD * 2;

  u8g2.setDrawColor(1);
  u8g2.drawBox(1, TAG_Y, t1w, TAG_H);

  u8g2.setDrawColor(0);
  u8g2.drawStr(1 + TAG_PAD, TAG_Y + TAG_H - 2, "VALVE");

  u8g2.setDrawColor(1);

  // Valve name
  uint8_t t2w = u8g2.getStrWidth(valveStr) + TAG_PAD * 2;
  uint8_t t3w = u8g2.getStrWidth("24V") + TAG_PAD * 2; // reserve max width
  uint8_t t2x = t1w + 1;

  // Draw line from valve to the first border on the right.
  u8g2.drawHLine(t2x, TAG_Y, t2w + t3w + 3); 
  // Draw valve string (e.g. "CR05")
  u8g2.drawStr(t2x + TAG_PAD, TAG_Y + TAG_H - 2, valveStr);
  // Get the x position of voltage string
  uint8_t t3x = t2x + t2w + 1;
  // Draw vertical line to separate valve string and voltage string
  u8g2.drawVLine(t3x, TAG_Y, TAG_H);


  uint8_t voltTextW = u8g2.getStrWidth(voltStr);
  uint8_t voltTx = t3x + (t3w - voltTextW) / 2;

  u8g2.drawStr(voltTx, TAG_Y + TAG_H - 2, voltStr);

  // ═══════════════════════ BOTTOM RECTANGLE ════════════════════════════════

  snprintf(buf, sizeof(buf), "Start: %02d:%02d",
  sm.irrigation_schedules[sm.schedule_index].schedule_time.startHour,
  sm.irrigation_schedules[sm.schedule_index].schedule_time.startMinute);

  snprintf(stopBuf, sizeof(stopBuf), "Stop : %02d:%02d",
  sm.irrigation_schedules[sm.schedule_index].schedule_time.stopHour,
  sm.irrigation_schedules[sm.schedule_index].schedule_time.stopMinute);

  u8g2.setFont(u8g2_font_6x13B_tr);
  
  if (sm.irrigation_schedules[sm.schedule_index].active) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(1, BOT_Y + 1, V_DIV - 1, BOT_H - 2);
    u8g2.setDrawColor(0);
  } else {
    u8g2.setDrawColor(1);
  }

  u8g2.drawStr(3, BOT_Y + 14, buf);
  u8g2.drawStr(3, BOT_Y + 28, stopBuf);
  u8g2.setDrawColor(1);

  // SCHEDULE BADGE
  const uint8_t BOX_X = V_DIV ;
  const uint8_t BOX_Y = BOT_Y + 1;
  const uint8_t BOX_W = SCREEN_W - BOX_X - 1;
  const uint8_t BOX_H = BOT_H - 2;

  // Single border only
  u8g2.drawVLine(BOX_X, BOX_Y, BOX_H);

  // "Sch"
  const uint8_t SCH_W = 18;
  const uint8_t SCH_H = 10;

  u8g2.drawBox(BOX_X + 1, BOX_Y, SCH_W, SCH_H);

  u8g2.setFont(u8g2_font_5x8_tr);

  u8g2.setDrawColor(0);
  u8g2.drawStr(BOX_X + 3, BOX_Y + SCH_H - 3, "Sch");

  u8g2.setDrawColor(1);

  // Enable/disable icon
  const uint8_t CX     = BOX_X + SCH_W / 2;
  const uint8_t CY     = BOX_Y + SCH_H + 1 + (BOX_H - SCH_H - 2) / 2;
  const uint8_t ICON_R = 7;

  u8g2.drawCircle(CX, CY, ICON_R);

  if (sm.irrigation_schedules[sm.schedule_index].enabled) {

    u8g2.drawLine(CX - 4, CY,     CX - 1, CY + 3);
    u8g2.drawLine(CX - 1, CY + 3, CX + 4, CY - 3);

  } else {

    u8g2.drawLine(CX - 3, CY - 3, CX + 3, CY + 3);
    u8g2.drawLine(CX + 3, CY - 3, CX - 3, CY + 3);
  }

  // Schedule number
  u8g2.setFont(u8g2_font_logisoso24_tn);

  snprintf(buf, sizeof(buf), "%d", sm.schedule_index + 1);

  uint8_t numW     = u8g2.getStrWidth(buf);
  uint8_t numAreaX = BOX_X + SCH_W + 2;
  uint8_t numAreaW = BOX_W - SCH_W - 3;

  uint8_t nx = numAreaX + (numAreaW - numW) / 2;
  uint8_t ny = BOX_Y + BOX_H - 2;

  u8g2.drawStr(nx, ny, buf);

  u8g2.sendBuffer();
}


void showIrrigationTimeSetting(ScheduleTime_t* schedule, IrrigationTimeEditingField editingField) {
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

void promptUserSaveNewSchedule(ScheduleTime_t schedule, StateMachine sm) {
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



void promptUserSaveNewValveType(StateMachine sm, ValveType *temporary_valve_type) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tr);
  
  const uint8_t screenW = u8g2.getDisplayWidth();
  const uint8_t screenH = u8g2.getDisplayHeight();
  
  // ===== Title =====
  const char* title = "Selected Valve";
  uint8_t titleWidth = u8g2.getStrWidth(title);
  u8g2.drawStr((screenW - titleWidth) / 2, 12, title);
  
  // ===== Selected Valve Name =====
  const char* valveName = VALVE_TYPE_NAMES[*temporary_valve_type];
  uint8_t valveNameWidth = u8g2.getStrWidth(valveName);
  u8g2.drawStr((screenW - valveNameWidth) / 2, screenH / 2, valveName);

  // ===== Buttons =====
  const char* discardTxt = "DISCARD";
  const char* saveTxt    = "SAVE";

  uint8_t buttonTextY  = screenH - 4;
  uint8_t buttonBoxTop = screenH - 14;
  uint8_t buttonBoxH   = 14;

  uint8_t discardX = 5;
  uint8_t saveX    = screenW - u8g2.getStrWidth(saveTxt) - 5;

  bool saveActive = sm.USER_CONFIRMS;

  // --- DISCARD ---
  if (!saveActive) {
    u8g2.drawBox(discardX - 3, buttonBoxTop, u8g2.getStrWidth(discardTxt) + 6, buttonBoxH);
    u8g2.setDrawColor(0);
    u8g2.drawStr(discardX, buttonTextY, discardTxt);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(discardX, buttonTextY, discardTxt);
  }

  // --- SAVE ---
  if (saveActive) {
    u8g2.drawBox(saveX - 3, buttonBoxTop, u8g2.getStrWidth(saveTxt) + 6, buttonBoxH);
    u8g2.setDrawColor(0);
    u8g2.drawStr(saveX, buttonTextY, saveTxt);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(saveX, buttonTextY, saveTxt);
  }

  u8g2.sendBuffer();
}


void promptUserSaveNewVoltage(StateMachine sm, ValveVoltage *temporary_voltage) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tr);
  
  const uint8_t screenW = u8g2.getDisplayWidth();
  const uint8_t screenH = u8g2.getDisplayHeight();
  
  // ===== Title =====
  const char* title = "Selected Voltage";
  uint8_t titleWidth = u8g2.getStrWidth(title);
  u8g2.drawStr((screenW - titleWidth) / 2, 12, title);
  
  // ===== Selected Voltage Name =====
  const char* voltageName = VALVE_VOLTAGE_NAMES[*temporary_voltage];
  uint8_t voltageNameWidth = u8g2.getStrWidth(voltageName);
  u8g2.drawStr((screenW - voltageNameWidth) / 2, screenH / 2, voltageName);

  // ===== Buttons =====
  const char* discardTxt = "DISCARD";
  const char* saveTxt    = "SAVE";

  uint8_t buttonTextY  = screenH - 4;
  uint8_t buttonBoxTop = screenH - 14;
  uint8_t buttonBoxH   = 14;

  uint8_t discardX = 5;
  uint8_t saveX    = screenW - u8g2.getStrWidth(saveTxt) - 5;

  bool saveActive = sm.USER_CONFIRMS;

  // --- DISCARD ---
  if (!saveActive) {
    u8g2.drawBox(discardX - 3, buttonBoxTop, u8g2.getStrWidth(discardTxt) + 6, buttonBoxH);
    u8g2.setDrawColor(0);
    u8g2.drawStr(discardX, buttonTextY, discardTxt);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(discardX, buttonTextY, discardTxt);
  }

  // --- SAVE ---
  if (saveActive) {
    u8g2.drawBox(saveX - 3, buttonBoxTop, u8g2.getStrWidth(saveTxt) + 6, buttonBoxH);
    u8g2.setDrawColor(0);
    u8g2.drawStr(saveX, buttonTextY, saveTxt);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(saveX, buttonTextY, saveTxt);
  }

  u8g2.sendBuffer();
}


// ── Shared grid constants (put in your header or top of the file) ────────────
#define GRID_HEADER_H   15
#define GRID_COLS       3
#define GRID_ROWS       2
#define GRID_TOP        (GRID_HEADER_H + 1)               // 16
#define GRID_BOTTOM     53                                 // leaves 10px footer
#define GRID_ROW_H      ((GRID_BOTTOM - GRID_TOP) / GRID_ROWS)  // 18
#define GRID_CELL_W     (128 / GRID_COLS)                 // 42  (last col gets 44)

void showMessage(const char *text) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tr);

  const uint8_t LINE_HEIGHT = 13;
  const uint8_t SCREEN_W    = 128;
  const uint8_t SCREEN_H    = 64;
  const uint8_t MAX_LINES   = 4;

  // ── Word-wrap: build lines greedily ──────────────────────────────────────────
  char lines[MAX_LINES][24];   // 128px / 6px per char ≈ 21 chars max per line
  uint8_t lineCount = 0;

  char buf[128];
  strncpy(buf, text, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  char currentLine[24] = "";
  char testLine[24];

  char *word = strtok(buf, " ");
  while (word != nullptr && lineCount < MAX_LINES) {
    if (strlen(currentLine) == 0) {
      strncpy(testLine, word, sizeof(testLine) - 1);
    } else {
      snprintf(testLine, sizeof(testLine), "%s %s", currentLine, word);
    }
    testLine[sizeof(testLine) - 1] = '\0';

    if (u8g2.getStrWidth(testLine) <= SCREEN_W) {
      // Word fits on the current line — keep building
      strncpy(currentLine, testLine, sizeof(currentLine) - 1);
    } else {
      // Flush the current line and start a new one with this word
      strncpy(lines[lineCount++], currentLine, sizeof(lines[0]) - 1);
      strncpy(currentLine, word, sizeof(currentLine) - 1);
    }
    word = strtok(nullptr, " ");
  }

  // Flush whatever is left in currentLine
  if (strlen(currentLine) > 0 && lineCount < MAX_LINES) {
    strncpy(lines[lineCount++], currentLine, sizeof(lines[0]) - 1);
  }

  // ── Vertically center the block of lines ─────────────────────────────────────
  uint8_t totalHeight = lineCount * LINE_HEIGHT;
  uint8_t startY      = (SCREEN_H - totalHeight) / 2 + LINE_HEIGHT;

  // ── Draw each line horizontally centered ─────────────────────────────────────
  for (uint8_t i = 0; i < lineCount; i++) {
    uint8_t w = u8g2.getStrWidth(lines[i]);
    uint8_t x = (SCREEN_W - w) / 2;
    uint8_t y = startY + i * LINE_HEIGHT;
    u8g2.drawStr(x, y, lines[i]);
  }

  u8g2.sendBuffer();
}

void showSchedule(const char *schedule, uint8_t current_position) {
  const uint8_t SCHEDULE_COUNT = 4;

  u8g2.clearBuffer();

  // ── Header ───────────────────────────────────────────────────────────────────
  u8g2.setFont(u8g2_font_6x13_tr);
  u8g2.setDrawColor(1);
  u8g2.drawBox(0, 0, 128, 15);
  u8g2.setDrawColor(0);
  u8g2.drawStr(5, 13, "Schedules");
  u8g2.setDrawColor(1);

  // ── Schedule name — centered in space between header and footer ───────────────
  uint8_t textWidth = u8g2.getStrWidth(schedule);
  uint8_t x = (128 - textWidth) / 2;
  u8g2.drawStr(x, 40, schedule);

  // ── Position indicator dots ───────────────────────────────────────────────────
  // e.g. • • ● • for 4 schedules with position 2 active (0-indexed)
  const uint8_t DOT_SIZE     = 4;    // filled dot diameter
  const uint8_t DOT_SPACING  = 10;
  const uint8_t DOTS_TOTAL_W = SCHEDULE_COUNT * DOT_SPACING - (DOT_SPACING - DOT_SIZE);
  uint8_t dotStartX          = (128 - DOTS_TOTAL_W) / 2;
  const uint8_t DOT_Y        = 57;   // near the bottom

  for (uint8_t i = 0; i < SCHEDULE_COUNT; i++) {
    uint8_t dx = dotStartX + i * DOT_SPACING;
    if (i == current_position) {
      u8g2.drawBox(dx, DOT_Y, DOT_SIZE, DOT_SIZE);      // filled = active
    } else {
      u8g2.drawFrame(dx, DOT_Y, DOT_SIZE, DOT_SIZE);    // outline = inactive
    }
  }

  u8g2.sendBuffer();
}




// ── Shared implementation ─────────────────────────────────────────────────────
static void showSelectionList(const char *const *names, uint8_t item_count, uint8_t activeIndex) {
  const uint8_t COMFORTABLE_VISIBLE = 4;

  static uint8_t topIndex      = 0;
  static int16_t currentScrollY = 0;

  // ── Scroll window ─────────────────────────────────────────────────────────
  uint8_t maxTopIndex = (item_count > COMFORTABLE_VISIBLE)
                      ? (item_count - COMFORTABLE_VISIBLE) : 0;

  if (activeIndex < topIndex) {
    topIndex = activeIndex;
  } else if (activeIndex >= topIndex + COMFORTABLE_VISIBLE) {
    uint8_t newTop = activeIndex - COMFORTABLE_VISIBLE + 1;
    topIndex = (newTop <= maxTopIndex) ? newTop : maxTopIndex;
  }

  // ── Smooth scroll ─────────────────────────────────────────────────────────
  int16_t targetScrollY = topIndex * ITEM_HEIGHT;
  if (currentScrollY < targetScrollY)
    currentScrollY = min((int16_t)(currentScrollY + SCROLL_SPEED), targetScrollY);
  else if (currentScrollY > targetScrollY)
    currentScrollY = max((int16_t)(currentScrollY - SCROLL_SPEED), targetScrollY);

  // ── Draw ──────────────────────────────────────────────────────────────────
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_tr);

  for (uint8_t i = 0; i < item_count; i++) {
    int16_t y = TOP_MARGIN + (int16_t)(i * ITEM_HEIGHT) - currentScrollY;

    if (y < -ITEM_HEIGHT || y > 64 + ITEM_HEIGHT) continue;

    if (i == activeIndex) {
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - ITEM_HEIGHT + 2, 128, ITEM_HEIGHT);
      u8g2.setDrawColor(0);
    } else {
      u8g2.setDrawColor(1);
    }

    u8g2.drawStr(5, y, names[i]);
  }

  u8g2.sendBuffer();
}

void showValveType(const char *const *valve_type_names, ValveType *temporary_valve_type_id) {
  showSelectionList(valve_type_names, 6, (uint8_t)(*temporary_valve_type_id));
}

void showVoltage(const char *const *valve_voltage_names, ValveVoltage *temporary_valve_voltage_id) {
  showSelectionList(valve_voltage_names, 6, (uint8_t)(*temporary_valve_voltage_id));
}

// void showVoltage(const char *const *valve_voltage_names, ValveVoltage *temporary_valve_voltage_id) {
//   const uint8_t ITEM_COUNT = 5;

//   // ── Clamp — hard stop at both ends ───────────────────────────────────────────
//   if ((int8_t)*temporary_valve_voltage_id < 0)                   *temporary_valve_voltage_id = (ValveVoltage)0;
//   if ((int8_t)*temporary_valve_voltage_id >= (int8_t)ITEM_COUNT) *temporary_valve_voltage_id = (ValveVoltage)(ITEM_COUNT - 1);
//   uint8_t activeIndex = (uint8_t)(*temporary_valve_voltage_id);

//   u8g2.clearBuffer();

//   // ── Header ───────────────────────────────────────────────────────────────────
//   u8g2.setFont(u8g2_font_6x13_tr);
//   u8g2.setDrawColor(1);
//   u8g2.drawBox(0, 0, 128, GRID_HEADER_H);
//   u8g2.setDrawColor(0);
//   u8g2.drawStr(5, GRID_HEADER_H - 2, "Select Voltage");
//   u8g2.setDrawColor(1);

//   // ── Grid ─────────────────────────────────────────────────────────────────────
//   u8g2.setFont(u8g2_font_5x8_tr);

//   for (uint8_t i = 0; i < GRID_ROWS * GRID_COLS; i++) {
//     uint8_t col = i % GRID_COLS;
//     uint8_t row = i / GRID_COLS;

//     uint8_t bx = col * GRID_CELL_W;
//     uint8_t by = GRID_TOP + row * GRID_ROW_H;
//     uint8_t cw = (col == GRID_COLS - 1) ? (128 - bx) : GRID_CELL_W;

//     u8g2.drawFrame(bx, by, cw, GRID_ROW_H);

//     if (i >= ITEM_COUNT) continue;

//     if (i == activeIndex) {
//       u8g2.drawBox(bx + 1, by + 1, cw - 2, GRID_ROW_H - 2);
//       u8g2.setDrawColor(0);
//     }

//     uint8_t textW = u8g2.getStrWidth(valve_voltage_names[i]);
//     uint8_t tx    = bx + (cw - textW) / 2;
//     uint8_t ty    = by + (GRID_ROW_H + 6) / 2;
//     u8g2.drawStr(tx, ty, valve_voltage_names[i]);

//     u8g2.setDrawColor(1);
//   }

//   // ── Footer ───────────────────────────────────────────────────────────────────
//   u8g2.setFont(u8g2_font_5x8_tr);
//   u8g2.drawStr(5, 63, "Long-press to save");

//   u8g2.sendBuffer();
// }



// void showValveType(const char *const *valve_type_names, ValveType *temporary_valve_type) {
//   const uint8_t ITEM_COUNT  = 5;
//   const uint8_t COLS        = 2;
//   const uint8_t ROWS        = 2;
//   const uint8_t VISIBLE     = COLS * ROWS;          // 4 items on screen
//   const uint8_t CELL_W      = 128 / COLS;           // 64px per cell
//   const uint8_t HEADER_H    = 15;
//   const uint8_t FOOTER_H    = 10;
//   const uint8_t GRID_TOP    = HEADER_H + 1;         // 16
//   const uint8_t GRID_BOTTOM = 64 - FOOTER_H - 1;    // 53
//   const uint8_t ROW_H       = (GRID_BOTTOM - GRID_TOP) / ROWS;  // ~18px

//   // ── Clamp — hard stop, no wrapping ────────────────────────────────────────
//   ValveType vt = *temporary_valve_type;
//   if(vt == VALVE_NOT_SELECTED) vt = CR01;
//   uint8_t activeIndex = (uint8_t)(vt);

//   // ── Row-aligned scroll window ──────────────────────────────────────────────
//   // topIndex is always a multiple of COLS so full rows enter/exit together
//   static uint8_t topIndex  = 0;
//   uint8_t maxTopIndex = (ITEM_COUNT > VISIBLE) ? (((ITEM_COUNT - 1) / COLS - ROWS + 1) * COLS) : 0;

//   if (activeIndex < topIndex) {
//     topIndex = (activeIndex / COLS) * COLS;                       // snap up to active row
//   } else if (activeIndex >= topIndex + VISIBLE) {
//     uint8_t newTop = ((activeIndex - VISIBLE + COLS) / COLS) * COLS;  // snap down to active row
//     topIndex = (newTop <= maxTopIndex) ? newTop : maxTopIndex;
//   }

//   // ── Smooth vertical scroll animation ──────────────────────────────────────
//   static int16_t currentScrollY = 0;
//   int16_t targetScrollY = (int16_t)(topIndex / COLS) * ROW_H;

//   if (currentScrollY < targetScrollY)
//     currentScrollY = min((int16_t)(currentScrollY + SCROLL_SPEED), targetScrollY);
//   else if (currentScrollY > targetScrollY)
//     currentScrollY = max((int16_t)(currentScrollY - SCROLL_SPEED), targetScrollY);

//   // ── Draw ──────────────────────────────────────────────────────────────────
//   u8g2.clearBuffer();
//   u8g2.setFont(u8g2_font_6x13_tr);

//   // Header — filled bar, inverted text
//   u8g2.setDrawColor(1);
//   u8g2.drawBox(0, 0, 128, HEADER_H);
//   u8g2.setDrawColor(0);
//   u8g2.drawStr(5, HEADER_H - 2, "Select Valve");
//   u8g2.setDrawColor(1);

//   // Grid — clipped so rows never bleed into header or footer
//   u8g2.setClipWindow(0, GRID_TOP, 127, GRID_BOTTOM);

//   for (uint8_t i = 0; i < ITEM_COUNT; i++) {
//     uint8_t col        = i % COLS;
//     uint8_t virtualRow = i / COLS;

//     int16_t bx = col * CELL_W;
//     int16_t by = GRID_TOP + (int16_t)(virtualRow * ROW_H) - currentScrollY;

//     // Cull rows fully outside the grid area
//     if (by + (int16_t)ROW_H < GRID_TOP || by > GRID_BOTTOM) continue;

//     if (i == activeIndex) {
//       u8g2.setDrawColor(1);
//       u8g2.drawBox(bx, by, CELL_W, ROW_H);
//       u8g2.setDrawColor(0);
//     } else {
//       u8g2.setDrawColor(1);
//     }

//     u8g2.drawStr(bx + 3, by + 13, valve_type_names[i]);  // 13 = font ascent
//   }

//   u8g2.setMaxClipWindow();
//   u8g2.setDrawColor(1);

//   // Footer — plain text, smaller font
//   u8g2.setFont(u8g2_font_6x10_tr);
//   u8g2.drawStr(5, 63, "Long-press to save");

//   u8g2.sendBuffer();
// }

// void showVoltage(const char *const *valve_voltage_names, ValveVoltage *temporary_valve_voltage) {
//   const uint8_t ITEM_COUNT  = 5;
//   const uint8_t COLS        = 2;
//   const uint8_t ROWS        = 2;
//   const uint8_t VISIBLE     = COLS * ROWS;
//   const uint8_t CELL_W      = 128 / COLS;
//   const uint8_t HEADER_H    = 15;
//   const uint8_t FOOTER_H    = 10;
//   const uint8_t GRID_TOP    = HEADER_H + 1;
//   const uint8_t GRID_BOTTOM = 64 - FOOTER_H - 1;
//   const uint8_t ROW_H       = (GRID_BOTTOM - GRID_TOP) / ROWS;

//   // ── Clamp — hard stop, no wrapping ────────────────────────────────────────
//   ValveVoltage v = *temporary_valve_voltage;
//   if(v == VOLTAGE_NOT_SELECTED) v = FIVE_VOLTS;
//   uint8_t activeIndex = (uint8_t)(v);

//   // ── Row-aligned scroll window ──────────────────────────────────────────────
//   static uint8_t topIndex = 0;
//   uint8_t maxTopIndex = (ITEM_COUNT > VISIBLE)
//       ? (((ITEM_COUNT - 1) / COLS - ROWS + 1) * COLS)
//       : 0;

//   if (activeIndex < topIndex) {
//     topIndex = (activeIndex / COLS) * COLS;
//   } else if (activeIndex >= topIndex + VISIBLE) {
//     uint8_t newTop = ((activeIndex - VISIBLE + COLS) / COLS) * COLS;
//     topIndex = (newTop <= maxTopIndex) ? newTop : maxTopIndex;
//   }

//   // ── Smooth vertical scroll animation ──────────────────────────────────────
//   static int16_t currentScrollY = 0;
//   int16_t targetScrollY = (int16_t)(topIndex / COLS) * ROW_H;

//   if (currentScrollY < targetScrollY)
//     currentScrollY = min((int16_t)(currentScrollY + SCROLL_SPEED), targetScrollY);
//   else if (currentScrollY > targetScrollY)
//     currentScrollY = max((int16_t)(currentScrollY - SCROLL_SPEED), targetScrollY);

//   // ── Draw ──────────────────────────────────────────────────────────────────
//   u8g2.clearBuffer();
//   u8g2.setFont(u8g2_font_6x13_tr);

//   // Header
//   u8g2.setDrawColor(1);
//   u8g2.drawBox(0, 0, 128, HEADER_H);
//   u8g2.setDrawColor(0);
//   u8g2.drawStr(5, HEADER_H - 2, "Select Voltage");
//   u8g2.setDrawColor(1);

//   // Grid
//   u8g2.setClipWindow(0, GRID_TOP, 127, GRID_BOTTOM);

//   for (uint8_t i = 0; i < ITEM_COUNT; i++) {
//     uint8_t col        = i % COLS;
//     uint8_t virtualRow = i / COLS;

//     int16_t bx = col * CELL_W;
//     int16_t by = GRID_TOP + (int16_t)(virtualRow * ROW_H) - currentScrollY;

//     if (by + (int16_t)ROW_H < GRID_TOP || by > GRID_BOTTOM) continue;

//     if (i == activeIndex) {
//       u8g2.setDrawColor(1);
//       u8g2.drawBox(bx, by, CELL_W, ROW_H);
//       u8g2.setDrawColor(0);
//     } else {
//       u8g2.setDrawColor(1);
//     }

//     u8g2.drawStr(bx + 3, by + 13, valve_voltage_names[i]);
//   }

//   u8g2.setMaxClipWindow();
//   u8g2.setDrawColor(1);

//   // Footer
//   u8g2.setFont(u8g2_font_6x10_tr);
//   u8g2.drawStr(5, 63, "Long-press to save");

//   u8g2.sendBuffer();
// }


// void showIrrigationSchedule(StateMachine *sm){
//   bool enabled = sm->irrigation_schedules[sm->schedule_index].enabled;
//   uint8_t start_hour = sm->irrigation_schedules[sm->schedule_index].startHour;
//   uint8_t start_minute = sm->irrigation_schedules[sm->schedule_index].startMinute;
//   uint8_t stop_hour = sm->irrigation_schedules[sm->schedule_index].stopHour;
//   uint8_t stop_minute = sm->irrigation_schedules[sm->schedule_index].stopMinute;
//   const char * str_start = "Start:";
//   const char * str_start = "Stop:";
//   const char * str_enable_status = enabled? "ENABLED": "DISABLED";
//   const char * str_schedules = "SCHEDULES"
// }

/*


void showSchedules(IrrigationSchedule_t schedules[], ScheduleIndex_t id){
  const char * title = "Schedules";
  const char * str_enabled_status = schedules[id].enabled? "Enabled": "Disabled";
  // A row with number 1 to 4 and Exit at the last cell. The cell with the number matching the id should be highlighted
  // |    1    |#####2#####|     3     |     4     |     Exit    |  if id = 1. If id is 4, highlight "Exit"
  const char * str_start = "Start: ";
  const char * str_stop  = "Stop: ";
  uint8_t start_hour = schedules[id].schedule_time.startHour;
  uint8_t start_minute = schedules[id].schedule_time.startMinute;
  uint8_t stop_hour = schedules[id].schedule_time.stopHour;
  uint8_t stop_minute = schedules[id].schedule_time.stopMinute;
  uint8_t highlighted_option = schedules[id].selected_option;

  uint8_t numOfOptions = sizeof(schedules[id].schedule_options);

  if(numOfOptions == 3){
    // We will print 3 options on the bottom row
    schedules[id].schedule_options[0];
    schedules[id].schedule_options[1];
    schedules[id].schedule_options[2];
    // Display "|  Opt 1   ||  Opt 2   ||  Opt 3   |"  at the bottom
    // Highlight the cell whose number matches <highlighted_option>
  } else if(numOfOptions == 2){
    // We will print 2 options on the bottom row
    schedules[id].schedule_options[0];
    schedules[id].schedule_options[1];
    // Display "|  Opt 1   |    Space between   |  Opt 2   |"  at the bottom
    // Highlight the cell whose number matches <highlighted_option>
  } else if(numOfOptions == 1){
    // We will print the only option at the center
    schedules[id].schedule_options[0];
    // Display "|   Space  |    Only Option displayed as text   |  Space  |"  at the bottom
  }


}

void showSchedules(StateMachine sm) {
  const uint8_t NUM_SCHEDULES = 4;
  const uint8_t HEADER_H     = 12;
  const uint8_t OPT_TOP      = 44;   // options bar top
  const uint8_t OPT_H        = 10;   // options bar height
  const uint8_t NAV_TOP      = 54;   // navigation tabs top
  const uint8_t NAV_H        = 10;   // navigation tabs height

  uint8_t currentId = (uint8_t)sm.schedule_index;

  u8g2.clearBuffer();

  // ── Header ─────────────────────────────────────────────────────────────────
  u8g2.setFont(u8g2_font_6x13_tr);
  u8g2.setDrawColor(1);
  u8g2.drawBox(0, 0, 128, HEADER_H);
  u8g2.setDrawColor(0);
  u8g2.drawStr(5, HEADER_H - 2, "Schedules");
  u8g2.setDrawColor(1);

  u8g2.setFont(u8g2_font_5x8_tr);

  // ── Schedule content (only when a valid schedule is selected, not Exit) ─────
  if (currentId < NUM_SCHEDULES) {

    // Status
    const char *status = sm.irrigation_schedules[currentId].enabled? "Enabled" : "Disabled";
    u8g2.drawStr(5, 21, status);

    // Times — treat 255 as unset (show 00:00)
    uint8_t sH = sm.irrigation_schedules[currentId].schedule_time.startHour;
    uint8_t sM = sm.irrigation_schedules[currentId].schedule_time.startMinute;
    uint8_t eH = sm.irrigation_schedules[currentId].schedule_time.stopHour;
    uint8_t eM = sm.irrigation_schedules[currentId].schedule_time.stopMinute;
    if (sH == 255) sH = 0;
    if (sM == 255) sM = 0;
    if (eH == 255) eH = 0;
    if (eM == 255) eM = 0;

    char buf[20];
    sprintf(buf, "Start: %02d:%02d", sH, sM);
    u8g2.drawStr(5, 32, buf);
    sprintf(buf, "Stop:  %02d:%02d", eH, eM);
    u8g2.drawStr(5, 43, buf);

    // ── Count options via nullptr sentinel ──────────────────────────────────
    uint8_t numOptions = 0;
    const char **opts = sm.irrigation_schedules[currentId].schedule_options;

    if (opts != nullptr) {
      while (opts[numOptions] != nullptr) numOptions++;
    }

    uint8_t sel = sm.irrigation_schedules[currentId].selected_option;

    // ── Options bar ─────────────────────────────────────────────────────────
    if (numOptions == 3) {
      // Three equal cells across full width
      const uint8_t CELL_W = 128 / 3;   // 42px; last cell gets remainder
      for (uint8_t i = 0; i < 3; i++) {
        uint8_t bx = i * CELL_W;
        uint8_t cw = (i == 2) ? (128 - bx) : CELL_W;
        u8g2.setDrawColor(1);
        u8g2.drawFrame(bx, OPT_TOP, cw, OPT_H);
        if (i == sel) {
          u8g2.drawBox(bx + 1, OPT_TOP + 1, cw - 2, OPT_H - 2);
          u8g2.setDrawColor(0);
        }
        uint8_t tw = u8g2.getStrWidth(opts[i]);
        u8g2.drawStr(bx + (cw - tw) / 2, OPT_TOP + OPT_H - 1, opts[i]);
        u8g2.setDrawColor(1);
      }

    } else if (numOptions == 2) {
      // Two cells: one at each edge, gap in the middle
      const uint8_t OPT_W = 45;
      const uint8_t OPT_RIGHT_X = 128 - OPT_W;

      // Left cell
      u8g2.setDrawColor(1);
      u8g2.drawFrame(0, OPT_TOP, OPT_W, OPT_H);
      if (sel == 0) {
        u8g2.drawBox(1, OPT_TOP + 1, OPT_W - 2, OPT_H - 2);
        u8g2.setDrawColor(0);
      }
      uint8_t tw0 = u8g2.getStrWidth(opts[0]);
      u8g2.drawStr((OPT_W - tw0) / 2, OPT_TOP + OPT_H - 1, opts[0]);

      // Right cell
      u8g2.setDrawColor(1);
      u8g2.drawFrame(OPT_RIGHT_X, OPT_TOP, OPT_W, OPT_H);
      if (sel == 1) {
        u8g2.drawBox(OPT_RIGHT_X + 1, OPT_TOP + 1, OPT_W - 2, OPT_H - 2);
        u8g2.setDrawColor(0);
      }
      uint8_t tw1 = u8g2.getStrWidth(opts[1]);
      u8g2.drawStr(OPT_RIGHT_X + (OPT_W - tw1) / 2, OPT_TOP + OPT_H - 1, opts[1]);
      u8g2.setDrawColor(1);

    } else if (numOptions == 1) {
      // Single option: centred text, no border
      uint8_t tw = u8g2.getStrWidth(opts[0]);
      u8g2.setDrawColor(1);
      u8g2.drawStr((128 - tw) / 2, OPT_TOP + OPT_H - 1, opts[0]);
    }
    // numOptions == 0 (BROWSING): nothing drawn in options area
  }

  u8g2.setDrawColor(1);

  // ── Navigation tabs: | 1 | 2 | 3 | 4 | Exit | ─────────────────────────────
  const char *NAV_LABELS[5] = {"1", "2", "3", "4", "Exit"};
  const uint8_t TAB_W = 128 / 5;   // 25px; last tab gets remainder (28px)

  for (uint8_t i = 0; i < 5; i++) {
    uint8_t bx = i * TAB_W;
    uint8_t cw = (i == 4) ? (128 - bx) : TAB_W;

    u8g2.setDrawColor(1);
    u8g2.drawFrame(bx, NAV_TOP, cw, NAV_H);

    if (currentId == i) {   // id 0→tab "1", 1→tab "2", 4→tab "Exit"
      u8g2.drawBox(bx + 1, NAV_TOP + 1, cw - 2, NAV_H - 2);
      u8g2.setDrawColor(0);
    }

    uint8_t tw = u8g2.getStrWidth(NAV_LABELS[i]);
    u8g2.drawStr(bx + (cw - tw) / 2, NAV_TOP + NAV_H - 1, NAV_LABELS[i]);
    u8g2.setDrawColor(1);
  }

  u8g2.sendBuffer();
}

*/



void showSchedules(StateMachine sm) {
  const uint8_t NUM_SCHEDULES = 4;
  const uint8_t HEADER_H  = 12;
  const uint8_t NAV_TOP   = 12;
  const uint8_t NAV_H     = 11;
  const uint8_t STATUS_Y  = 33;   // baseline — right-aligned
  const uint8_t START_Y   = 34;   // baseline — centered
  const uint8_t STOP_Y    = 44;   // baseline — centered
  const uint8_t OPT_TOP   = 53;
  const uint8_t OPT_H     = 11;
  const uint8_t FONT_H    = 8;    // u8g2_font_5x8_tr glyph height

  uint8_t currentId = (uint8_t)sm.schedule_index;

  // ── Blink state (only active during SCHEDULE_SELECTION) ────────────────────
  static unsigned long lastBlinkTime = 0;
  static bool blinkState = false;
  if (millis() - lastBlinkTime >= 500) {
    blinkState = !blinkState;
    lastBlinkTime = millis();
  }
  bool editing = (sm.schedule_state== SCHEDULE_SET_TIME );

  u8g2.clearBuffer();

  // ── Header — centered ──────────────────────────────────────────────────────
  u8g2.setFont(u8g2_font_6x13_tr);
  u8g2.setDrawColor(1);
  u8g2.drawBox(0, 0, 128, HEADER_H);
  u8g2.setDrawColor(0);
  const char *title = "Schedules";
  u8g2.drawStr((128 - u8g2.getStrWidth(title)) / 2, HEADER_H - 2, title);
  u8g2.setDrawColor(1);

  u8g2.setFont(u8g2_font_5x8_tr);

// ── Navigation tabs ─────────────────────────────────────────────────────────
  const char *NAV_LABELS[5] = {"1", "2", "3", "4", "Exit"};
  const uint8_t TAB_W = 128 / 5;

  for (uint8_t i = 0; i < 5; i++) {
    uint8_t bx = i * TAB_W;
    uint8_t cw = (i == 4) ? (128 - bx) : TAB_W;

    u8g2.setDrawColor(1);
    u8g2.drawFrame(bx, NAV_TOP, cw, NAV_H);

    bool isActive = (currentId == i);

    if (isActive && blinkState) {
      // Invert: filled box + white text
      u8g2.drawBox(bx + 1, NAV_TOP + 1, cw - 2, NAV_H - 2);
      u8g2.setDrawColor(0);
    }
    // When isActive && !blinkState: just the frame is shown — tab "uninverts"

    uint8_t tw = u8g2.getStrWidth(NAV_LABELS[i]);
    u8g2.drawStr(bx + (cw - tw) / 2, NAV_TOP + NAV_H - 1, NAV_LABELS[i]);
    u8g2.setDrawColor(1);
  }

  // ── Schedule content ────────────────────────────────────────────────────────
  if (currentId < NUM_SCHEDULES) {

    // ── Enabled/Disabled icon — right side of content area ───────────────────
    // Tick-in-circle = Enabled,  X-in-circle = Disabled
    const uint8_t ICO_CX = 113;   // icon center x (right side, 15px from edge)
    const uint8_t ICO_CY = 38;    // icon center y (mid content area)
    const uint8_t ICO_R  = 7;     // circle radius

    u8g2.drawCircle(ICO_CX, ICO_CY, ICO_R);

    if (sm.irrigation_schedules[currentId].enabled) {
      // ✓  Checkmark: short upstroke then long upstroke to the right
      u8g2.drawLine(ICO_CX - 4, ICO_CY + 1, ICO_CX - 1, ICO_CY + 4);   // short leg
      u8g2.drawLine(ICO_CX - 1, ICO_CY + 4, ICO_CX + 4, ICO_CY - 3);   // long leg
      // Thicken by repeating one pixel up
      u8g2.drawLine(ICO_CX - 4, ICO_CY,     ICO_CX - 1, ICO_CY + 3);
      u8g2.drawLine(ICO_CX - 1, ICO_CY + 3, ICO_CX + 4, ICO_CY - 4);
    } else {
      // ✕  Cross: two diagonal lines
      u8g2.drawLine(ICO_CX - 4, ICO_CY - 4, ICO_CX + 4, ICO_CY + 4);
      u8g2.drawLine(ICO_CX + 4, ICO_CY - 4, ICO_CX - 4, ICO_CY + 4);
      // Thicken
      u8g2.drawLine(ICO_CX - 3, ICO_CY - 4, ICO_CX + 4, ICO_CY + 3);
      u8g2.drawLine(ICO_CX + 3, ICO_CY - 4, ICO_CX - 4, ICO_CY + 3);
    }

    // ── Start / Stop — centered, moved up to fill the space the label used ───
    // const uint8_t START_Y = 34;
    // const uint8_t STOP_Y  = 44;

    // Resolve times (255 = unset → show 00)
    uint8_t sH = sm.irrigation_schedules[currentId].schedule_time.startHour;
    uint8_t sM = sm.irrigation_schedules[currentId].schedule_time.startMinute;
    uint8_t eH = sm.irrigation_schedules[currentId].schedule_time.stopHour;
    uint8_t eM = sm.irrigation_schedules[currentId].schedule_time.stopMinute;
    if (sH == 255) sH = 0;  if (sM == 255) sM = 0;
    if (eH == 255) eH = 0;  if (eM == 255) eM = 0;

    char sHStr[3], sMStr[3], eHStr[3], eMStr[3];
    sprintf(sHStr, "%02d", sH);  sprintf(sMStr, "%02d", sM);
    sprintf(eHStr, "%02d", eH);  sprintf(eMStr, "%02d", eM);

    // ── Time display ─────────────────────────────────────────────────────────
    // Build full strings to get centered x for each line
    char startBuf[14], stopBuf[14];
    sprintf(startBuf, "Start: %s:%s", sHStr, sMStr);
    sprintf(stopBuf,  "Stop:  %s:%s", eHStr, eMStr);

    uint8_t startX = (128 - u8g2.getStrWidth(startBuf)) / 2;
    uint8_t stopX  = (128 - u8g2.getStrWidth(stopBuf))  / 2;

    if (!editing) {
      // ── Not editing: draw both lines plain ───────────────────────────────
      u8g2.drawStr(startX, START_Y, startBuf);
      u8g2.drawStr(stopX,  STOP_Y,  stopBuf);

    } else {
      // ── Editing: draw each field individually so we can invert the active one
      // Helper lambda — draws one two-digit field, inverted when active & blinkState is on
      auto drawField = [&](uint8_t x, uint8_t baselineY, const char *str, bool isActive) {
        uint8_t fw = u8g2.getStrWidth(str);
        if (isActive && blinkState) {
          u8g2.setDrawColor(1);
          u8g2.drawBox(x, baselineY - FONT_H + 1, fw, FONT_H);
          u8g2.setDrawColor(0);
        } else {
          u8g2.setDrawColor(1);
        }
        u8g2.drawStr(x, baselineY, str);
        u8g2.setDrawColor(1);
      };

      // ── Start line ───────────────────────────────────────────────────────
      const char *startPrefix = "Start: ";
      uint8_t spW = u8g2.getStrWidth(startPrefix);
      uint8_t cwW = u8g2.getStrWidth(":");

      u8g2.drawStr(startX, START_Y, startPrefix);

      uint8_t xSH = startX + spW;
      drawField(xSH, START_Y, sHStr, sm.currentIrrigationTimeField == IRRIGATION_START_HOUR);

      uint8_t xC1 = xSH + u8g2.getStrWidth(sHStr);
      u8g2.drawStr(xC1, START_Y, ":");

      uint8_t xSM = xC1 + cwW;
      drawField(xSM, START_Y, sMStr, sm.currentIrrigationTimeField == IRRIGATION_START_MINUTE);

      // ── Stop line ────────────────────────────────────────────────────────
      const char *stopPrefix = "Stop:  ";
      uint8_t stpW = u8g2.getStrWidth(stopPrefix);

      u8g2.drawStr(stopX, STOP_Y, stopPrefix);

      uint8_t xEH = stopX + stpW;
      drawField(xEH, STOP_Y, eHStr, sm.currentIrrigationTimeField == IRRIGATION_STOP_HOUR);

      uint8_t xC2 = xEH + u8g2.getStrWidth(eHStr);
      u8g2.drawStr(xC2, STOP_Y, ":");

      uint8_t xEM = xC2 + cwW;
      drawField(xEM, STOP_Y, eMStr, sm.currentIrrigationTimeField == IRRIGATION_STOP_MINUTE);
    }

    // ── Count options (nullptr-sentinel) ─────────────────────────────────────
    uint8_t numOptions = 0;
    const char **opts = sm.irrigation_schedules[currentId].schedule_options;
    if (opts != nullptr) {
      while (opts[numOptions] != nullptr) numOptions++;
    }

    uint8_t sel = sm.irrigation_schedules[currentId].selected_option;

    // ── Options bar — at the bottom ───────────────────────────────────────────
    u8g2.setDrawColor(1);

    if (numOptions == 3) {
      const uint8_t CELL_W = 128 / 3;
      for (uint8_t i = 0; i < 3; i++) {
        uint8_t bx = i * CELL_W;
        uint8_t cw = (i == 2) ? (128 - bx) : CELL_W;
        u8g2.drawFrame(bx, OPT_TOP, cw, OPT_H);
        if (i == sel) {
          u8g2.drawBox(bx + 1, OPT_TOP + 1, cw - 2, OPT_H - 2);
          u8g2.setDrawColor(0);
        }
        uint8_t tw = u8g2.getStrWidth(opts[i]);
        u8g2.drawStr(bx + (cw - tw) / 2, OPT_TOP + OPT_H - 1, opts[i]);
        u8g2.setDrawColor(1);
      }

    } else if (numOptions == 2) {
      const uint8_t OPT_W     = 45;
      const uint8_t OPT_RIGHT = 128 - OPT_W;

      u8g2.drawFrame(0, OPT_TOP, OPT_W, OPT_H);
      if (sel == 0) { u8g2.drawBox(1, OPT_TOP + 1, OPT_W - 2, OPT_H - 2); u8g2.setDrawColor(0); }
      u8g2.drawStr((OPT_W - u8g2.getStrWidth(opts[0])) / 2, OPT_TOP + OPT_H - 1, opts[0]);

      u8g2.setDrawColor(1);
      u8g2.drawFrame(OPT_RIGHT, OPT_TOP, OPT_W, OPT_H);
      if (sel == 1) { u8g2.drawBox(OPT_RIGHT + 1, OPT_TOP + 1, OPT_W - 2, OPT_H - 2); u8g2.setDrawColor(0); }
      u8g2.drawStr(OPT_RIGHT + (OPT_W - u8g2.getStrWidth(opts[1])) / 2, OPT_TOP + OPT_H - 1, opts[1]);
      u8g2.setDrawColor(1);

    } else if (numOptions == 1) {
      uint8_t tw = u8g2.getStrWidth(opts[0]);
      u8g2.drawStr((128 - tw) / 2, OPT_TOP + OPT_H - 1, opts[0]);
    }

    u8g2.setDrawColor(1);
  }

  u8g2.sendBuffer();
}