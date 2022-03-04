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
void scan_i2c_bus();

char serial_command_buffer[256];
uint8_t serial_command_buffer_ptr = 0;


void parse_write(int argc, char *argv[]);
void parse_read(int argc, char *argv[]);
void parse_scan_i2c(int argc, char *argv[]);
void parse_set_fan(int argc, char *argv[]);
void parse_measurement_mode(int argc, char *argv[]);
void parse_message(char *msg);
void serial_read();

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
  statsItem inVolts;
  statsItem outVolts;
  statsItem inWatts;
};

statsRecorder stats;

uint32_t runFlags = 0x0;

#define RUNFLAG_MODE_MEASUREMENT 0x01



void setup() {
  delay(2500);
  Serial.begin(115200);
  Wire.setClock(CONFIG_IIC_SPEED);
  
#ifdef ESP32
  Wire.begin(21, 22, CONFIG_IIC_SPEED);
  Serial.printf("ESP32 is defined\n");
#elif ESP8266
  Wire.begin(D3, D1);  
  Serial.printf("ESP8266 is defined, using SDA/pin %d and SCL/pin %d\n", D3, D1);
#endif

  Wire.setClock(100000);

  Serial.println("Initializing display..");
  
  if (oled_init() < 0) {
    Serial.println("oled_init() failed!");
  }

}

void loop() {
  while (Serial.available()) {
    serial_read();
  }
  if (runFlags & RUNFLAG_MODE_MEASUREMENT) {
    read_stats();
  }
}

void read_stats() {
  uint32_t buffer;
  pmbus_request_by_name("READ_POUT", (byte *) &buffer);
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
  scan_i2c_bus();
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
  }

  if (!p) {
    Serial.printf("Couldn't find PMBus command by %s\n", argv[1]);
    if (!cmdregister) return;
    else {
      pmbus_write(I2C_PSU_ADDRESS, cmdregister, argc - 2, (byte *) buffer);
      return;
    }
  }

  Serial.printf("Found PMBus command information for %s at 0x%2x takes %d bytes\n", p->name, p->reg, p->length);

  pmbus_send_by_obj(p, buffer);
  
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
  pmbus_request_by_name(p->name, (byte *) &buffer);
  Serial.printf("Device responded with: %lx\n", buffer);
  
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
  const char *h_start = h;
  char *n_start = n;

  while (*h) {
    if (*h == '0' && *(h+1) == 'x') {
      h++;h++;
      continue;
    }
    if ((*h >= '0' && *h <= '9') || (*h >= 'a' && *h <= 'f') || (*h >= 'A' && *h <= 'F')) {
      *n++ = *h;
    }
    h++;
  }
  return (n_start);
}

void scan_i2c_bus() {
  byte error, address; 

  Serial.println("Scanning...");

  for (address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (!error) {
      Serial.printf("I2C device found at address 0x%2x\n", address);   
    }
    else if (error == 4) {
      Serial.printf("Unknown error at address 0x%2x\n", address);
    }
  }
}

