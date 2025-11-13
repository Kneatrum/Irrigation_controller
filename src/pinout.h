#ifndef PINOUT_H
#define PINOUT_H

// I2C communication pins
#define I2C_SDA_PIN 2
#define I2C_SCL_PIN 3

// SPI communication pins for GLCD 
#define PIN_SCK 13
#define PIN_MOSI 11
#define PIN_CS 10
#define PIN_RST 5

// Screen backlight pin
#define BACKLIGHT_PIN 6

// Relay control pins
#define POWER_RELAY_PIN 4 
#define POLARITY_RELAY_1_PIN 8
#define POLARITY_RELAY_2_PIN 9

// Rotary encoder pins
#define ROTARY_ENCODER_CLK_PIN 0
#define ROTARY_ENCODER_DT_PIN 1
#define ROTARY_ENCODER_SW_PIN 7

#endif