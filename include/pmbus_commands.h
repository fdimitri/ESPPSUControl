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
