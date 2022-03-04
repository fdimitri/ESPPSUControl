extern void stub(byte *buffer);

extern int pmbus_read(int idx, uint8_t command, uint8_t len, byte *buffer);
extern int pmbus_write(int idx, uint8_t command, uint8_t len, byte *buffer);

extern int pmbus_request_by_name(int idx, const char *cmd, byte *buffer);
extern int pmbus_send_by_name(int idx, const char *cmd, uint32_t buffer);
extern int pmbus_send_by_obj(int idx, PMBusCommand *p, uint32_t buffer);

extern PMBusCommand *pmbus_cmd_get_by_name(const char *cmd);
extern PMBusCommand *pmbus_cmd_get_by_register(uint8_t reg);

extern unsigned char pmbus_crc8(unsigned char *d, int n);
extern void pmbus_read_all();

extern float pmbus_convert_linear16_to_float(int16_t value, int16_t vout_mode);
extern float pmbus_convert_linear11_to_float(uint16_t value);
extern float pmbus_convert_linear11_to_float_bitwise(uint16_t value);

extern int pmbus_add_device(TwoWire *wire, uint8_t address);
extern void pmbus_init();
