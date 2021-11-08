#include <Arduino.h>
#include <Wire.h>

#include "structs.h"
#include "main.h"

void stub(byte *buffer);
int pmbus_read(uint8_t i2c_address, uint8_t command, uint8_t len, byte *buffer);
int pmbus_write(uint8_t i2c_address, uint8_t command, uint8_t len, byte *buffer);
int pmbus_request_by_name(const char *cmd, byte *buffer);
int pmbus_send_by_name(const char *cmd, uint32_t buffer);
int pmbus_send_by_obj(const char *cmd, uint32_t buffer);
PMBusCommand *pmbus_cmd_get_by_name(const char *cmd);
PMBusCommand *pmbus_cmd_get_by_register(uint8_t reg);
unsigned char pmbus_crc8(unsigned char *d, int n);
void pmbus_read_all();
float pmbus_convert_linear16_to_float(int16_t value, int16_t vout_mode);
float pmbus_convert_linear11_to_float(uint16_t value);

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
  { "COEFFICIENTS", 0x30, PMBUS_READ, 6, stub, stub },
  { "POUT_MAX", 0x31, PMBUS_READ, 2, stub, stub },
  
  { "VIN_ON", 0x35, PMBUS_READ, 2, stub, stub },
  { "VIN_OFF", 0x36, PMBUS_READ, 2, stub, stub },

  { "FAN_CONFIG", 0x3a, PMBUS_READ, 1, stub, stub },
  { "FAN_COMMAND_1", 0x3b, PMBUS_RW, 2, stub, stub },

  { "VOUT_OV_FAULT_LIMIT", 0x40, PMBUS_RW, 2, stub, stub },
  { "VOUT_OV_FAULT_RESPONSE", 0x41, PMBUS_READ, 1, stub, stub },
  { "VOUT_OV_WARN_LIMIT", 0x42, PMBUS_RW, 2, stub, stub },

  { "VOUT_UV_WARN_LIMIT", 0x43, PMBUS_RW, 2, stub, stub },
  { "VOUT_UV_FAULT_LIMIT", 0x44, PMBUS_RW, 2, stub, stub },
  { "VOUT_UV_FAULT_RESPONSE", 0x45, PMBUS_READ, 1, stub, stub },

  { "IOUT_OC_FAULT_LIMIT", 0x46, PMBUS_READ, 2, stub, stub },
  { "IOUT_OC_FAULT_RESPONSE", 0x47, PMBUS_READ, 1, stub, stub },
  { "IOUT_OC_WARN_LIMIT", 0x4A, PMBUS_READ, 2, stub, stub },
  
  { "OT_FAULT_LIMIT", 0x4F, PMBUS_RW, 2, stub, stub },
  { "OT_FAULT_RESPONSE", 0x50, PMBUS_RW, 1, stub, stub },
  { "OT_WARN_LIMIT", 0x51, PMBUS_RW, 2, stub, stub },

  { "STATUS_BYTE", 0x78, PMBUS_READ, 1, stub, stub },
  { "STATUS_WORD", 0x79, PMBUS_READ, 2, stub, stub },
  { "STATUS_VOUT", 0x7A, PMBUS_READ, 1, stub, stub },
  { "STATUS_IOUT", 0x7B, PMBUS_READ, 1, stub, stub },
  { "STATUS_INPUT", 0x7C, PMBUS_READ, 1, stub, stub },
  { "STATUS_TEMPERATURE", 0x7D, PMBUS_READ, 1, stub, stub },
  { "STATUS_CML", 0x7E, PMBUS_READ, 1, stub, stub },

  { "STATUS_MFR_SPECIFIC", 0x80, PMBUS_READ, 1, stub, stub },
  { "STATUS_FANS_1_2", 0x81, PMBUS_READ, 1, stub, stub },


  { "READ_EIN", 0x86, PMBUS_READ | PMBUS_DATATYPE_DIRECT, 6, stub, stub},
  { "READ_EOUT", 0x87, PMBUS_READ | PMBUS_DATATYPE_DIRECT, 6, stub, stub},
  { "READ_VIN", 0x88, PMBUS_READ, 2, stub, stub},
  { "READ_IIN", 0x89, PMBUS_READ, 2, stub, stub},
  { "READ_VCAP", 0x8A, PMBUS_READ, 2, stub, stub},
  { "READ_VOUT", 0x8B, PMBUS_READ | PMBUS_DATATYPE_DIRECT, 2, stub, stub},
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

void stub(byte *buffer) {

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
        Serial.printf("(0x%02x)%24s: 0x%04x (L11:%f) (L16:%f)\n", p->reg, p->name, buffer, pmbus_convert_linear11_to_float(buffer), pmbus_convert_linear16_to_float(buffer,0x17));
      }
    }
    delay(100);
  }
  Serial.printf("---------------- READING DONE ----------------\n");
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

unsigned char pmbus_crc8(unsigned char *d, int n) {
  unsigned char pec = 0;
  for (unsigned int i = 0; i < n; i++) {
    pec ^= d[i];
    pec = pec ^ (pec << 1) ^ (pec << 2) ^ ((pec & 0x80) ? 9 : 0) ^ ((pec & 0x40) ? 7 : 0);
  }
  return pec;
}


float pmbus_convert_linear11_to_float(uint16_t value) {
  // I could bang bits around like crazy and play the 2s complement XOR game, but if the compiler wants to do it for me..
  linear11_t *v = (linear11_t *) &value;
  return((float) v->base * powf(2, v->mantissa));
}

float pmbus_convert_linear16_to_float(int16_t value, int16_t vout_mode) {
  // I could bang bits around like crazy and play the 2s complement XOR game, but if the compiler wants to do it for me..
  // Maybe I should write a bunch of bitwise operators just for the fun of it?
  linear16_t *v = (linear16_t *) &vout_mode;
  return((float) value * powf(2, v->mantissa));
}
