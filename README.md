# log_buffer

A lightweight, header-only C++17 logging library that writes to a user-provided buffer with zero dynamic allocation.

## Features

- **Header-only**: Just include and use, no linking required
- **Zero allocation**: All data written to user-provided buffer
- **Type-safe**: Template-based API for various data types
- **No dependencies**: Only standard C++17 library
- **Overflow protection**: Safe buffer boundary checking
- **Embedded-friendly**: No exceptions, suitable for constrained environments

## Supported Data Types

- **Raw C buffers**: `uint8_t*` with size (binary data)
- **Strings**: `std::string_view`, `const char*`, `std::string` (null-terminated)
- **Integers**: All integral types with configurable format:
  - Decimal (default)
  - Hexadecimal (lowercase or uppercase)
  - Octal

## Quick Start

```cpp
#include "log_buffer/logger.hpp"

uint8_t buffer[256];
log_buffer::Logger logger(buffer, sizeof(buffer));

// Log strings
logger.log("Hello, World!");

// Log integers
logger.log(42);
logger.log(-123);

// Log integers in different formats
logger.set_int_format(log_buffer::IntFormat::Hex);
logger.log(255);  // Writes "0xff"

logger.set_int_format(log_buffer::IntFormat::HEX);
logger.log(255);  // Writes "0XFF"

logger.set_int_format(log_buffer::IntFormat::Oct);
logger.log(64);   // Writes "0100"

// Or use standard manipulators (std::hex, std::dec, std::oct, std::uppercase)
logger << std::hex << 255;  // Writes "0xff"
logger << std::hex << std::uppercase << 255;  // Writes "0XFF"
logger << std::oct << 64;   // Writes "0100"
logger << std::dec << 42;   // Writes "42"

// Stream-style logging with format control
logger << "Value: " << std::hex << 255 << " End";  // Writes "Value: 0xff End"

// Log raw binary data
uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
logger.log(data, sizeof(data));

// Check status
if (logger.has_overflowed()) {
    // Handle overflow
}
```

## Installation

### Header-Only Usage

Simply copy the `include/log_buffer` directory to your project and include it:

```cpp
#include "log_buffer/logger.hpp"
```

Or add it to your include path:
```bash
g++ -std=c++17 -I/path/to/log_buffer/include your_file.cpp
```

### CMake Integration

Add to your `CMakeLists.txt`:
```cmake
add_subdirectory(path/to/log_buffer)
target_link_libraries(your_target PRIVATE log_buffer)
```

## API Reference

### Constructor
```cpp
Logger(uint8_t* buffer, size_t size)
```
Creates a logger with a user-provided buffer.

### Logging Methods
```cpp
bool log(const uint8_t* data, size_t size)  // Raw binary data
bool log(std::string_view str)              // String (null-terminated)
bool log(const char* str)                   // C string (null-terminated)
bool log(const std::string& str)            // std::string (null-terminated)
bool log(T value)                           // Integer (formatted per current format)
```
Returns `true` on success, `false` if buffer overflow would occur.

### Integer Format Control
```cpp
Logger& set_int_format(IntFormat format)    // Set format (Dec, Hex, HEX, Oct)
IntFormat get_int_format() const            // Get current format
```
Format options:
- `IntFormat::Dec` - Decimal (e.g., "42")
- `IntFormat::Hex` - Hexadecimal lowercase with 0x prefix (e.g., "0x2a")
- `IntFormat::HEX` - Hexadecimal uppercase with 0X prefix (e.g., "0X2A")
- `IntFormat::Oct` - Octal with 0 prefix (e.g., "052")

### Stream Operators
```cpp
Logger& operator<<(const uint8_t* data, size_t size)
Logger& operator<<(std::string_view str)
Logger& operator<<(const char* str)
Logger& operator<<(const std::string& str)
Logger& operator<<(T value)  // Integers use current format setting
Logger& operator<<(BinaryData{ptr, size})  // Convenient binary data syntax
Logger& operator<<(std::ios_base& (*manip)(std::ios_base&))  // std::hex, std::dec, std::oct, std::uppercase
```

Supported standard manipulators:
- `std::hex` - Switch to hexadecimal format (lowercase)
- `std::dec` - Switch to decimal format (default)
- `std::oct` - Switch to octal format
- `std::uppercase` - Make hex uppercase (when in hex mode)
- `std::nouppercase` - Make hex lowercase (when in hex mode)

### Status Methods
```cpp
size_t bytes_written() const        // Total bytes written
size_t remaining_capacity() const   // Available space
bool has_overflowed() const         // Check if overflow occurred
void reset()                        // Reset to beginning
const uint8_t* data() const         // Get buffer pointer
```

## Building Examples and Tests

### Using CMake (Recommended)

```bash
# Configure and build
mkdir build && cd build
cmake ..
cmake --build .

# Run tests
./test_logger
# or
ctest --output-on-failure

# Run example
./basic_usage
```

### Using g++ directly (header-only)

```bash
# Build and run example (no external dependencies)
g++ -std=c++17 -Iinclude examples/basic_usage.cpp -o basic_usage
./basic_usage
```

## Thread Safety

⚠️ **This library is NOT thread-safe.** Users must provide their own synchronization if accessing a logger instance from multiple threads.

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Standard library support for `<charconv>`, `<string_view>`

## License

MIT (or specify your license)

## Examples

### Integer Formatting
```cpp
uint8_t buffer[512];
log_buffer::Logger logger(buffer, sizeof(buffer));

// Using standard manipulators (recommended)
logger << std::hex << 255;  // "0xff"
logger << std::hex << std::uppercase << 255;  // "0XFF"
logger << std::oct << 64;   // "0100"
logger << std::dec << 42;   // "42"

// Or using set_int_format()
logger.set_int_format(log_buffer::IntFormat::Hex);
logger.log(255);  // "0xff"

logger.set_int_format(log_buffer::IntFormat::HEX);
logger.log(255);  // "0XFF"

logger.set_int_format(log_buffer::IntFormat::Oct);
logger.log(64);   // "0100"

// Mix formats in one buffer
logger << std::dec << "Dec: " << 16 << " ";
logger << std::hex << "Hex: " << 16;  // "Dec: 16 Hex: 0x10"
```

### Structured Logging
```cpp
uint8_t buffer[512];
log_buffer::Logger logger(buffer, sizeof(buffer));

logger.log("User: ");
logger.log("alice");
logger.log(" Action: ");
logger.log("login");
logger.log(" Timestamp: ");
logger.log(1701436800);
```

### Buffer Reuse
```cpp
logger.reset();  // Clear buffer and start over
logger.log("New session");
```

### Overflow Handling
```cpp
if (!logger.log("Very long string...")) {
    if (logger.has_overflowed()) {
        // Handle overflow - maybe flush buffer or increase size
    }
}
```
