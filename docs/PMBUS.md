# PMBus Implementation Guide

This document explains how the PMBus implementation works in ESPPSUControl and how to extend it.

## Quick Start

### Reading a Value

```cpp
uint16_t buffer;
pmbus_request_by_name(0, "READ_VOUT", (byte *) &buffer);
float voltage = pmbus_convert_linear16_to_float(buffer, vout_mode);
Serial.printf("Output voltage: %.2fV\n", voltage);
```

### Writing a Value

```cpp
uint16_t fan_speed = 0x8000;  // 50% in LINEAR11 format
pmbus_send_by_name(0, "FAN_COMMAND_1", fan_speed);
```

## Command Registry

All PMBus commands are defined in `include/pmbus_commands.h` as a table:

```cpp
PMBusCommand pmbus_commands[] = {
  { "READ_VIN", 0x88, PMBUS_READ | PMBUS_DATATYPE_LINEAR11, 2, stub, stub },
  { "READ_VOUT", 0x8B, PMBUS_READ | PMBUS_DATATYPE_LINEAR16, 2, stub, stub },
  // ... more commands ...
  { NULL, 0, 0, 0 }  // Terminator
};
```

### Command Structure

```cpp
struct PMBusCommand {
  const char *name;      // Command name (e.g., "READ_VIN")
  uint8_t reg;          // PMBus register address (e.g., 0x88)
  uint8_t type;         // Flags: read/write + data type
  uint8_t length;       // Number of bytes to read/write
  writeMessage write;   // Write callback (currently stub)
  readMessage read;     // Read callback (currently stub)
};
```

### Type Flags

**Access Flags:**
- `PMBUS_READ` (0x01) - Command supports reading
- `PMBUS_WRITE` (0x02) - Command supports writing
- `PMBUS_RW` - Shorthand for read+write
- `PMBUS_NONE` (0x04) - No data (e.g., CLEAR_FAULTS)

**Data Type Flags:**
- `PMBUS_DATATYPE_LINEAR11` (0x10) - 5-bit exp + 11-bit mantissa
- `PMBUS_DATATYPE_LINEAR16` (0x20) - Uses VOUT_MODE exponent
- `PMBUS_DATATYPE_INTEGER` (0x40) - Raw integer value
- `PMBUS_DATATYPE_DIRECT` (0x08) - Direct format (not implemented)

## Adding a New Command

### Step 1: Add to Command Table

Edit `include/pmbus_commands.h`:

```cpp
PMBusCommand pmbus_commands[] = {
  // ... existing commands ...
  
  // Add your new command
  { "READ_DUTY_CYCLE", 0x94, PMBUS_READ | PMBUS_DATATYPE_LINEAR11, 2, stub, stub },
  
  { NULL, 0, 0, 0 }  // Keep terminator at end
};
```

### Step 2: Use It

The command is now available via `pmbus_request_by_name()`:

```cpp
uint16_t buffer;
pmbus_request_by_name(0, "READ_DUTY_CYCLE", (byte *) &buffer);
float duty = pmbus_convert_linear11_to_float(buffer);
Serial.printf("Duty cycle: %.1f%%\n", duty);
```

### Step 3: (Optional) Add Serial Command

If you want users to access it via serial, add a handler in `src/main.cpp`:

```cpp
void parse_read_duty(int argc, char *argv[]) {
  uint16_t buffer;
  pmbus_request_by_name(0, "READ_DUTY_CYCLE", (byte *) &buffer);
  float duty = pmbus_convert_linear11_to_float(buffer);
  Serial.printf("Duty cycle: %.1f%%\n", duty);
}

// Add to serial_commands table:
SerialCommand serial_commands[] = {
  // ... existing commands ...
  { "duty", "d", parse_read_duty },
  { NULL, NULL, NULL }
};
```

## Core Functions

### pmbus_request_by_name()

**Purpose:** Read data from a PMBus command by name

**Signature:**
```cpp
int pmbus_request_by_name(int idx, const char *cmd, byte *buffer)
```

**Parameters:**
- `idx` - Device index (0-15)
- `cmd` - Command name (e.g., "READ_VIN")
- `buffer` - Pointer to receive data

**Returns:** 0 on success, negative on error

**How it works:**
1. Looks up command in `pmbus_commands[]` table by name
2. Extracts register address and length
3. Calls `pmbus_read()` to perform I2C transaction
4. Returns raw bytes in buffer

**Example:**
```cpp
uint16_t raw_voltage;
if (pmbus_request_by_name(0, "READ_VOUT", (byte *) &raw_voltage) == 0) {
  // Success - convert raw value
  float volts = pmbus_convert_linear16_to_float(raw_voltage, vout_mode);
}
```

### pmbus_send_by_name()

**Purpose:** Write data to a PMBus command by name

**Signature:**
```cpp
int pmbus_send_by_name(int idx, const char *cmd, uint32_t buffer)
```

**Parameters:**
- `idx` - Device index (0-15)
- `cmd` - Command name (e.g., "FAN_COMMAND_1")
- `buffer` - Data to write (up to 4 bytes)

**Returns:** 0 on success, negative on error

**Example:**
```cpp
uint16_t fan_value = 0xC000;  // 75% speed
pmbus_send_by_name(0, "FAN_COMMAND_1", fan_value);
```

### pmbus_read() / pmbus_write()

**Purpose:** Low-level I2C read/write operations

**Signatures:**
```cpp
int pmbus_read(int idx, uint8_t command, uint8_t len, byte *buffer)
int pmbus_write(int idx, uint8_t command, uint8_t len, byte *buffer)
```

**Note:** Usually you don't call these directly - use `pmbus_request_by_name()` instead.

## Data Conversion

### LINEAR11 Format

**When to use:** Most PMBus readings (voltage, current, power, temperature)

**Conversion:**
```cpp
uint16_t raw;
pmbus_request_by_name(0, "READ_IIN", (byte *) &raw);
float amps = pmbus_convert_linear11_to_float(raw);
```

**Format:** 5-bit exponent (bits 15-11) + 11-bit mantissa (bits 10-0)

**Formula:** `value = mantissa × 2^exponent`

### LINEAR16 Format

**When to use:** Output voltage readings (VOUT)

**Conversion:**
```cpp
uint16_t vout_mode, raw;
pmbus_request_by_name(0, "VOUT_MODE", (byte *) &vout_mode);
pmbus_request_by_name(0, "READ_VOUT", (byte *) &raw);
float volts = pmbus_convert_linear16_to_float(raw, vout_mode);
```

**Format:** 16-bit mantissa + separate 5-bit exponent from VOUT_MODE register

**Formula:** `value = mantissa × 2^exponent`

**Note:** VOUT_MODE is cached in the device structure after `pmbus_add_device()`

## Device Management

### Adding a Device

```cpp
int psu_id = pmbus_add_device(&Wire, 0x58);
if (psu_id < 0) {
  Serial.println("Failed to add device");
}
```

**What it does:**
1. Finds free slot in device array (max 16 devices)
2. Stores I2C bus reference and address
3. Reads and caches VOUT_MODE register
4. Returns device index for future operations

### Multiple Devices

```cpp
int psu1 = pmbus_add_device(&Wire, 0x58);
int psu2 = pmbus_add_device(&Wire, 0x59);

// Read from different devices
pmbus_request_by_name(psu1, "READ_VOUT", buffer1);
pmbus_request_by_name(psu2, "READ_VOUT", buffer2);
```

## Common Patterns

### Reading Multiple Values

```cpp
void read_all_stats(int device_id) {
  uint16_t buf;
  
  pmbus_request_by_name(device_id, "READ_VIN", (byte *) &buf);
  float vin = pmbus_convert_linear11_to_float(buf);
  
  pmbus_request_by_name(device_id, "READ_IIN", (byte *) &buf);
  float iin = pmbus_convert_linear11_to_float(buf);
  
  pmbus_request_by_name(device_id, "READ_PIN", (byte *) &buf);
  float pin = pmbus_convert_linear11_to_float(buf);
  
  Serial.printf("Input: %.1fV, %.2fA, %.1fW\n", vin, iin, pin);
}
```

### Checking Status

```cpp
uint16_t status;
pmbus_request_by_name(0, "STATUS_WORD", (byte *) &status);

if (status & 0x0080) Serial.println("VOUT overvoltage!");
if (status & 0x0020) Serial.println("IOUT overcurrent!");
if (status & 0x0010) Serial.println("VIN undervoltage!");
if (status & 0x0004) Serial.println("Temperature fault!");
```

### Clearing Faults

```cpp
pmbus_send_by_name(0, "CLEAR_FAULTS", 0);
```

## Troubleshooting

### Command Not Found

```
Couldn't find PMBus command by READ_FOO
```

**Solution:** Check spelling in `pmbus_commands[]` table. Command names are case-insensitive.

### Wrong Data Type

If you get garbage values, check the data type flag:
- LINEAR11 commands need `pmbus_convert_linear11_to_float()`
- LINEAR16 commands need `pmbus_convert_linear16_to_float()` with VOUT_MODE
- INTEGER commands are raw values (no conversion)

### I2C Errors

```
Bytes available on i2c: 0, expected 2
```

**Possible causes:**
- Wrong I2C address
- Device not responding
- I2C speed too high
- Wiring issues

**Debug steps:**
1. Run `scan_i2c` to verify device address
2. Try lower I2C speed: `Wire.setClock(100000)`
3. Check SDA/SCL connections
4. Verify PSU is powered on

## References

- [PMBus Specification v1.3.1](https://pmbus.org/specification/)
- [SMBus Specification v2.0](http://smbus.org/specs/)

---

*Last Updated: 2026-04-25*