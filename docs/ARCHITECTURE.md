# ESPPSUControl Architecture

This document describes the software architecture and design decisions of ESPPSUControl.

## Overview

ESPPSUControl is a firmware for ESP32/ESP8266 microcontrollers that provides monitoring and control capabilities for PMBus-compatible power supply units via I2C communication.

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        User Interface                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │ Serial CLI   │  │ OLED Display │  │ (Future: Web UI) │  │
│  └──────┬───────┘  └──────┬───────┘  └──────────────────┘  │
└─────────┼──────────────────┼──────────────────────────────┘
          │                  │
┌─────────┼──────────────────┼──────────────────────────────┐
│         │    Application Layer (main.cpp)                  │
│  ┌──────▼───────┐  ┌──────▼───────┐  ┌─────────────────┐  │
│  │   Command    │  │   Display    │  │  Measurements   │  │
│  │   Parser     │  │   Manager    │  │   & Stats       │  │
│  └──────┬───────┘  └──────────────┘  └────────┬────────┘  │
└─────────┼──────────────────────────────────────┼──────────┘
          │                                       │
┌─────────┼───────────────────────────────────────┼──────────┐
│         │         PMBus Layer (pmbus.cpp)       │          │
│  ┌──────▼───────────────────────────────────────▼───────┐  │
│  │              PMBus Protocol Handler                   │  │
│  │  • Command Registry  • Data Conversion  • CRC-8      │  │
│  └──────┬────────────────────────────────────────────────┘  │
└─────────┼──────────────────────────────────────────────────┘
          │
┌─────────▼──────────────────────────────────────────────────┐
│              Hardware Abstraction Layer                     │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────┐  │
│  │   I2C/Wire   │  │    Serial    │  │   SSD1306 OLED  │  │
│  └──────┬───────┘  └──────────────┘  └─────────────────┘  │
└─────────┼──────────────────────────────────────────────────┘
          │
┌─────────▼──────────────────────────────────────────────────┐
│                    Hardware Layer                           │
│         ESP32/ESP8266  ←→  PMBus PSU  ←→  OLED             │
└─────────────────────────────────────────────────────────────┘
```

## Module Descriptions

### Main Application (`src/main.cpp`)

**Responsibilities:**
- Arduino setup() and loop() lifecycle
- Serial command parsing and routing
- User interaction handling
- I2C bus scanning

**Key Functions:**
- `parse_message()`: Tokenizes and routes commands
- `serial_read()`: Handles serial input buffering
- `scan_i2c_bus()`: Discovers I2C devices
- Command handlers: `parse_read()`, `parse_write()`, etc.

**Design Decisions:**
- Uses function pointer table for command dispatch
- Stack-based command buffer (256 bytes)
- Supports both full command names and mnemonics

### PMBus Layer (`src/pmbus.cpp`)

**Responsibilities:**
- PMBus protocol implementation
- Device management (up to 16 devices)
- Data format conversions (LINEAR11, LINEAR16)
- Command registry and lookup

**Key Data Structures:**
```cpp
struct pmbDevice {
  TwoWire *wire;      // I2C bus reference
  uint8_t address;    // Device I2C address
  uint16_t vout_mode; // Cached VOUT_MODE for LINEAR16
};
```

**Key Functions:**
- `pmbus_read()` / `pmbus_write()`: Low-level I2C operations
- `pmbus_convert_linear11_to_float()`: LINEAR11 → float
- `pmbus_convert_linear16_to_float()`: LINEAR16 → float
- `pmbus_cmd_get_by_name()`: Command lookup

**Design Decisions:**
- Device array for multi-PSU support
- Cached VOUT_MODE to avoid repeated reads
- Both struct-based and bitwise conversion methods

### Measurements (`src/measurements.cpp`)

**Responsibilities:**
- Statistical data collection
- Running averages, min/max tracking
- Circular buffer for samples

**Key Data Structures:**
```cpp
struct statsItem {
  float peak;                    // Maximum value seen
  float min;                     // Minimum value seen
  float samples[STATS_NSAMPLES]; // Circular buffer (256 samples)
  uint8_t hptr, tptr;           // Head/tail pointers
};
```

**Design Decisions:**
- 256-sample circular buffer (uint8_t naturally wraps)
- Separate tracking for voltage, current, power, efficiency
- Min initialized to INFINITY for proper first-sample handling

### Display (`src/display.cpp`)

**Responsibilities:**
- OLED display initialization and management
- Text rendering via Adafruit libraries
- Printf-style formatted output

**Key Functions:**
- `oled_init()`: Initialize SSD1306 display
- `oled_printf()`: Formatted text output
- `oled_msg()`: Raw text output

## Data Flow

### Command Execution Flow
```
User Input → Serial Buffer → parse_message() → Command Lookup
    → Handler Function → PMBus Operation → I2C Transaction
    → Response Processing → Serial Output
```

### Measurement Flow
```
Loop() → stats_collect() → PMBus Read Commands
    → Data Conversion → stats_update_item()
    → Circular Buffer Update → Min/Max/Avg Calculation
```

## PMBus Data Formats

### LINEAR11 Format
```
Bits 15-11: 5-bit two's complement exponent (N)
Bits 10-0:  11-bit two's complement mantissa (Y)
Value = Y × 2^N
```

### LINEAR16 Format
```
VOUT_MODE register bits 4-0: 5-bit two's complement exponent (N)
Data: 16-bit two's complement mantissa (Y)
Value = Y × 2^N
```

## Memory Management

### Static Allocation
- Command buffer: 256 bytes (stack)
- PMBus device array: 16 devices
- Stats buffers: 256 samples × 7 metrics × 4 bytes = ~7KB

### Dynamic Allocation
- Minimal use (only in command parsing for tokenization)
- Always freed before function return

## Error Handling

### I2C Communication
- Timeout detection via `Wire.available()`
- Partial read detection
- Error codes returned to caller

### Command Parsing
- Argument count validation
- Bounds checking on buffers
- Graceful error messages

## Thread Safety

**Note:** Current implementation is single-threaded (Arduino loop model).

Future considerations for RTOS:
- Mutex protection for device array
- Queue for command processing
- Separate task for measurements

## Performance Considerations

### I2C Speed
- Default: 100 kHz (safe for most PSUs)
- Configurable via `Wire.setClock()`
- Trade-off: speed vs. reliability

### Measurement Rate
- Controlled by loop() execution
- No fixed sampling rate
- Consider adding delay for consistent timing

### Display Updates
- Blocking calls to `display.display()`
- Consider buffering for smoother updates

## Future Enhancements

### Planned Features
1. **Web Interface**: ESP32 WiFi for remote monitoring
2. **Data Logging**: SD card or SPIFFS storage
3. **MQTT Integration**: IoT platform connectivity
4. **Multi-PSU Support**: Parallel monitoring of multiple units

### Architecture Changes Needed
1. **Task Separation**: RTOS tasks for measurements, display, network
2. **Event System**: Pub/sub for decoupled modules
3. **Configuration Storage**: EEPROM/Flash for settings persistence

## Testing Strategy

### Unit Testing
- Conversion functions (LINEAR11/LINEAR16)
- Command parsing logic
- Statistics calculations

### Integration Testing
- I2C communication with actual PSU
- Command end-to-end flow
- Display rendering

### Hardware Testing
- Multiple PSU models
- Different I2C speeds
- Long-term stability

## Build Configurations

### ESP32 (`esp32doit-devkit-v1`)
- Full feature set
- WiFi capable
- More memory

### ESP8266 (`nodemcuv2`)
- Limited memory
- Basic features only
- WiFi capable

### Linux (`linux_x86_64`)
- Development/testing
- No hardware dependencies
- Useful for algorithm testing, serial console interface test (UX), new command parsing

## Dependencies

### Core Libraries
- Arduino Framework
- Wire (I2C)
- Serial

### External Libraries
- Adafruit_SSD1306: OLED display driver
- Adafruit_GFX: Graphics primitives
- Adafruit_BusIO: I2C abstraction

## Code Metrics

- Total Lines: ~1500
- Main Application: ~400 lines
- PMBus Layer: ~270 lines
- Measurements: ~90 lines
- Display: ~60 lines

---

*Last Updated: 2026-04-25*