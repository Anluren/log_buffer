#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <charconv>
#include <type_traits>

namespace log_buffer {

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
        : m_buffer(buffer), m_capacity(size), m_position(0), m_overflow(false) {}

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
     * @brief Log a raw C buffer (binary data).
     * 
     * Writes raw bytes as-is without any formatting or null terminator.
     * 
     * @param data Pointer to the data to write.
     * @param size Number of bytes to write.
     * @return true if successful, false if buffer overflow would occur.
     */
    inline bool log(const uint8_t* data, std::size_t size) noexcept {
        if (size > remaining_capacity()) {
            m_overflow = true;
            return false;
        }
        std::memcpy(m_buffer + m_position, data, size);
        m_position += size;
        return true;
    }

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
    inline bool log(std::string_view str) noexcept {
        const std::size_t total_size = str.size() + 1; // +1 for null terminator
        if (total_size > remaining_capacity()) {
            m_overflow = true;
            return false;
        }
        std::memcpy(m_buffer + m_position, str.data(), str.size());
        m_buffer[m_position + str.size()] = '\0';
        m_position += total_size;
        return true;
    }

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
     * @brief Log an integer value as ASCII decimal text with null terminator.
     * 
     * Converts the integer to its ASCII decimal representation and writes it
     * followed by a null terminator. Uses std::to_chars for efficient conversion
     * without locale dependency or allocations.
     * 
     * @tparam T Any integral type (int, uint32_t, int64_t, etc.)
     * @param value The integer value to log.
     * @return true if successful, false if conversion fails or buffer overflow would occur.
     * 
     * @note Negative numbers include the '-' sign.
     * @note Maximum space required: 21 bytes (20 digits + null for int64_t min value).
     */
    template<typename T>
    inline std::enable_if_t<std::is_integral_v<T>, bool> log(T value) noexcept {
        // Reserve space for conversion (max 20 chars for int64_t + null terminator)
        char temp_buffer[21];
        auto result = std::to_chars(temp_buffer, temp_buffer + sizeof(temp_buffer) - 1, value);
        
        if (result.ec != std::errc{}) {
            return false;
        }
        
        const std::size_t str_length = result.ptr - temp_buffer;
        const std::size_t total_size = str_length + 1; // +1 for null terminator
        
        if (total_size > remaining_capacity()) {
            m_overflow = true;
            return false;
        }
        
        std::memcpy(m_buffer + m_position, temp_buffer, str_length);
        m_buffer[m_position + str_length] = '\0';
        m_position += total_size;
        return true;
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

private:
    uint8_t* m_buffer;      ///< Pointer to the user-provided buffer
    std::size_t m_capacity; ///< Total capacity of the buffer in bytes
    std::size_t m_position; ///< Current write position in the buffer
    bool m_overflow;        ///< Flag indicating if overflow has occurred
};

} // namespace log_buffer
