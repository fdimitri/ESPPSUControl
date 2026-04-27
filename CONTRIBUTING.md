# Contributing to ESPPSUControl

Thank you for your interest in contributing to ESPPSUControl! This document provides guidelines and information for contributors.

## Code of Conduct

- Be respectful and constructive
- Focus on what is best for the project and community
- Show empathy towards other community members

## Getting Started

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Test thoroughly on actual hardware if possible
5. Commit your changes with clear, descriptive messages
6. Push to your branch
7. Open a Pull Request

## Development Setup

### Prerequisites
- PlatformIO Core or PlatformIO IDE
- Python 3.9+
- ESP32 or ESP8266 development board (for hardware testing)
- PMBus-compatible PSU (for full testing)

### Building
```bash
# For ESP32
platformio run -e esp32doit-devkit-v1

# For ESP8266
platformio run -e nodemcuv2

# For Linux (testing without hardware)
platformio run -e linux_x86_64
```

### Testing
```bash
# Upload to device
platformio run -e esp32doit-devkit-v1 --target upload

# Monitor serial output
platformio device monitor --baud 115200
```

## Coding Standards

### C/C++ Style
- Use 2-space indentation
- Opening braces on same line for functions
- Clear, descriptive variable names
- Comment complex logic
- Keep functions focused and concise

### Example
```cpp
void parse_command(int argc, char *argv[]) {
  if (argc < 2) {
    Serial.printf("Usage: command <arg>\n");
    return;
  }
  // Process command
  process_arg(argv[1]);
}
```

## Commit Messages

### Format
```
<type>: <subject>

<body>

<footer>
```

### Types
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

### Example
```
fix: Correct LINEAR11 bit field order in structs.h

The mantissa and exponent fields were reversed, causing incorrect
conversion of PMBus LINEAR11 values. This fix swaps the field order
to match the PMBus specification.

Fixes #42
```

## Pull Request Process

1. **Update Documentation**: Ensure README.md and relevant docs are updated
2. **Test Your Changes**: Verify on hardware when possible
3. **Describe Your Changes**: Provide clear description in PR
4. **Link Issues**: Reference any related issues
5. **Be Responsive**: Address review feedback promptly

## Areas for Contribution

### High Priority
- Additional PMBus command implementations
- Error handling improvements
- Unit tests for conversion functions
- Documentation improvements

### Medium Priority
- Web interface for monitoring
- Data logging to SD card
- MQTT integration
- Additional display modes

### Low Priority
- Support for additional display types
- Multi-language support
- Custom themes

## PMBus Implementation Notes

### Adding New Commands

1. Add command definition to `include/pmbus_commands.h`:
```cpp
{ "NEW_COMMAND", 0xXX, PMBUS_READ | PMBUS_DATATYPE_LINEAR11, 2, stub, stub },
```

2. Implement handler if needed in `src/pmbus.cpp`

3. Add serial command parser if user-facing in `src/main.cpp`

4. Update documentation in `docs/API.md`

### Data Type Flags
- `PMBUS_DATATYPE_LINEAR11`: 5-bit exponent, 11-bit mantissa
- `PMBUS_DATATYPE_LINEAR16`: Separate VOUT_MODE exponent
- `PMBUS_DATATYPE_INTEGER`: Raw integer value
- `PMBUS_DATATYPE_DIRECT`: Direct format (not yet implemented)

## Hardware Safety

**IMPORTANT**: When contributing features that control PSU behavior:

1. Always implement safety checks
2. Add warnings in documentation
3. Test with current-limited power supply first
4. Never assume PSU will behave as expected
5. Implement timeout and fault detection

## Questions?

- Open an issue for bugs or feature requests
- Start a discussion for questions or ideas
- Check existing issues before creating new ones

## License

By contributing, you agree that your contributions will be licensed under the GNU General Public License v2.0.

---

Thank you for contributing to ESPPSUControl! 🚀