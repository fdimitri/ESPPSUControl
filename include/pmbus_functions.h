#include "structs.h"

pmbDevice *pmbus_get_device(int idx);

void stub(byte *buffer);

int pmbus_read(int idx, uint8_t command, uint8_t len, byte *buffer);
int pmbus_write(int idx, uint8_t command, uint8_t len, byte *buffer);

int pmbus_request_by_name(int idx, const char *cmd, byte *buffer);
int pmbus_send_by_name(int idx, const char *cmd, uint32_t buffer);
int pmbus_send_by_obj(int idx, PMBusCommand *p, uint32_t buffer);

PMBusCommand *pmbus_cmd_get_by_name(const char *cmd);
PMBusCommand *pmbus_cmd_get_by_register(uint8_t reg);

unsigned char pmbus_crc8(unsigned char *d, unsigned int n);
void pmbus_read_all();

float pmbus_convert_linear16_to_float(int16_t value, int16_t vout_mode);
float pmbus_convert_linear11_to_float(uint16_t value);
float pmbus_convert_linear11_to_float_bitwise(uint16_t value);

int pmbus_add_device(TwoWire *wire, uint8_t address);
void pmbus_init();
