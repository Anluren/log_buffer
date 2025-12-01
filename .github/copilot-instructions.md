# Copilot Instructions for log_buffer

## Project Overview
A C++17 header-only logging library that writes to a user-provided buffer. The library accepts a pre-allocated buffer (`uint8_t*` and size) and provides type-safe methods to log various data types without dynamic allocation.

## Architecture

### Header-Only Design
- All implementation must be in headers (`.hpp` files)
- Use `inline` for functions to avoid multiple definition errors
- Template implementations should be in the same header as declarations

### Buffer Management
- **External buffer**: User provides `uint8_t*` buffer and size
- **No dynamic allocation**: Library never allocates memory
- **Bounds checking**: Always verify writes don't exceed buffer capacity
- Handle buffer overflow gracefully (return error codes or track overflow state)

### Supported Data Types
The library must support logging these types:
1. **C buffer**: `uint8_t*` with size parameter - raw bytes copied as-is
2. **std::string_view**: Efficient string logging without copies - written with null terminator
3. **const char***: Null-terminated C strings - written with null terminator
4. **std::string**: Standard string objects - written with null terminator
5. **Integer types**: `int`, `uint32_t`, `int64_t`, etc. - formatted as ASCII text with null terminator

Consider template-based overloading for extensibility.

### Threading Model
- **Not thread-safe**: Users are responsible for synchronization if accessing from multiple threads
- Document this clearly in API comments
- Avoid internal locks or atomics to keep the library lightweight

## Development Workflow

### Setup
```bash
# C++17 compiler required (GCC 7+, Clang 5+, MSVC 2017+)
# No build step needed - header-only library
```

### Testing
```bash
# Build with CMake (fetches GoogleTest automatically)
mkdir build && cd build
cmake ..
cmake --build .

# Run tests
./test_logger
# or use CTest
ctest --output-on-failure

# Alternative: compile manually without CMake (requires GoogleTest installed)
g++ -std=c++17 -Wall -Wextra -Werror tests/test_logger.cpp -lgtest -lgtest_main -pthread -o test_logger
```

### Integration
Users include headers and provide their own buffer:
```cpp
#include "log_buffer/logger.hpp"
uint8_t buffer[1024];
LogBuffer logger(buffer, sizeof(buffer));
logger.log("Hello");
logger.log(42);
```

## Code Conventions

### C++17 Features
- Use `std::string_view` for string parameters (avoid copies)
- Use `if constexpr` for compile-time branching
- Use structured bindings where appropriate
- Prefer `constexpr` for compile-time constants

### API Design
- Methods should return status (bool, enum, or std::optional)
- Use method chaining where it makes sense
- Provide const-correct interfaces
- Clear naming: `log()`, `write()`, `append()`, etc.

### Error Handling
- No exceptions (suitable for embedded/constrained environments)
- Return error codes or status enums
- Track buffer overflow state
- Provide `remaining_capacity()` or similar query methods

## Key Files & Directories
```
include/log_buffer/  - Public API headers
tests/               - Unit tests
examples/            - Usage examples
README.md            - API documentation and examples
```

### String Null Terminators
All strings (including `std::string`, `std::string_view`, `const char*`) are written with null terminators. Account for the extra byte when checking buffer capacity.

### Integer Formatting
Integers are converted to ASCII decimal representation with null terminators. Use `std::to_chars` for efficient conversion without locale dependency or allocations.
### String Null Terminators
When logging C strings, decide if null terminators should be included in the buffer or not (document the behavior).

### Integer Formatting
Decide on integer representation: binary, ASCII decimal, or hex. Consider providing formatting options.

### Alignment
Consider if data should be aligned (e.g., for efficient parsing) or packed tightly.

---
*Last updated: December 1, 2025*
