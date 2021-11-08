#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#endif

#include "structs.h"
#include "main.h"

#include "pmbus_functions.h"
#include "pmbus_commands.h"


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
