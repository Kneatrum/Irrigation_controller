#include <Arduino.h>
#include "rotary_enc.h"
#include "pinout.h"

volatile int lastStateCLK = 0;
volatile int lastStateDT = 0;
volatile bool buttonPressed = false;
volatile bool longPressFlag = false;
volatile bool shortPressFlag = false;
volatile unsigned long buttonPressTime = 0;
volatile unsigned long buttonDuration;
volatile bool buttonInterruptFlag = false; 
volatile unsigned long lastDebounceTime = 0;
volatile bool clockwiseTurn = false;
volatile bool counterClockwiseTurn = false;
volatile bool encoderMoved = false;

void initializeRotaryEncoder() {
  noInterrupts();
  pinMode(ROTARY_ENCODER_CLK_PIN, INPUT);
  pinMode(ROTARY_ENCODER_DT_PIN, INPUT);   
  pinMode(ROTARY_ENCODER_SW_PIN, INPUT_PULLUP);

  lastStateCLK = digitalRead(ROTARY_ENCODER_CLK_PIN);
  lastStateDT = digitalRead(ROTARY_ENCODER_DT_PIN);

  encoderMoved = false;
  buttonPressed = false;
  clockwiseTurn = false;
  counterClockwiseTurn = false;
  lastDebounceTime = 0;
  
  interrupts();
}

void attachRotaryEncoderInterrupts(void (*clkISR)(), void (*dtISR)(), void (*swISR)()) {
  attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_CLK_PIN), clkISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_DT_PIN), dtISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_SW_PIN), swISR, CHANGE);
}

void updateEncoder() {
  int currentCLK = digitalRead(ROTARY_ENCODER_CLK_PIN);

  if(currentCLK == HIGH && lastStateCLK == LOW){
    // Rising edge detected on CLK
    int currentDT = digitalRead(ROTARY_ENCODER_DT_PIN);

    if(currentDT == LOW){
      // Clockwise rotation
      clockwiseTurn = true;
      counterClockwiseTurn = false;
    } else {
      // Counter-clockwise rotation
      counterClockwiseTurn = true;
      clockwiseTurn = false;
    }

    encoderMoved = true;
  }

  lastStateCLK = currentCLK;

}

void handleButton() {
  static unsigned long lastInterruptTime = 0;
  static bool lastButtonState = HIGH;
  unsigned long interruptTime = millis();
  
  // Read current state
  int buttonState = digitalRead(ROTARY_ENCODER_SW_PIN);
  
  // Only process if state actually changed AND debounce time passed
  if (buttonState != lastButtonState && 
      (interruptTime - lastInterruptTime > DEBOUNCE_TIME)) {
    
    if (buttonState == LOW) {
      // Button pressed
      buttonPressTime = interruptTime;
      buttonPressed = true;
    } 
    else {
      // Button released
      if (buttonPressed) {  // Make sure we had a valid press
        buttonDuration = interruptTime - buttonPressTime;
        
        if (buttonDuration >= LONG_PRESS_TIME) {
          longPressFlag = true;
        } else if (buttonDuration >= 50) {  // Minimum 50ms for valid press
          shortPressFlag = true;
        }
        
        buttonPressed = false;
      }
    }
    
    lastButtonState = buttonState;
    lastInterruptTime = interruptTime;
  }
}