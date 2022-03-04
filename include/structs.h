#ifndef PROJECT_STRUCTS_H
#define PROJECT_STRUCTS_H
#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#else
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
typedef unsigned char byte;
#endif

#define PMBUS_READ  0x01
#define PMBUS_WRITE 0x02
#define PMBUS_NONE  0x04
#define PMBUS_RW    (PMBUS_READ | PMBUS_WRITE)

#define PMBUS_DATATYPE_DIRECT   0x08
#define PMBUS_DATATYPE_LINEAR11 0x10
#define PMBUS_DATATYPE_LINEAR16 0x20

#define PMBUS_IS_READONLY(x) (x == PMBUS_READ)
#define PMBUS_IS_READWRITE(x) (x & PMBUS_READ && x & PMBUS_WRITE)
#define PMBUS_IS_WRITEONLY(x) (x == PMBUS_WRITE)

#define I2C_PSU_ADDRESS 0x58

#define SSD1306_128_32

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define CONFIG_IIC_SPEED 100000U

typedef void (*writeMessage)(byte *buffer);
typedef void (*readMessage)(byte *buffer);
typedef void (*parseCallback)(int argc, char *argv[]);

struct PMBusCommand {
  const char *name;
  uint8_t reg;
  uint8_t type;
  uint8_t length;
  writeMessage write;
  readMessage read;
};

struct SerialCommand {
  const char *command;
  const char *mnemonic;
  parseCallback callback;
};

struct linear11_t {
  int16_t base : 11;
  int16_t mantissa : 5;
};

struct linear16_t {
  int16_t mantissa : 5;
  int16_t ignored : 11;
};

struct pmbDevice {
  TwoWire *wire;
  uint8_t address;
};

#endif