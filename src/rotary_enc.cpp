#include <Arduino.h>
#include "rotary_enc.h"
#include "pinout.h"

volatile int lastStateCLK = 0;
volatile int lastStateDT = 0;
volatile bool buttonPressed = false;
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
  attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_SW_PIN), swISR, FALLING);
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
  unsigned long currentTime = millis();
  // Debounce: ignore button presses within 50ms
  if (currentTime - lastDebounceTime > 200) {
    // buttonInterruptFlag = true;
    buttonPressed = true;
    lastDebounceTime = currentTime;
  }
}