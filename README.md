# ESPPSUControl

A powerful ESP32/ESP8266-based firmware for monitoring and controlling PMBus-compatible power supply units (PSUs) with real-time display capabilities.

## 🌟 Features

- **PMBus Protocol Support**: Full implementation of PMBus commands for PSU communication
- **Multi-Platform**: Supports ESP32, ESP8266 (NodeMCU), and Linux x86_64 for development/testing
- **Real-time Monitoring**: Track voltage, current, power output, and other PSU metrics
- **OLED Display**: Visual feedback via SSD1306 OLED display (128x64)
- **Serial Command Interface**: Control and query PSU via serial commands
- **I2C Bus Scanning**: Automatic device discovery on the I2C bus
- **Statistics Collection**: Measurement mode for continuous data logging
- **Fan Control**: Adjust PSU fan speeds
- **Fault Management**: Clear and monitor PSU fault conditions

## 📋 Requirements

### Hardware
- ESP32 DevKit v1 or ESP8266 NodeMCU v2
- PMBus-compatible power supply unit
- SSD1306 OLED display (128x64, I2C)
- I2C connection to PSU (SDA/SCL)

### Software
- [PlatformIO](https://platformio.org/) (recommended) or Arduino IDE
- Python 3.9+ (for PlatformIO)

## 🚀 Getting Started

### Installation

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd ESPPSUControl
   ```

2. **Install PlatformIO** (if not already installed)
   ```bash
   pip install -U platformio
   ```

3. **Build the project**
   
   For ESP32:
   ```bash
   platformio run -e esp32doit-devkit-v1
   ```
   
   For ESP8266:
   ```bash
   platformio run -e nodemcuv2
   ```
   
   For Linux (testing):
   ```bash
   platformio run -e linux_x86_64
   ```

4. **Upload to device**
   ```bash
   platformio run -e esp32doit-devkit-v1 --target upload
   ```

### Hardware Connections

#### ESP32
- **SDA**: GPIO 21
- **SCL**: GPIO 22
- **I2C Speed**: 100 kHz

#### ESP8266 (NodeMCU)
- **SDA**: D6
- **SCL**: D5
- **I2C Speed**: 100 kHz

Connect your SSD1306 OLED display and PMBus PSU to these I2C pins.

## 💻 Usage

### Serial Commands

Connect to the device via serial monitor at **115200 baud**. Available commands:

| Command | Shorthand | Description |
|---------|-----------|-------------|
| `read` | `r` | Read PMBus register |
| `write` | `w` | Write to PMBus register |
| `scan_i2c` | `s` | Scan I2C bus for devices |
| `power_off` | `poff` | Turn off PSU |
| `power_on` | `pon` | Turn on PSU |
| `set_fan` | `sf` | Set fan speed |
| `clear_faults` | `clr` | Clear PSU faults |
| `measurement_mode` | `mm` | Enable continuous measurement mode |
| `attach_psu` | `ap` | Attach PSU device |

### Example Usage

```bash
# Scan for I2C devices
> scan_i2c

# Attach PSU at address 0x69
> attach_psu 0x69

# Read power output
> read READ_POUT

# Enable measurement mode
> measurement_mode on

# Clear faults
> clear_faults
```

## 🏗️ Project Structure

```
ESPPSUControl/
├── include/              # Header files
│   ├── display.h        # OLED display functions
│   ├── pmbus.h          # PMBus protocol implementation
│   ├── measurements.h   # Statistics and data collection
│   ├── structs.h        # Data structures
│   └── pmbus_commands.h # PMBus command definitions
├── src/                 # Source files
│   ├── main.cpp         # Main application logic
│   ├── display.cpp      # Display implementation
│   ├── pmbus.cpp        # PMBus communication
│   └── measurements.cpp # Measurement functions
├── lib/                 # Project libraries
├── test/                # Test files
├── platformio.ini       # PlatformIO configuration
└── README.md           # This file
```

## 🔧 Configuration

### I2C Speed
Default I2C speed is set to 100 kHz. Modify in `src/main.cpp`:
```cpp
Wire.setClock(100000);  // 100 kHz
```

### PMBus Device Address
Default PSU address is `0x69`. Change during initialization:
```cpp
int pfd = pmbus_add_device(&Wire, 0x69);
```

## 📊 PMBus Features

- **Linear Data Format Conversion**: Supports LINEAR11 and LINEAR16 formats
- **CRC-8 Validation**: Ensures data integrity
- **Command-by-Name**: Use human-readable command names
- **Multiple Device Support**: Manage multiple PSUs on the same bus

## 🧪 Development

### Building for Linux
For development and testing without hardware:
```bash
platformio run -e linux_x86_64
```

### CI/CD
The project includes GitLab CI configuration (`.gitlab-ci.yml`) for automated builds.

## 📦 Dependencies

- [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306) v2.5.1+
- [Adafruit BusIO](https://github.com/adafruit/Adafruit_BusIO) v1.11.2+
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)

Dependencies are automatically managed by PlatformIO.

## 📝 License

This project is licensed under the GNU General Public License v2.0 - see the [LICENSE](LICENSE) file for details.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## ⚠️ Safety Notice

**WARNING**: This firmware interfaces with power supply units that can deliver high voltages and currents. Always:
- Disconnect power before making connections
- Use proper insulation and safety equipment
- Verify all connections before powering on
- Follow your PSU manufacturer's safety guidelines

## 🔍 Troubleshooting

### Device Not Found
- Verify I2C connections (SDA/SCL)
- Check PSU address with `scan_i2c` command
- Ensure PSU is powered and PMBus-enabled

### Display Not Working
- Verify OLED display I2C address (usually 0x3C or 0x3D)
- Check display power connections
- Ensure correct I2C pins are used

### Communication Errors
- Reduce I2C speed if experiencing errors
- Check for proper pull-up resistors on I2C lines
- Verify cable length (keep I2C cables short)

## 📚 Resources

- [PMBus Specification](https://pmbus.org/)
- [ESP32 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [PlatformIO Documentation](https://docs.platformio.org/)

---

**Made with ❤️ for the maker community**