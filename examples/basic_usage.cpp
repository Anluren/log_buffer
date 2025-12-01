#include "../include/log_buffer/logger.hpp"
#include <iostream>
#include <cstdint>

int main() {
    // Allocate a buffer on the stack
    uint8_t buffer[256];
    
    // Create logger with the buffer
    log_buffer::Logger logger(buffer, sizeof(buffer));
    
    // Log various data types
    logger.log("User logged in: ");
    logger.log("john_doe");
    logger.log(" at timestamp: ");
    logger.log(1701436800);
    
    // Log raw binary data
    uint8_t binary_data[]  {0xDE, 0xAD, 0xBE, 0xEF};
    logger.log(binary_data, sizeof(binary_data));
    
    // Check status
    std::cout << "Bytes written: " << logger.bytes_written() << std::endl;
    std::cout << "Remaining capacity: " << logger.remaining_capacity() << std::endl;
    std::cout << "Has overflowed: " << (logger.has_overflowed() ? "yes" : "no") << std::endl;
    
    // Print the buffer contents (strings only, skip binary)
    std::cout << "\nBuffer contents (text portion): " << std::endl;
    const char* ptr = reinterpret_cast<const char*>(logger.data());
    
    // Print first few null-terminated strings
    for (int i = 0; i < 4; ++i) {
        std::cout << "  Entry " << i << ": " << ptr << std::endl;
        ptr += std::strlen(ptr) + 1;
    }
    
    // Reset and reuse the buffer
    logger.reset();
    logger.log("Buffer reused!");
    
    std::cout << "\nAfter reset: " << reinterpret_cast<const char*>(logger.data()) << std::endl;
    
    return 0;
}
