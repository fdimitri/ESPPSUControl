#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#else
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#endif

#include <stdio.h>

#include "structs.h"
#include "pmbus.h"
#include "display.h"

#ifndef ARDUINO
#include "linux_arduino_wrapper.h"
#endif
void scan_i2c_bus(TwoWire *wire);

char serial_command_buffer[256];
uint8_t serial_command_buffer_ptr = 0;


void parse_write(int argc, char *argv[]);
void parse_read(int argc, char *argv[]);
void parse_scan_i2c(int argc, char *argv[]);
void parse_set_fan(int argc, char *argv[]);
void parse_measurement_mode(int argc, char *argv[]);
void parse_message(char *msg);
void serial_read();
void parse_attach_psu(int argc, char *argv[]);

char *string_to_hex(char *string, int maxn);
char *hexstring_strip(const char *h, char *n);
long hexstring_to_long(const char *h);

void pmbus_read_all();

SerialCommand serial_commands[] = {
  { "read", "r", parse_read },
  { "write", "w", parse_write },
  { "scan_i2c", "s", parse_scan_i2c },
  { "power_off", "poff", NULL },
  { "power_on", "pon", NULL },
  { "set_fan", "sf", NULL },
  { "clear_faults", "clr", NULL },
  { "measurement_mode", "mm", parse_measurement_mode },
  { "attach_psu", "ap", parse_attach_psu },
  { NULL, NULL, NULL }
};

#define STATS_NSAMPLES 256

struct statsItem {
  float peak;
  float min;
  float samples[STATS_NSAMPLES];
  uint8_t hptr, tptr;
};

struct statsRecorder {
  statsItem outVolts, inVolts;
  statsItem outWatts, inWatts;
  statsItem outAmps, inAmps;
  statsItem efficiency;
};


void stats_update_item(statsItem *cItem, float f);
void stats_collect(int pfd);
float stats_get_average(statsItem *cItem);
void stats_initialize();

statsRecorder stats;

uint32_t runFlags = 0x0;

#define RUNFLAG_MODE_MEASUREMENT 0x01

void stats_collect(int pfd) {
  uint16_t buf, vomode;
  struct statsItem *cItem;
  float f;

  pmbus_request_by_name(pfd, "VOUT_MODE", (byte *) &buf);
  vomode = buf;

  cItem = &stats.inAmps;
  pmbus_request_by_name(pfd, "READ_IIN", (byte *) &buf);
  f = pmbus_convert_linear11_to_float(buf);
  stats_update_item(cItem, f);

  cItem = &stats.inVolts;
  pmbus_request_by_name(pfd, "READ_VIN", (byte *) &buf);
  f = pmbus_convert_linear11_to_float(buf);
  stats_update_item(cItem, f);

  cItem = &stats.outVolts;
  pmbus_request_by_name(pfd, "READ_VOUT", (byte *) &buf);
  f = pmbus_convert_linear16_to_float(buf, vomode);
  stats_update_item(cItem, f);

  cItem = &stats.outAmps;
  pmbus_request_by_name(pfd, "READ_IOUT", (byte *) &buf);
  f = pmbus_convert_linear11_to_float(buf);
  stats_update_item(cItem, f);

  return;
}

void stats_update_item(statsItem *cItem, float f) {
  if (cItem->peak < f) cItem->peak = f;
  if (cItem->min > f) cItem->min = f;
  cItem->samples[cItem->tptr++] = f;
  return;  
}

float stats_get_average(statsItem *cItem) {
  float t;
  for (int i = 0; i < STATS_NSAMPLES; i++) t += cItem->samples[i];
  return(t / STATS_NSAMPLES);
}

void stats_initialize() {
  memset((void *) &stats, 0, sizeof(stats));

}

void setup() {
  delay(2500);
  Serial.begin(115200);
  Wire.setClock(CONFIG_IIC_SPEED);
  
#ifdef ESP32
  Wire.begin(21, 22, CONFIG_IIC_SPEED);
  Serial.printf("ESP32 is defined\n");
#elif ESP8266
  Wire.begin(D6, D5);  
  Serial.printf("ESP8266 is defined, using SDA/pin %d and SCL/pin %d\n", D6, D5);
#endif

  Wire.setClock(100000);

  Serial.println("Initializing display..");
  
  if (oled_init() < 0) {
    Serial.println("oled_init() failed!");
  }

  pmbus_init();
  int pfd = pmbus_add_device(&Wire, 0x6c);
  oled_printf("\nGot PMBus Device ID: %d\n", pfd);
 
}

void read_stats() {
  uint32_t buffer;
  pmbus_request_by_name(0, "READ_POUT", (byte *) &buffer);
}

void loop() {
  while (Serial.available()) {
    serial_read();
  }
  if (runFlags & RUNFLAG_MODE_MEASUREMENT) {
    read_stats();
  }
}

void parse_message(char *omsg) {
  char *argv[32];
  unsigned int argc = 0;
  char *msg, *msgstart;

  oled_printf("%s", omsg);

  msgstart = msg = (char *) malloc(strlen(omsg) + 1);
  memcpy(msg, omsg, strlen(omsg));
  msg[strlen(omsg)] = '\0';

  argv[0] = strtok(msg, " ");
  msg = strtok(NULL, " ");

  while (msg != NULL && argc < 32) {
    argv[++argc] = msg;
    msg = strtok(NULL, " ");
  }
  
  for (unsigned int i = 0; serial_commands[i].command != NULL; i++) {
    if (!strcasecmp(serial_commands[i].command, argv[0]) || !strcmp(serial_commands[i].mnemonic, argv[0])) {
      if (serial_commands[i].callback) serial_commands[i].callback(argc + 1, argv);
      else Serial.printf("Command %s at index %d has no callback..?\n", argv[0], i);
      free(msgstart);
      return;
    }
  }
  Serial.printf("Command %s not found.\n", argv[0]);

  free(msgstart);
  return;
}

void parse_scan_i2c(int argc, char *argv[]) {
  Serial.printf("parse_scan_i2c(%d, char *[])\n", argc);
  if (argc == 2) {
    if (!strcasecmp("main", argv[1])) {
      scan_i2c_bus(&Wire);
    }
    return;
  }
  scan_i2c_bus(&Wire);
  return;
}

void parse_create_i2c(int argc, char *argv[]) {

}

void parse_write(int argc, char *argv[]) {
  uint8_t cmdregister = 0;
  uint32_t buffer = 0;
  PMBusCommand *p;
  if (argc < 3) {
    Serial.printf("Expected more arguments from you!\n");
    return;
  }

  if (argc > 6) {
    Serial.printf("Too many arguments! I'm leaving.\n");
    return;
  }
  
  // Reverse Byte Order?
  // for (unsigned int i = 0; i < argc - 2; i++) {
  //   buffer |= strtol(argv[i+2], NULL, 16) << ((argc - 3 - i) * 8);
  // }

  // Forward byte order
  for (int i = 0; i < argc - 2; i++) {
    buffer |= strtol(argv[i+2], NULL, 16) << (i * 8);
  }
  
  if (!strncmp("0x", argv[1], 2)) {
    cmdregister = strtol(argv[1], NULL, 16);
    p = pmbus_cmd_get_by_register(cmdregister);
  }

  else {
    p = pmbus_cmd_get_by_name(argv[1]);
    cmdregister = p->reg;
  }

  if (!p) {
    Serial.printf("Couldn't find PMBus command by %s\n", argv[1]);
    if (!cmdregister) return;
    else {
      pmbus_write(0, cmdregister, argc - 2, (byte *) buffer);
      return;
    }
  }

  Serial.printf("Found PMBus command information for %s at 0x%2x takes %d bytes\n", p->name, p->reg, p->length);
  
  pmbus_send_by_obj(0, p, buffer);
  
  Serial.printf("Writing to %02x: %04x\n", cmdregister, buffer);
}


void parse_read(int argc, char *argv[]) {
  uint8_t cmdregister = 0;
  PMBusCommand *p = NULL;
  uint64_t buffer = 0;
  if (argc < 2) {
    Serial.printf("Expected more arguments from you!\n");
    return;
  }

  if (argc > 2) {
    Serial.printf("Too many arguments! I'm leaving.\n");
    return;
  }

  if (!strcasecmp("all", argv[1])) {
    pmbus_read_all();
    return;
  }

  if (!strncmp("0x", argv[1], 2)) {
    cmdregister = strtol(argv[1], NULL, 16);
    p = pmbus_cmd_get_by_register(cmdregister);
  }

  else {
    p = pmbus_cmd_get_by_name(argv[1]);
  }

  if (!p) {
    Serial.printf("Couldn't find PMBus command by %s\n", argv[1]);
    return;
  }

  Serial.printf("Found PMBus command information for %s at 0x%2x takes %d bytes\n", p->name, p->reg, p->length);
  pmbus_request_by_name(0, p->name, (byte *) &buffer);
  Serial.printf("Device responded with: %llx\n", buffer);
  
}

void parse_set_fan(int argc, char *argv[]) {

}

void parse_measurement_mode(int argc, char *argv[]) {

}

void draw_measurement_mode() {

}

void serial_read() {
  while (Serial.available() > 0) {
    uint8_t c = Serial.read();
    Serial.printf("%c", (char) c);
    if (serial_command_buffer_ptr > sizeof(serial_command_buffer) - 1) {
      Serial.printf("Serial input exceeded length! Discarding input.\n");
      serial_command_buffer_ptr = 0;
    }
    switch (c) {
      case '\r':
      case '\n':
        if (strlen(serial_command_buffer)) {
          parse_message((char *) &serial_command_buffer);
          serial_command_buffer_ptr = 0;
        }
        memset((void *) &serial_command_buffer, 0, sizeof(serial_command_buffer));
        break;
      case '\b':
        serial_command_buffer_ptr--;
        serial_command_buffer[serial_command_buffer_ptr] = 0;
        break;
      default:
        serial_command_buffer[serial_command_buffer_ptr++] = c;  
    }
  }
}



char *string_to_hex(char *string, int maxn) {
  static char buf[256];
  memset(&buf, 0 , sizeof(buf));
  for (int i = 0; i < maxn; i++) {
    sprintf(&buf[0] + strlen(buf), "0x%02x ", string[i]);
  }
  return(&buf[0]);
}

//0x0f 0x1a

long hexstring_to_long(const char *h) {
  char *n = (char *) malloc(strlen(h) + 1);
  char *stripped = hexstring_strip(h, n);
  printf("stripped string: %s\n", stripped);
  free(n);
  return(strtol(stripped, NULL, 16));
}

char *hexstring_strip(const char *h, char *n) {
  memset(n, 0, strlen(h) + 1);
  char *n_start = n;

  while (*h) {
    if (*h == '0' && *(h+1) == 'x') {
      h++;
      continue;
    }
    if ((*h >= '0' && *h <= '9') || (*h >= 'a' && *h <= 'f') || (*h >= 'A' && *h <= 'F')) {
      *n++ = *h;
    }
    h++;
  }
  return (n_start);
}


void print_i2c_bus_info(uint16_t devices[8]) {
  Serial.printf("    \t");
  for (uint8_t c = 0; c < 0x10; c++) {
    Serial.printf("%x\t", c);
  }
  Serial.printf("\n");
  for (uint8_t c = 0; c < 128; c++) Serial.print("-");
  Serial.println("");
  for (uint8_t highN = 0; highN < 0x8; highN++) {
    Serial.printf("0x%x\t", highN);
    for (uint8_t lowN = 0; lowN < 0x10; lowN++) {
      Serial.printf("%d\t", devices[highN] & (1<<lowN) ? 1 : 0);
    }
    Serial.printf("\n");
  }
}

void scan_i2c_bus(TwoWire *wire) {
//  wire = &Wire;
  byte error;
  uint8_t address; 
  uint16_t devices[8];
  uint8_t numDevices = 0;

  for (uint8_t highN = 0; highN < 0x8; highN++) {
    devices[highN] = 0;
    for (uint8_t lowN = 0; lowN < 0x10; lowN++) {
      address = ((highN << 4) + lowN);
      if (!address) {       // I2C address 0x0 is special
        continue;
      }
      wire->beginTransmission(address);
      error = wire->endTransmission();
      if (!error) {
        Serial.printf("I2C device found at address 0x%2x\n", address);   
        devices[highN] |= 1 << lowN;
        numDevices++;
      }
      else if (error == 4) {
        Serial.printf("Unknown error at address 0x%2x\n", address);
      }
    }
  }
  oled_printf("\nSCAN!");
  Serial.printf("Found %d devices on this bus\n", numDevices);
  // for (uint8_t c = 0; c < 0x8; c++) {
  //   printf("%016x\n", devices[c]);
  // }
  print_i2c_bus_info(devices);
}

void parse_attach_psu(int argc, char *argv[]) {
  // ap BUS_NAME HEX_ADDR
  if (argc != 3) {
    Serial.printf("attach_psu: Unsupported number of arguments\n");
    return;
  }
  if (strcasecmp("main", argv[1])) {
    Serial.printf("attach_psu: NYI multiple i2c busses, use 'main' only\n");
    return;
  }
  uint8_t addr = strtol(argv[2], NULL, 16);
  if (addr >= 0x01 && addr <= 0x7f) {
    Serial.printf("Adding device at 0x%x", addr);
    pmbus_add_device(&Wire, addr);
    return;
  }
  Serial.printf("attach_psu: Invalid or unrecognized i2c address - %s interpreted as 0x%x\n", argv[2], addr);
  return;
}