#ifndef PINOUT_H
#define PINOUT_H

// I2C communication pins
#define I2C_SDA_PIN PB7
#define I2C_SCL_PIN PB6

// SPI communication pins for GLCD 
#define PIN_SCK  PA5
#define PIN_MOSI PA7
#define PIN_CS   PA4
#define PIN_RST  PB11

// Screen backlight pin
#define BACKLIGHT_PIN PA6  // Change this to another non-spi pin and leave it unused.

// Relay control pins
#define CR02_CTRL PB8 
#define CR03_CTRL PC13 
#define CR04_CTRL PB9 

// Rotary encoder pins
#define ROTARY_ENCODER_CLK_PIN PB10
#define ROTARY_ENCODER_DT_PIN  PA0
#define ROTARY_ENCODER_SW_PIN  PB1

// Analog multiplexer pins.
#define TMUX_1208_EN_PIN PB13
#define TMUX_1208_A0_PIN PB12
#define TMUX_1208_A1_PIN PB15
#define TMUX_1208_A2_PIN PB14

// Motor driver pins
#define CR01_EN       PA11
#define CR05_EN       PA13
#define CR01_1A_CTRL  PA8
#define CR01_2A_CTRL  PA14
#define CR05_3A_CTRL  PA15
#define CR05_4A_CTRL  PA12

// CR05 status feedback pin
#define CR05_STATUS_CLOSED PB4
#define CR05_STATUS_OPEN   PB5

// Analog pins
#define VALVE_V_MEASURE_PIN PA2
#define VIN_MEASURE_PIN     PA3

#endif