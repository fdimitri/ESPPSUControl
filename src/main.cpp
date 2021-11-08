#include <Arduino.h>
#include <Wire.h>

#include "structs.h"
#include "pmbus.h"

void scan_i2c_bus();

SerialCommand serial_commands[] = {
  { "read", "r", NULL },
  { "write", "w", NULL },
  { "power_off", "poff", NULL },
  { "power_on", "pon", NULL },
  { "set_fan", "sf", NULL },
  { "clear_faults", "clr", NULL },
  { NULL, NULL, NULL }
};

#define CONFIG_IIC_SPEED 100000
char serial_command_buffer[256];
uint8_t serial_command_buffer_ptr = 0;


void parse_message(char *msg);
void serial_read();

unsigned char crc8(unsigned char *d, int n);

char *string_to_hex(char *string, int maxn);

void pmbus_read_all();

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
  if (Serial.available() > 0 && Serial.read(&sbuf, 1)) {
    switch (sbuf) {
      case 'r':
        pmbus_read_all();
        break;
      case 'f':
        pmbus_send_by_name("FAN_COMMAND_1", 0xa0);
        break;
      case 'F':
        pmbus_send_by_name("FAN_COMMAND_1", 0xFFFFFFFF);
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
    if (!strcmp(serial_commands[i].command, argv[0])) {
      serial_commands[i].callback(argc + 1, argv);
      free(msgstart);
      return;
    }
  }
  Serial.printf("Command %s not found.\n", argv[0]);

  free(msgstart);
  return;
}


void serial_read() {
  while (Serial.available() > 0) {
    uint8_t c = Serial.read();
    Serial.print((char) c);
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



void scan_i2c_bus() {
  byte error, address; 
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++ ) {

    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (!error) {
      Serial.print("I2C device found at address 0x");
      Serial.print(address, HEX);
      Serial.println("  !");
      nDevices++;
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("done\n");
  }
}

