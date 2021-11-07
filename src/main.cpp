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

struct PMBusCommands {
  const char *name;
  uint8_t code;
  uint8_t length;
  uint8_t type;
  writeMessage write;
  readMessage read;
};

void scan_i2c_bus();

void pmbus_operation_read(byte *buffer) {

}

void pmbus_operation_write(byte *buffer) {

}

void stub(byte *buffer) {

}
PMBusCommands pm_bus_commands[] = {
  { "OPERATION", 0x01, 1, PMBUS_RW, pmbus_operation_read, pmbus_operation_write },
  { "ON_OFF_CONFIG", 0x02, 1, PMBUS_RW, stub, stub },
  { "CLEAR_FAULTS", 0x03, 0, PMBUS_NONE, stub, stub },
  { "WRITE_PROTECT", 0x10, PMBUS_RW, 1, stub, stub },
  { "CAPABILITY", 0x19, PMBUS_READ, 1, stub, stub },
  { "VOUT_MODE", 0x20, PMBUS_READ, 1, stub, stub },
  { "VOUT_COMMAND", 0x21, PMBUS_RW, 2, stub, stub },
  { "VOUT_TRIM", 0x22, PMBUS_RW, 2, stub, stub },
  { "VOUT_CAL_OFFSET", 0x23, PMBUS_RW, 2, stub, stub },
  { "VOUT_MAX", 0x24, PMBUS_READ, 2, stub, stub },
  { "POUT_MAX", 0x31, PMBUS_READ, 2, stub, stub },
  { "FAN_CONFIG", 0x3a, PMBUS_READ, 1, stub, stub },
  { "FAN_COMMAND_1", 0x38, PMBUS_RW, 2, stub, stub },
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
  
  { NULL, 0, 0, 0 }
};

uint8_t pmbus_read(uint8_t i2c_address, uint8_t command, uint8_t len, byte *buffer);
uint8_t pmbus_write(uint8_t i2c_address, uint8_t command, uint8_t len, byte *buffer);
uint8_t calculate_checksum_u16(uint8_t reg, uint16_t msg);
unsigned char crc8(unsigned char *d, int n);

#define CONFIG_IIC_SPEED 200000
void setup() {
  // put your setup code here, to run once:
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

}

void loop() {
  // put your main code here, to run repeatedly:
  //scan_i2c_bus();
  uint32_t buffer;

  buffer = 0;
  pmbus_read(I2C_PSU_ADDRESS, 0x78, 1, (byte *) &buffer);
  Serial.printf("STATUS_BYTE: 0x%x\n", buffer);
  delay(1000);
  buffer = 0;
  pmbus_read(I2C_PSU_ADDRESS, 0x79, 2, (byte *) &buffer);
  Serial.printf("STATUS_WORD: 0x%x\n", buffer);
  delay(1000);
  buffer = 0;
  pmbus_read(I2C_PSU_ADDRESS, 0x7e, 1, (byte *) &buffer);
  Serial.printf("STATUS_CML: 0x%x\n", buffer);  
  delay(1000);

  buffer = 0;
  pmbus_read(I2C_PSU_ADDRESS, 0x10, 1, (byte *) &buffer);
  Serial.printf("write protect: 0x%x\n", buffer);
  delay(5000);
}

uint8_t pmbus_read(uint8_t i2c_address, uint8_t command, uint8_t len, byte *buffer) {
  Wire.beginTransmission(i2c_address);
	size_t n = Wire.write(command);
  Wire.write(crc8(&command, 1));
	Wire.endTransmission(false);

	Wire.requestFrom(i2c_address, len, 1);
  //++len;
  uint8_t ptr = len;
  uint8_t timeout = 64;
  while (!Wire.available() && timeout--) { Serial.printf(".");delay(125);}
  while (ptr > 0) {
    Serial.printf("-- %d bytes available from wire..\n", Wire.available());
    buffer[--ptr] = Wire.read();
    delay(1);
  }
  return(0);
}

uint8_t pmbus_write(uint8_t i2c_address, uint8_t command, uint8_t len, byte *buffer) {
  Wire.beginTransmission(i2c_address);
	Wire.write(command);
  for (uint8_t n = 0; n < len; n++) {
    Wire.write(buffer[n]);
  }
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
  unsigned int i;
  unsigned char pec = 0;
  for (unsigned int i = 0; i < n; i++) {
    pec ^= d[i];
    pec = pec ^ (pec << 1) ^ (pec << 2) ^ ((pec & 0x80) ? 9 : 0) ^ ((pec & 0x40) ? 7 : 0);
  }
  return pec;
}