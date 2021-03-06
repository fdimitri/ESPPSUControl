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

#ifndef ARDUINO
#include "linux_arduino_wrapper.h"
#endif
void scan_i2c_bus();


#define CONFIG_IIC_SPEED 100000
char serial_command_buffer[256];
uint8_t serial_command_buffer_ptr = 0;


void parse_write(int argc, char *argv[]);
void parse_read(int argc, char *argv[]);
void parse_scan_i2c(int argc, char *argv[]);
void parse_set_fan(int argc, char *argv[]);
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
  { NULL, NULL, NULL }
};


void setup() {
  delay(2500);
  Serial.begin(115200);
  Wire.setClock(CONFIG_IIC_SPEED);

  Wire.begin(21, 22, CONFIG_IIC_SPEED);
  Wire.setClock(CONFIG_IIC_SPEED);
  uint8_t cmdBytes = 0x80;
  pmbus_write(I2C_PSU_ADDRESS, 0x01, 1, &cmdBytes);

  cmdBytes = 0x00;
  pmbus_write(I2C_PSU_ADDRESS, 0x10, 1, &cmdBytes);

  cmdBytes = 0x18;
  pmbus_write(I2C_PSU_ADDRESS, 0x02, 1, &cmdBytes);

  cmdBytes = 0xFF;
  pmbus_write(I2C_PSU_ADDRESS, 0x03, 0, &cmdBytes);

  cmdBytes = 0x80;
  pmbus_write(I2C_PSU_ADDRESS, 0x01, 1, &cmdBytes);

  pmbus_send_by_name("WRITE_PROTECT", 0x0);
  pmbus_send_by_name("OPERATION", 0x80);
  pmbus_send_by_name("ON_OFF_CONFIG", 0xA);
  //pmbus_send_by_name("VOUT_COMMAND", 0x1801);
  pmbus_send_by_name("VOUT_COMMAND", 0x0118);
}

void loop() {
  // put your main code here, to run repeatedly:
  //scan_i2c_bus();
  char sbuf;
  PMBusCommand *p;
  if (0 && Serial.available() > 0 && Serial.read(&sbuf, 1)) {
    switch (sbuf) {
      case 'r':
        pmbus_read_all();
        break;
      case 'f':
        pmbus_send_by_name("FAN_COMMAND_1", 0x0a);
        pmbus_send_by_name("FAN_SPEED_TEST", 0x0a);
        break;
      case 'F':
        pmbus_send_by_name("FAN_COMMAND_1", 0xFFFFFFFF);
        pmbus_send_by_name("FAN_SPEED_TEST", 0xFFFFFFFF);

        break;
      case 's':
      case 'S':
        scan_i2c_bus();
        break;
      case 'p':
        pmbus_send_by_name("OPERATION", 0x0);
        break;
      case 'P':
        pmbus_send_by_name("OPERATION", 0x80);
        break;

      case 'z':
        pmbus_send_by_name("FAN_COMMAND_1", 0x12);
        break;
      case 'Z':
        pmbus_send_by_name("FAN_COMMAND_1", 0x26);
        break;
      case 'x':
        pmbus_send_by_name("FAN_COMMAND_1", 0x62);
        break;
      case 'X':
        pmbus_send_by_name("FAN_COMMAND_1", 0x64); // Can't set fans past 0x64/100.. they couldn't normalize the range?!
        break;
      case 'c':
        pmbus_send_by_name("FAN_COMMAND_1", 0x66);
        break;
      case 'C':
        pmbus_send_by_name("FAN_COMMAND_1", 0xeb1f);
        break;
    }
  }
  else {
    serial_read();
  }
}

void parse_message(char *omsg) {
  char *argv[32];
  unsigned int argc = 0;
  char *msg, *msgstart;

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
  for (unsigned int i = 0; i < argc - 2; i++) {
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
  for (unsigned int i = 0; i < maxn; i++) {
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

