#include "structs.h"
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
float pmbus_convert_linear11_to_float_bitwise(uint16_t value);