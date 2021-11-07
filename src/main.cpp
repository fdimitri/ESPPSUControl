#include <Arduino.h>
#include <Wire.h>

#define PMBUS_READ  0x01
#define PMBUS_WRITE 0x02
#define PMBUS_NONE  0x04
#define PMBUS_RW    (PMBUS_READ | PMBUS_WRITE)

#define PMBUS_IS_READONLY(x) (x == PMBUS_READ)
#define PMBUS_IS_READWRITE(x) (x & PMBUS_READ && x & PMBUS_WRITE)
#define PMBUS_IS_WRITEONLY(x) (x == PMBUS_WRITE)

#define I2C_PSU_ADDRESS 0x58

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

void scan_i2c_bus();

void stub(byte *buffer) {

}

PMBusCommand pmbus_commands[] = {
  { "OPERATION", 0x01, PMBUS_WRITE, 1, stub, stub },
  { "ON_OFF_CONFIG", 0x02, PMBUS_RW, 1, stub, stub },
  { "CLEAR_FAULTS", 0x03, PMBUS_NONE, 0, stub, stub },
  { "WRITE_PROTECT", 0x10, PMBUS_RW, 1, stub, stub },
  { "CAPABILITY", 0x19, PMBUS_READ, 1, stub, stub },
  { "VOUT_MODE", 0x20, PMBUS_READ, 1, stub, stub },
  { "VOUT_COMMAND", 0x21, PMBUS_RW, 2, stub, stub },
  { "VOUT_TRIM", 0x22, PMBUS_RW, 2, stub, stub },
  { "VOUT_CAL_OFFSET", 0x23, PMBUS_RW, 2, stub, stub },
  { "VOUT_MAX", 0x24, PMBUS_READ, 2, stub, stub },
  { "POUT_MAX", 0x31, PMBUS_READ, 2, stub, stub },
  
  { "VIN_ON", 0x35, PMBUS_READ, 2, stub, stub },
  { "VIN_OFF", 0x36, PMBUS_READ, 2, stub, stub },

  { "FAN_CONFIG", 0x3a, PMBUS_READ, 1, stub, stub },
  { "FAN_COMMAND_1", 0x3b, PMBUS_RW, 2, stub, stub },

  { "STATUS_BYTE", 0x78, PMBUS_READ, 1, stub, stub },
  { "STATUS_WORD", 0x79, PMBUS_READ, 2, stub, stub },
  { "STATUS_VOUT", 0x7A, PMBUS_READ, 1, stub, stub },
  { "STATUS_IOUT", 0x7B, PMBUS_READ, 1, stub, stub },
  { "STATUS_INPUT", 0x7C, PMBUS_READ, 1, stub, stub },
  { "STATUS_TEMPERATURE", 0x7D, PMBUS_READ, 1, stub, stub },
  { "STATUS_CML", 0x7E, PMBUS_READ, 1, stub, stub },
  { "STATUS_MFR_SPECIFIC", 0x80, PMBUS_READ, 1, stub, stub },
  { "STATUS_FANS_1_2", 0x81, PMBUS_READ, 1, stub, stub },


  { "READ_EIN", 0x86, PMBUS_READ, 6, stub, stub},
  { "READ_EOUT", 0x87, PMBUS_READ, 6, stub, stub},
  { "READ_VIN", 0x88, PMBUS_READ, 2, stub, stub},
  { "READ_IIN", 0x89, PMBUS_READ, 2, stub, stub},
  { "READ_VCAP", 0x8A, PMBUS_READ, 2, stub, stub},
  { "READ_VOUT", 0x8B, PMBUS_READ, 2, stub, stub},
  { "READ_IOUT", 0x8C, PMBUS_READ, 2, stub, stub},
  { "READ_TEMPERATURE_1", 0x8D, PMBUS_READ, 2, stub, stub},
  { "READ_TEMPERATURE_2", 0x8E, PMBUS_READ, 2, stub, stub},
  { "READ_TEMPERATURE_3", 0x8F, PMBUS_READ, 2, stub, stub},
  { "READ_FAN_SPEED_1", 0x90, PMBUS_READ, 2, stub, stub},
  { "READ_POUT", 0x91, PMBUS_READ, 2, stub, stub},
  { "READ_PIN", 0x92, PMBUS_READ, 2, stub, stub},
  { "PMBUS_REVISION", 0x98, PMBUS_READ, 1, stub, stub },
  { "MFR_MODEL", 0x9A, PMBUS_READ, 12, stub, stub },
  { NULL, 0, 0, 0 }
};

SerialCommand serial_commands[] = {
  { "read", "r", NULL },
  { "write", "w", NULL },
  { "power_off", "poff", NULL },
  { "power_on", "pon", NULL },
  { NULL, NULL, NULL }
};

#define CONFIG_IIC_SPEED 100000
char serial_command_buffer[256];
uint8_t serial_command_buffer_ptr = 0;

int pmbus_read(uint8_t i2c_address, uint8_t command, uint8_t len, byte *buffer);
int pmbus_write(uint8_t i2c_address, uint8_t command, uint8_t len, byte *buffer);
int pmbus_request_by_name(const char *cmd, byte *buffer);
int pmbus_send_by_name(const char *cmd, uint32_t buffer);
int pmbus_send_by_obj(const char *cmd, uint32_t buffer);
void parse_message(char *msg);
void serial_read();

unsigned char crc8(unsigned char *d, int n);

PMBusCommand *pmbus_cmd_get_by_name(const char *cmd);
PMBusCommand *pmbus_cmd_get_by_register(uint8_t reg);
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
        pmbus_send_by_name("FAN_COMMAND_1", 0x08);
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

void pmbus_read_all() {
  uint32_t buffer;
  char cbuf[16];
  PMBusCommand *p;
  Serial.printf("---------------- READING ALL ----------------\n");
  for (uint32_t i = 0; pmbus_commands[i].name != NULL; i++) {
    p = &pmbus_commands[i];
    buffer = 0;
    if (p->type & PMBUS_READ) {
      if (p->length > 4) {
        memset(&cbuf, 0, sizeof(cbuf));
        pmbus_request_by_name(p->name, (byte *) &cbuf[0]);
        Serial.printf("(0x%02x)%24s: 0x%x / %16s %s\n", p->reg, p->name, cbuf, string_to_hex(cbuf, 16), cbuf);
        continue;
      }
      if (pmbus_request_by_name(p->name, (byte *) &buffer) < 0) {
        Serial.printf("(0x%02x)%24s: 0x%04x (ERROR)\n", p->reg, p->name, buffer);
      }
      else {
        Serial.printf("(0x%02x)%24s: 0x%04x (%f)\n", p->reg, p->name, buffer, buffer / 512.0);
      }
    }
    delay(100);
  }
  Serial.printf("---------------- READING DONE ----------------\n");
}

char *string_to_hex(char *string, int maxn) {
  static char buf[256];
  memset(&buf, 0 , sizeof(buf));
  for (unsigned int i = 0; i < maxn; i++) {
    sprintf(&buf[0] + strlen(buf), "0x%02x ", string[i]);
  }
  return(&buf[0]);
}

int pmbus_send_by_obj(PMBusCommand *p, uint32_t buffer) {
  //byte *bptr = (byte *) &buffer + (sizeof(buffer) - p->length - 1);
  byte *bptr;
  bptr = (byte *) &buffer;
  Serial.printf("Sending command:\n(0x%02x)%24s: %d - 0x%04x", p->reg, p->name, p->length, buffer);

  return(pmbus_write(I2C_PSU_ADDRESS, p->reg, p->length, bptr));
}

int pmbus_send_by_name(const char *cmd, uint32_t buffer) {
  PMBusCommand *p = pmbus_cmd_get_by_name(cmd);

  if (!p) {
    return(-1);
  }
  return(pmbus_send_by_obj(p, buffer));
}

int pmbus_request_by_name(const char *cmd, byte *buffer) {
  PMBusCommand *pcmd = pmbus_cmd_get_by_name(cmd);

  if (!pcmd) {
    return(-1);
  }
  
  return(pmbus_read(I2C_PSU_ADDRESS, pcmd->reg, pcmd->length, buffer));
}

PMBusCommand *pmbus_cmd_get_by_register(uint8_t reg) {
  for (unsigned int i = 0; pmbus_commands[i].name != NULL; i++) {
    if (pmbus_commands[i].reg == reg) {
      return(&pmbus_commands[i]);
    }
  }
  return(NULL);
}

PMBusCommand *pmbus_cmd_get_by_name(const char *cmd) {
  for (unsigned int i = 0; pmbus_commands[i].name != NULL; i++) {
    if (!strcmp(cmd, pmbus_commands[i].name)) {
      return(&pmbus_commands[i]);
    }
  }
  return(NULL);
}

int pmbus_read(uint8_t i2c_address, uint8_t command, uint8_t len, byte *buffer) {
  Wire.beginTransmission(i2c_address);
	Wire.write(command);
  //Wire.write(crc8(&command, 1));
	Wire.endTransmission(false);

	Wire.requestFrom(i2c_address, len, (uint8_t) true);
  //++len;
  uint8_t ptr = len;
  uint8_t timeout = 64;
  ptr = 0;
  // delay(5);
  // Serial.printf("%d bytes", Wire.available());
  while (!Wire.available() && timeout--) { Serial.printf(".");delay(125);}
  // while (ptr > 0) {
  //   Serial.printf("-- %d bytes available from wire..\n", Wire.available());
  //   buffer[--ptr] = Wire.read();
  //   delay(1);
  // }
  delay(5);
  timeout = 0;
  if (Wire.available()) {
    while (ptr < len && timeout < 8) {
      if (Wire.available()) {
         buffer[ptr++] = Wire.read();
      } else {
        delay(1);
        timeout++;
      }
    }
  }
  if (ptr < len) {
    Serial.printf("\n---Error: only received %d of %d bytes", ptr, len);
    return(-1);
  }
  return(0);
}

int pmbus_read_string(uint8_t i2c_address, uint8_t command, uint8_t len, byte *buffer) {
  Wire.beginTransmission(i2c_address);
	Wire.write(command);
  //Wire.write(crc8(&command, 1));
	Wire.endTransmission(false);

	Wire.requestFrom(i2c_address, len, (uint8_t) true);
  uint8_t ptr = 0;
  uint8_t timeout = 0;

  delay(5);
  
  if (!Wire.available()) return(-1);
  uint8_t i2c_length = Wire.read();

  while (ptr < i2c_length && ptr < len) {
    buffer[ptr++] = Wire.read();
  }

  return(0);
}

int pmbus_write(uint8_t i2c_address, uint8_t command, uint8_t len, byte *buffer) {
  Serial.printf("Starting write to 0x%02x at register 0x%02x: ", i2c_address, command);
  Wire.beginTransmission(i2c_address);
	Wire.write(command);
  for (uint8_t n = 0; n < len; n++) {
    Serial.printf("0x%02x ", buffer[n]);
    Wire.write(buffer[n]);
  }
  Serial.printf(".. completed\n");
  //Wire.write(crc8(&command, 1));

	Wire.endTransmission(true);

  return(0);
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

unsigned char crc8(unsigned char *d, int n) {
  unsigned char pec = 0;
  for (unsigned int i = 0; i < n; i++) {
    pec ^= d[i];
    pec = pec ^ (pec << 1) ^ (pec << 2) ^ ((pec & 0x80) ? 9 : 0) ^ ((pec & 0x40) ? 7 : 0);
  }
  return pec;
}