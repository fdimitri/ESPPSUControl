#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#else
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#endif


#include "structs.h"
#include "main.h"

#include "pmbus_functions.h"
#include "pmbus_commands.h"

#ifndef ARDUINO
#include "linux_arduino_wrapper.h"
#endif

#define PMB_MAX_DEVICES 16

pmbDevice pmb_devices[PMB_MAX_DEVICES];

void pmbus_init() {
  memset((void *) &pmb_devices, 0, sizeof(pmb_devices));
  return;
}

int pmbus_add_device(TwoWire *wire, uint8_t address) {
  unsigned char i;
  for (i = 0; pmb_devices[i].wire != NULL && i < PMB_MAX_DEVICES; i++);
  if (i == PMB_MAX_DEVICES || pmb_devices[i].wire) {
    return(-1);
  }
  pmb_devices[i].wire = wire;
  pmb_devices[i].address = address;
  pmbus_request_by_name(i, "VOUT_MODE", (byte *) &pmb_devices[i].vout_mode);
  if (pmb_devices[i].vout_mode == 0) {
    Serial.printf("pmbus_add_device(): VOUT_MODE was set to 0\n");
  }
  return(i);
}

void pmbus_remove_device(int idx) {
  memset(&pmb_devices[idx], 0, sizeof(pmbDevice));
}

void stub(byte *buffer) {

}

void pmbus_read_all() {
  uint32_t buffer;
  char cbuf[16];
  PMBusCommand *p;
  int idx = 0;

  Serial.printf("---------------- READING ALL ----------------\n");

  for (uint32_t i = 0; pmbus_commands[i].name != NULL; i++) {
    p = &pmbus_commands[i];
    buffer = 0;
    if (p->type & PMBUS_READ) {
      if (p->length > 4) {
        memset(&cbuf, 0, sizeof(cbuf));
        pmbus_request_by_name(idx, p->name, (byte *) &cbuf[0]);
        Serial.printf("(0x%02x)%24s: 0x%x / %16s %s\n", p->reg, p->name, cbuf, string_to_hex(cbuf, 16), cbuf);
        continue;
      }
      if (pmbus_request_by_name(0, p->name, (byte *) &buffer) < 0) {
        Serial.printf("(0x%02x)%24s: 0x%04x (ERROR)\n", p->reg, p->name, buffer);
      }
      else {
        if (p->type & PMBUS_DATATYPE_DIRECT) {
          Serial.printf("(0x%02x)%24s: 0x%04x (DIRECT_NYI)\n", p->reg, p->name, buffer);
        }
        else if (p->type & PMBUS_DATATYPE_INTEGER) {
          Serial.printf("(0x%02x)%24s: 0x%04x\n", p->reg, p->name, buffer);
        }
        else if (p->type & PMBUS_DATATYPE_LINEAR11) {
          Serial.printf("(0x%02x)%24s: %f (L11)\n", p->reg, p->name, pmbus_convert_linear11_to_float(buffer));
        }
        else if (p->type & PMBUS_DATATYPE_LINEAR16) {
          Serial.printf("(0x%02x)%24s: %f (L16)\n", p->reg, p->name, pmbus_convert_linear16_to_float(buffer,0x17));
        }
        else {
          Serial.printf("(0x%02x)%24s: 0x%04x (L11:%f) (L16:%f)\n", p->reg, p->name, buffer, pmbus_convert_linear11_to_float_bitwise(buffer), pmbus_convert_linear16_to_float(buffer,0x17));
        }
      }
    }
    delay(100);
  }
  Serial.printf("---------------- READING DONE ----------------\n");
}

int pmbus_send_by_obj(int idx, PMBusCommand *p, uint32_t buffer) {
  //byte *bptr = (byte *) &buffer + (sizeof(buffer) - p->length - 1);
  byte *bptr;
  bptr = (byte *) &buffer;
  Serial.printf("Sending command:\n(0x%02x)%24s: %d - 0x%04x", p->reg, p->name, p->length, buffer);

  return(pmbus_write(idx, p->reg, p->length, bptr));
}

int pmbus_send_by_name(int idx, const char *cmd, uint32_t buffer) {
  PMBusCommand *p = pmbus_cmd_get_by_name(cmd);

  if (!p) {
    return(-1);
  }
  return(pmbus_send_by_obj(idx, p, buffer));
}


void pmbus_request_float11_by_name(int idx, const char *cmd, float *result) {
  uint16_t buf;
  pmbus_request_by_name(idx, cmd, (byte *) &buf);
  *result = pmbus_convert_linear11_to_float(buf);
  return;
}

void pmbus_request_float16_by_name(int idx, const char *cmd, float *result) {
  uint16_t buf, vout_mode;
  pmbus_request_by_name(idx, "VOUT_MODE", (byte *) &vout_mode);
  pmbus_request_by_name(idx, cmd, (byte *) &buf);
  *result = pmbus_convert_linear16_to_float(buf, vout_mode);
}

int pmbus_request_by_name(int idx, const char *cmd, byte *buffer) {
  PMBusCommand *pcmd = pmbus_cmd_get_by_name(cmd);

  if (!pcmd) {
    return(-1);
  }
  
  return(pmbus_read(idx, pcmd->reg, pcmd->length, buffer));
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
    if (!strcasecmp(cmd, pmbus_commands[i].name)) {
      return(&pmbus_commands[i]);
    }
  }
  return(NULL);
}

pmbDevice *pmbus_get_device(int idx) {
  if (idx < 0 || idx >= PMB_MAX_DEVICES) {
    return(NULL);
  }
  if (pmb_devices[idx].wire != NULL) {
    return(&pmb_devices[idx]);
  }
  return(NULL);
}

int pmbus_read(int idx, uint8_t command, uint8_t len, byte *buffer) {
  pmbDevice *p;
  if (!(p = pmbus_get_device(idx))) {
    return(-1);
  }
  
  p->wire->beginTransmission(p->address);
	p->wire->write(command);
  //Wire.write(crc8(&command, 1));
	p->wire->endTransmission(false);
  // delay(10);
	p->wire->requestFrom(p->address, len, (uint8_t) true);
  uint8_t ptr = len;
  // delay(10);
  if (p->wire->available() != len) {
    Serial.printf("Bytes available on i2c: %d, expected %d\n", p->wire->available(), len);
  }
  while (p->wire->available() && ptr < len) {
     buffer[ptr++] = p->wire->read();
    //  delay(1);
  }

  if (ptr < len) {
    Serial.printf("\n---Error: only received %d of %d bytes", ptr, len);
    return(-ptr);
  }
  return(0);
}

int pmbus_read_string(uint8_t idx, uint8_t command, uint8_t len, byte *buffer) {
  pmbDevice *p;
  if (!(p = pmbus_get_device(idx))) {
    return(-1);
  }

  p->wire->beginTransmission(p->address);
	p->wire->write(command);
//  p->wire->write(pmbus_crc8(&command, 1));
	p->wire->endTransmission(false);

	p->wire->requestFrom(p->address, len, (uint8_t) true);
  uint8_t ptr = 0;

//  delay(5);
  
  if (!p->wire->available()) return(-1);
  uint8_t i2c_length = p->wire->read();

  while (ptr < i2c_length && ptr < len) {
    buffer[ptr++] = p->wire->read();
  }

  return(0);
}

int pmbus_write(int idx, uint8_t command, uint8_t len, byte *buffer) {
  pmbDevice *p;
  if (!(p = pmbus_get_device(idx))) {
    return(-1);
  }

  Serial.printf("\nStarting write to 0x%02x at register 0x%02x: ", p->address, command);
  p->wire->beginTransmission(p->address);
	p->wire->write(command);
  for (uint8_t n = 0; n < len; n++) {
    Serial.printf("0x%02x ", buffer[n]);
    p->wire->write(buffer[n]);
  }
  Serial.printf(".. completed\n");
  //Wire.write(crc8(&command, 1));

	p->wire->endTransmission(true);

  return(0);
}

unsigned char pmbus_crc8(unsigned char *d, unsigned int n) {
  unsigned char pec = 0;
  for (unsigned int i = 0; i < n; i++) {
    pec ^= d[i];
    pec = pec ^ (pec << 1) ^ (pec << 2) ^ ((pec & 0x80) ? 9 : 0) ^ ((pec & 0x40) ? 7 : 0);
  }
  return pec;
}


float pmbus_convert_linear11_to_float(uint16_t value) {
  linear11_t *v = (linear11_t *) &value;
  return((float) v->base * powf(2, v->mantissa));
}

float pmbus_convert_linear16_to_float(int16_t value, int16_t vout_mode) {
  linear16_t *v = (linear16_t *) &vout_mode;
  return((float) value * powf(2, v->mantissa));
}

float pmbus_convert_linear11_to_float_bitwise(uint16_t value) {
    // Move some bits
    int8_t exponent = value >> 11; 
    int16_t mantissa = value & 0x7ff;

    if (exponent > 0x0F) exponent |= 0xE0;      // sign extend exponent
    if (mantissa > 0x03FF) mantissa |= 0xF800;  // sign extend mantissa

    return(mantissa * powf(2, exponent));
 }