#include "log_buffer/logger.hpp"

namespace log_buffer {

bool Logger::log(const uint8_t* data, std::size_t size) noexcept {
    if (size > remaining_capacity()) {
        m_overflow = true;
        return false;
    }
    std::memcpy(m_buffer + m_position, data, size);
    m_position += size;
    return true;
}

bool Logger::log(std::string_view str) noexcept {
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

Logger& Logger::operator<<(std::ios_base& (*manip)(std::ios_base&)) noexcept {
    // Detect which manipulator by calling it on a test stream and checking flags
    
    // Use a dummy ios_base-derived object to detect the manipulator
    struct DummyStream : std::ios {
        DummyStream() : std::ios(nullptr) {}
    } dummy;
    
    std::ios_base::fmtflags old_flags = dummy.flags();
    manip(dummy);
    std::ios_base::fmtflags new_flags = dummy.flags();
    
    // Check if basefield changed
    auto old_basefield = old_flags & std::ios_base::basefield;
    auto new_basefield = new_flags & std::ios_base::basefield;
    
    if (old_basefield != new_basefield) {
        // Basefield changed - handle hex/oct/dec
        if (new_basefield == std::ios_base::hex) {
            // Check uppercase flag
            if (new_flags & std::ios_base::uppercase) {
                m_int_format = IntFormat::HEX;
            } else {
                m_int_format = IntFormat::Hex;
            }
        } else if (new_basefield == std::ios_base::oct) {
            m_int_format = IntFormat::Oct;
        } else if (new_basefield == std::ios_base::dec) {
            m_int_format = IntFormat::Dec;
        }
    } else {
        // Basefield didn't change - check if uppercase flag changed
        bool old_uppercase = old_flags & std::ios_base::uppercase;
        bool new_uppercase = new_flags & std::ios_base::uppercase;
        
        if (old_uppercase != new_uppercase) {
            // uppercase flag changed - update format if currently in hex mode
            if (m_int_format == IntFormat::Hex || m_int_format == IntFormat::HEX) {
                m_int_format = new_uppercase ? IntFormat::HEX : IntFormat::Hex;
            }
        }
    }
    
    return *this;
}

bool Logger::log_formatted_string(const char* str, std::size_t length) noexcept {
    const std::size_t total_size = length + 1; // +1 for null terminator
    
    if (total_size > remaining_capacity()) {
        m_overflow = true;
        return false;
    }
    
    std::memcpy(m_buffer + m_position, str, length);
    m_buffer[m_position + length] = '\0';
    m_position += total_size;
    return true;
}

} // namespace log_buffer
