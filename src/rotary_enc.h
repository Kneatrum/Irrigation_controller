#ifndef ROTARY_ENC_H
#define ROTARY_ENC_H

#define STEPS_PER_DETENT 4 // Number of steps per detent for the rotary encoder

#define DEBOUNCE_TIME 50
#define LONG_PRESS_TIME 1000

extern volatile int lastStateCLK;   // Last state of the CLK pin
extern volatile int lastStateDT;    // Last state of the DT pin
extern volatile bool buttonPressed; // State of the encoder button
extern volatile bool longPressFlag;
extern volatile bool shortPressFlag;
extern volatile unsigned long buttonPressTime;
extern volatile unsigned long buttonDuration;  // Store actual duration
extern volatile bool buttonInterruptFlag; 
extern volatile unsigned long lastDebounceTime; // For button debounce

extern volatile bool clockwiseTurn;    // Flag for clockwise turn
extern volatile bool counterClockwiseTurn; // Flag for counter-clockwise turn
extern volatile bool encoderMoved;      // Flag to indicate if encoder has moved

// Initializes the rotary encoder
void initializeRotaryEncoder();

// Attach interrupt handlers for rotary encoder
void attachRotaryEncoderInterrupts(void (*clkISR)(), void (*dtISR)(), void (*swISR)());

void updateEncoder();
void handleButton();

#endif