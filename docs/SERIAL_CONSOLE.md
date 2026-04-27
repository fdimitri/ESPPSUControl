# Serial Console Reference

Complete reference for all serial console commands available in ESPPSUControl.

## Command Format

Commands are case-insensitive and can use either full names or mnemonics:

```
command [arg1] [arg2] ...
```

## Available Commands

### scan_i2c (s)

Scan the I2C bus for connected devices.

**Usage:**
```
scan_i2c [bus]
s [bus]
```

**Arguments:**
- `bus` (optional) - Bus name (currently only "main" supported)

**Example:**
```
> scan_i2c
I2C device found at address 0x58
I2C device found at address 0x3C
Found 2 devices on this bus
```

### attach_psu (ap)

Attach a PMBus PSU at specified I2C address.

**Usage:**
```
attach_psu <bus> <address>
ap <bus> <address>
```

**Arguments:**
- `bus` - Bus name (use "main")
- `address` - I2C address in hex (e.g., 0x58)

**Example:**
```
> attach_psu main 0x58
Adding device at 0x58
```

**Notes:**
- Valid addresses: 0x01 to 0x7F
- Device is automatically initialized and VOUT_MODE is cached

### read (r)

Read a PMBus register value.

**Usage:**
```
read <command|register>
r <command|register>
```

**Arguments:**
- `command` - PMBus command name (e.g., "READ_VIN")
- `register` - Register address in hex (e.g., 0x88)

**Special:**
- `read all` - Read all supported commands

**Examples:**
```
> read READ_VIN
Found PMBus command information for READ_VIN at 0x88 takes 2 bytes
Device responded with: 1e0c

> read 0x88
Found PMBus command information for READ_VIN at 0x88 takes 2 bytes
Device responded with: 1e0c

> read all
---------------- READING ALL ----------------
(0x88) READ_VIN: 120.5V (L11)
(0x8B) READ_VOUT: 12.1V (L16)
(0x8C) READ_IOUT: 5.2A (L11)
...
---------------- READING DONE ----------------
```

### write (w)

Write value(s) to a PMBus register.

**Usage:**
```
write <command|register> <byte1> [byte2] [byte3] [byte4]
w <command|register> <byte1> [byte2] [byte3] [byte4]
```

**Arguments:**
- `command` - PMBus command name (e.g., "FAN_COMMAND_1")
- `register` - Register address in hex (e.g., 0x3B)
- `byte1-4` - Data bytes in hex

**Examples:**
```
> write FAN_COMMAND_1 0x80 0x0C
Found PMBus command information for FAN_COMMAND_1 at 0x3b takes 2 bytes
Starting write to 0x58 at register 0x3b: 0x80 0x0c .. completed

> write 0x3B 0x80 0x0C
Writing to 3b: 0c80
```

**Notes:**
- Bytes are sent in the order specified
- Maximum 4 bytes per write
- Use hex format (0xNN)

### set_fan (sf)

Set fan speed as a percentage.

**Usage:**
```
set_fan <percent>
sf <percent>
```

**Arguments:**
- `percent` - Fan speed 0-100%

**Example:**
```
> set_fan 75
Set fan speed to 75% (0xc000)
```

**Notes:**
- Converts percentage to LINEAR11 format automatically
- Writes to FAN_COMMAND_1 register

### measurement_mode (mm)

Enable or disable continuous measurement mode.

**Usage:**
```
measurement_mode <on|off>
mm <on|off>
```

**Arguments:**
- `on` - Enable measurement mode
- `off` - Disable measurement mode

**Example:**
```
> measurement_mode on
Measurement mode enabled

> mm off
Measurement mode disabled
```

**Notes:**
- When enabled, stats are collected continuously in loop()
- Statistics include min, max, and average values
- Uses circular buffer for 256 samples per metric

### power_on (pon)

Turn on the PSU output.

**Usage:**
```
power_on
pon
```

**Status:** Not yet implemented

### power_off (poff)

Turn off the PSU output.

**Usage:**
```
power_off
poff
```

**Status:** Not yet implemented

### clear_faults (clr)

Clear all PSU fault conditions.

**Usage:**
```
clear_faults
clr
```

**Status:** Not yet implemented

## Command Responses

### Success
Commands typically respond with confirmation:
```
> set_fan 50
Set fan speed to 50% (0x8000)
```

### Errors

**Command not found:**
```
> foo
Command foo not found.
```

**Wrong number of arguments:**
```
> read
Expected more arguments from you!

> read FOO BAR
Too many arguments! I'm leaving.
```

**Invalid PMBus command:**
```
> read INVALID_CMD
Couldn't find PMBus command by INVALID_CMD
```

**I2C communication error:**
```
Bytes available on i2c: 0, expected 2
---Error: only received 0 of 2 bytes
```

## Tips

### Use Mnemonics
Save typing with short command names:
```
r READ_VIN    # Same as: read READ_VIN
w 0x3B 0x80   # Same as: write 0x3B 0x80
s             # Same as: scan_i2c
```

### Hex Format
Always use `0x` prefix for hex values:
```
✓ read 0x88
✗ read 88
```

### Case Insensitive
Commands are case-insensitive:
```
READ_VIN = read_vin = Read_Vin
```

## Future Commands

Planned additions:
- `stats` - Display collected statistics
- `monitor` - Real-time monitoring display
- `config` - Save/load configuration
- `log` - Data logging control

---

*Last Updated: 2026-04-25*