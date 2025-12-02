#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <charconv>
#include <type_traits>
#include <ios>

namespace log_buffer {

/**
 * @enum IntFormat
 * @brief Integer formatting options for logging.
 */
enum class IntFormat {
    Dec,  ///< Decimal format (base 10)
    Hex,  ///< Hexadecimal format (base 16, lowercase)
    HEX,  ///< Hexadecimal format (base 16, uppercase)
    Oct   ///< Octal format (base 8)
};

/**
 * @struct BinaryData
 * @brief Helper struct for logging binary data with convenient brace initialization.
 * 
 * Allows using brace-enclosed syntax for binary data logging:
 * @code
 * uint8_t data[] = {0x01, 0x02, 0x03};
 * logger << BinaryData{data, sizeof(data)};
 * @endcode
 */
struct BinaryData {
    const uint8_t* data;  ///< Pointer to binary data
    std::size_t size;     ///< Size of data in bytes
};

/**
 * @class Logger
 * @brief A header-only logging library that writes to a user-provided buffer.
 * 
 * This class provides a lightweight, zero-allocation logging interface that writes
 * various data types to a pre-allocated buffer. All data is formatted and stored
 * sequentially in the buffer with appropriate null terminators for strings.
 * 
 * @note Thread Safety: This class is NOT thread-safe. Users must provide their own
 *       synchronization if accessing from multiple threads.
 * 
 * @note Memory: No dynamic allocation - all data written to user-provided buffer.
 * 
 * @example
 * @code
 * uint8_t buffer[256];
 * Logger logger(buffer, sizeof(buffer));
 * logger.log("User: ");
 * logger.log("alice");
 * logger.log(" Count: ");
 * logger.log(42);
 * @endcode
 */
class Logger {
public:
    /**
     * @brief Construct a logger with a user-provided buffer.
     * 
     * @param buffer Pointer to the buffer to write to. Must remain valid for the
     *               lifetime of the Logger instance.
     * @param size Size of the buffer in bytes. Must be greater than 0.
     * 
     * @note The buffer is not initialized or cleared by the constructor.
     */
    inline Logger(uint8_t* buffer, std::size_t size) noexcept
        : m_buffer(buffer), m_capacity(size), m_position(0), m_overflow(false), m_int_format(IntFormat::Dec) {}

    /**
     * @brief Get the number of bytes written to the buffer.
     * 
     * @return Total number of bytes written since construction or last reset().
     */
    inline std::size_t bytes_written() const noexcept { return m_position; }

    /**
     * @brief Get the remaining capacity in the buffer.
     * 
     * @return Number of bytes available for writing before overflow occurs.
     */
    inline std::size_t remaining_capacity() const noexcept { 
        return m_capacity > m_position ? m_capacity - m_position : 0; 
    }

    /**
     * @brief Check if a buffer overflow has occurred.
     * 
     * An overflow occurs when a write operation would exceed the buffer capacity.
     * When overflow happens, the write is rejected and this flag is set.
     * 
     * @return true if an overflow has occurred, false otherwise.
     */
    inline bool has_overflowed() const noexcept { return m_overflow; }

    /**
     * @brief Reset the logger to start writing from the beginning of the buffer.
     * 
     * Clears the overflow flag and resets the write position to 0.
     * Does not clear the buffer contents.
     */
    inline void reset() noexcept {
        m_position = 0;
        m_overflow = false;
    }

    /**
     * @brief Set the integer format for subsequent integer logging.
     * 
     * @param format The format to use (Dec, Hex, HEX, Oct).
     * @return Reference to this Logger for chaining.
     */
    inline Logger& set_int_format(IntFormat format) noexcept {
        m_int_format = format;
        return *this;
    }

    /**
     * @brief Get the current integer format.
     * 
     * @return The current integer format setting.
     */
    inline IntFormat get_int_format() const noexcept {
        return m_int_format;
    }

    /**
     * @brief Log a raw C buffer (binary data).
     * 
     * Writes raw bytes as-is without any formatting or null terminator.
     * 
     * @param data Pointer to the data to write.
     * @param size Number of bytes to write.
     * @return true if successful, false if buffer overflow would occur.
     */
    bool log(const uint8_t* data, std::size_t size) noexcept;

    /**
     * @brief Log a std::string_view with null terminator.
     * 
     * Writes the string contents followed by a null terminator ('\0').
     * 
     * @param str The string view to log.
     * @return true if successful, false if buffer overflow would occur.
     * 
     * @note Requires str.size() + 1 bytes of available capacity.
     */
    bool log(std::string_view str) noexcept;

    /**
     * @brief Log a C string (const char*) with null terminator.
     * 
     * Writes the null-terminated C string followed by a null terminator.
     * 
     * @param str The null-terminated C string to log.
     * @return true if successful, false if buffer overflow would occur.
     */
    inline bool log(const char* str) noexcept {
        return log(std::string_view(str));
    }

    /**
     * @brief Log a std::string with null terminator.
     * 
     * Writes the string contents followed by a null terminator.
     * 
     * @param str The std::string to log.
     * @return true if successful, false if buffer overflow would occur.
     */
    inline bool log(const std::string& str) noexcept {
        return log(std::string_view(str));
    }

    /**
     * @brief Log an integer value as ASCII text with null terminator.
     * 
     * Converts the integer according to the current format setting (decimal, hex, or octal)
     * and writes it followed by a null terminator. Uses std::to_chars for efficient conversion
     * without locale dependency or allocations.
     * 
     * @tparam T Any integral type (int, uint32_t, int64_t, etc.)
     * @param value The integer value to log.
     * @return true if successful, false if conversion fails or buffer overflow would occur.
     * 
     * @note Negative numbers include the '-' sign.
     * @note Maximum space required: 68 bytes (64 bits in binary + prefix + null).
     */
    template<typename T>
    inline std::enable_if_t<std::is_integral_v<T>, bool> log(T value) noexcept {
        // Reserve space for conversion (64 bits in any base + prefix + null)
        char temp_buffer[68];
        char* start = temp_buffer;
        
        // Add prefix for hex/oct if needed
        int base = 10;
        switch (m_int_format) {
            case IntFormat::Hex:
                *start++ = '0';
                *start++ = 'x';
                base = 16;
                break;
            case IntFormat::HEX:
                *start++ = '0';
                *start++ = 'X';
                base = 16;
                break;
            case IntFormat::Oct:
                *start++ = '0';
                base = 8;
                break;
            case IntFormat::Dec:
            default:
                base = 10;
                break;
        }
        
        auto result = std::to_chars(start, temp_buffer + sizeof(temp_buffer) - 1, value, base);
        
        if (result.ec != std::errc{}) {
            return false;
        }
        
        // Convert to uppercase if HEX format
        if (m_int_format == IntFormat::HEX) {
            for (char* p = start; p < result.ptr; ++p) {
                if (*p >= 'a' && *p <= 'f') {
                    *p = *p - 'a' + 'A';
                }
            }
        }
        
        const std::size_t str_length = result.ptr - temp_buffer;
        return log_formatted_string(temp_buffer, str_length);
    }

    /**
     * @brief Get a pointer to the buffer.
     * 
     * @return Const pointer to the beginning of the buffer.
     * 
     * @note The returned pointer remains valid as long as the Logger instance exists
     *       and the original buffer has not been deallocated.
     */
    inline const uint8_t* data() const noexcept { return m_buffer; }

    /**
     * @brief Stream insertion operator for binary data.
     * 
     * Allows using the << operator to log data to the buffer.
     * 
     * @param data Pointer to the data to write.
     * @param size Number of bytes to write.
     * @return Reference to this Logger for chaining.
     * 
     * @note If the write fails due to overflow, the operation is silently ignored.
     *       Check has_overflowed() to detect failures.
     */
    inline Logger& operator<<(const std::pair<const uint8_t*, std::size_t>& data) noexcept {
        log(data.first, data.second);
        return *this;
    }

    /**
     * @brief Stream insertion operator for BinaryData.
     * 
     * Allows convenient brace-initialization syntax for binary data:
     * @code
     * logger << BinaryData{data_ptr, data_size};
     * @endcode
     * 
     * @param data The BinaryData struct containing pointer and size.
     * @return Reference to this Logger for chaining.
     */
    inline Logger& operator<<(const BinaryData& data) noexcept {
        log(data.data, data.size);
        return *this;
    }

    /**
     * @brief Stream insertion operator for std::string_view.
     * 
     * @param str The string view to log.
     * @return Reference to this Logger for chaining.
     */
    inline Logger& operator<<(std::string_view str) noexcept {
        log(str);
        return *this;
    }

    /**
     * @brief Stream insertion operator for C strings.
     * 
     * @param str The null-terminated C string to log.
     * @return Reference to this Logger for chaining.
     */
    inline Logger& operator<<(const char* str) noexcept {
        log(str);
        return *this;
    }

    /**
     * @brief Stream insertion operator for std::string.
     * 
     * @param str The std::string to log.
     * @return Reference to this Logger for chaining.
     */
    inline Logger& operator<<(const std::string& str) noexcept {
        log(str);
        return *this;
    }

    /**
     * @brief Stream insertion operator for integral types.
     * 
     * @tparam T Any integral type (int, uint32_t, int64_t, etc.)
     * @param value The integer value to log.
     * @return Reference to this Logger for chaining.
     */
    template<typename T>
    inline std::enable_if_t<std::is_integral_v<T>, Logger&> operator<<(T value) noexcept {
        log(value);
        return *this;
    }

    /**
     * @brief Stream insertion operator for std::ios_base manipulators.
     * 
     * Supports standard manipulators like std::hex, std::dec, std::oct, std::uppercase.
     * 
     * @param manip The manipulator function pointer.
     * @return Reference to this Logger for chaining.
     * 
     * @note std::uppercase affects hex format (Hex -> HEX), while std::nouppercase
     *       resets to lowercase hex.
     */
    Logger& operator<<(std::ios_base& (*manip)(std::ios_base&)) noexcept;

private:
    /**
     * @brief Helper function to log a formatted string with null terminator.
     * 
     * @param str Pointer to the formatted string buffer.
     * @param length Length of the string (without null terminator).
     * @return true if successful, false if buffer overflow would occur.
     */
    bool log_formatted_string(const char* str, std::size_t length) noexcept;

    uint8_t* m_buffer;         ///< Pointer to the user-provided buffer
    std::size_t m_capacity;    ///< Total capacity of the buffer in bytes
    std::size_t m_position;    ///< Current write position in the buffer
    bool m_overflow;           ///< Flag indicating if overflow has occurred
    IntFormat m_int_format;    ///< Current integer format setting
};

} // namespace log_buffer
