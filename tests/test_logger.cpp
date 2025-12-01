#include "log_buffer/logger.hpp"
#include <gtest/gtest.h>
#include <ios>
#include <cstring>

using namespace log_buffer;
using log_buffer::BinaryData;  // Allow using BinaryData without namespace prefix

class LoggerTest : public ::testing::Test {
protected:
    static constexpr size_t kBufferSize = 100;
    uint8_t buffer[kBufferSize];
    
    void SetUp() override {
        std::memset(buffer, 0, sizeof(buffer));
    }
};

TEST_F(LoggerTest, CBuffer) {
    Logger logger(buffer, sizeof(buffer));
    
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    EXPECT_TRUE(logger.log(data, sizeof(data)));
    EXPECT_EQ(logger.bytes_written(), 4);
    EXPECT_EQ(std::memcmp(buffer, data, 4), 0);
}

TEST_F(LoggerTest, StringView) {
    Logger logger(buffer, sizeof(buffer));
    
    std::string_view sv = "Hello";
    EXPECT_TRUE(logger.log(sv));
    EXPECT_EQ(logger.bytes_written(), 6); // 5 chars + null terminator
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "Hello");
    EXPECT_EQ(buffer[5], '\0');
}

TEST_F(LoggerTest, ConstCharPtr) {
    Logger logger(buffer, sizeof(buffer));
    
    const char* str = "World";
    EXPECT_TRUE(logger.log(str));
    EXPECT_EQ(logger.bytes_written(), 6); // 5 chars + null terminator
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "World");
}

TEST_F(LoggerTest, StdString) {
    Logger logger(buffer, sizeof(buffer));
    
    std::string str = "C++17";
    EXPECT_TRUE(logger.log(str));
    EXPECT_EQ(logger.bytes_written(), 6); // 5 chars + null terminator
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "C++17");
}

TEST_F(LoggerTest, PositiveInteger) {
    Logger logger(buffer, sizeof(buffer));
    
    EXPECT_TRUE(logger.log(42));
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "42");
    EXPECT_EQ(logger.bytes_written(), 3); // "42" + null
}

TEST_F(LoggerTest, NegativeInteger) {
    Logger logger(buffer, sizeof(buffer));
    
    EXPECT_TRUE(logger.log(-123));
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "-123");
    EXPECT_EQ(logger.bytes_written(), 5); // "-123" + null
}

TEST_F(LoggerTest, LargeUnsignedInteger) {
    Logger logger(buffer, sizeof(buffer));
    
    uint64_t big_num = 9876543210ULL;
    EXPECT_TRUE(logger.log(big_num));
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "9876543210");
}

TEST_F(LoggerTest, MultipleLogs) {
    Logger logger(buffer, sizeof(buffer));
    
    EXPECT_TRUE(logger.log("Name:"));
    EXPECT_TRUE(logger.log("Alice"));
    EXPECT_TRUE(logger.log("Age:"));
    EXPECT_TRUE(logger.log(30));
    
    // Verify data
    const char* ptr = reinterpret_cast<const char*>(buffer);
    EXPECT_STREQ(ptr, "Name:");
    ptr += sizeof("Name:");
    EXPECT_STREQ(ptr, "Alice");
    ptr += sizeof("Alice");
    EXPECT_STREQ(ptr, "Age:");
    ptr += sizeof("Age:");
    EXPECT_STREQ(ptr, "30");
}

TEST_F(LoggerTest, BufferOverflow) {
    uint8_t small_buffer[10];
    Logger logger(small_buffer, sizeof(small_buffer));
    
    EXPECT_FALSE(logger.has_overflowed());
    
    // This should fit
    EXPECT_TRUE(logger.log("Hi"));
    EXPECT_FALSE(logger.has_overflowed());
    EXPECT_EQ(logger.bytes_written(), 3);
    
    // This should cause overflow (10 bytes total, 3 used, need 11 for "VeryLong")
    EXPECT_FALSE(logger.log("VeryLong"));
    EXPECT_TRUE(logger.has_overflowed());
    EXPECT_EQ(logger.bytes_written(), 3); // Position shouldn't change
}

TEST_F(LoggerTest, Reset) {
    Logger logger(buffer, sizeof(buffer));
    
    logger.log("First");
    EXPECT_EQ(logger.bytes_written(), 6);
    
    logger.reset();
    EXPECT_EQ(logger.bytes_written(), 0);
    EXPECT_FALSE(logger.has_overflowed());
    EXPECT_EQ(logger.remaining_capacity(), sizeof(buffer));
    
    logger.log("Second");
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "Second");
}

TEST_F(LoggerTest, RemainingCapacity) {
    uint8_t small_buffer[20];
    Logger logger(small_buffer, sizeof(small_buffer));
    
    EXPECT_EQ(logger.remaining_capacity(), 20);
    
    logger.log("ABC"); // 4 bytes
    EXPECT_EQ(logger.remaining_capacity(), 16);
    
    logger.log(99); // 3 bytes ("99" + null)
    EXPECT_EQ(logger.remaining_capacity(), 13);
}

TEST_F(LoggerTest, DataPointer) {
    Logger logger(buffer, sizeof(buffer));
    
    logger.log("test");
    EXPECT_EQ(logger.data(), buffer);
    EXPECT_STREQ(reinterpret_cast<const char*>(logger.data()), "test");
}

TEST_F(LoggerTest, StreamOperatorString) {
    Logger logger(buffer, sizeof(buffer));
    
    logger << "Hello" << " " << "World";
    
    const char* ptr = reinterpret_cast<const char*>(buffer);
    EXPECT_STREQ(ptr, "Hello");
    ptr += sizeof("Hello");
    EXPECT_STREQ(ptr, " ");
    ptr += sizeof(" ");
    EXPECT_STREQ(ptr, "World");
}

TEST_F(LoggerTest, StreamOperatorInteger) {
    Logger logger(buffer, sizeof(buffer));
    
    logger << 42 << -100 << 999ULL;
    
    const char* ptr = reinterpret_cast<const char*>(buffer);
    EXPECT_STREQ(ptr, "42");
    ptr += sizeof("42");
    EXPECT_STREQ(ptr, "-100");
    ptr += sizeof("-100");
    EXPECT_STREQ(ptr, "999");
}

TEST_F(LoggerTest, StreamOperatorMixed) {
    Logger logger(buffer, sizeof(buffer));
    
    logger << "Count: " << 5 << " Name: " << "Alice";
    
    const char* ptr = reinterpret_cast<const char*>(buffer);
    EXPECT_STREQ(ptr, "Count: ");
    ptr += sizeof("Count: ");
    EXPECT_STREQ(ptr, "5");
    ptr += sizeof("5");
    EXPECT_STREQ(ptr, " Name: ");
    ptr += sizeof(" Name: ");
    EXPECT_STREQ(ptr, "Alice");
}

TEST_F(LoggerTest, StreamOperatorChaining) {
    Logger logger(buffer, sizeof(buffer));
    
    auto& result = (logger << "test");
    EXPECT_EQ(&result, &logger);
    
    // Verify chaining works
    logger.reset();
    logger << "A" << "B" << "C";
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "A");
}

TEST_F(LoggerTest, StreamOperatorBinaryData) {
    Logger logger(buffer, sizeof(buffer));
    
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    logger << std::make_pair(data, sizeof(data));
    
    EXPECT_EQ(logger.bytes_written(), 4);
    EXPECT_EQ(buffer[0], 0xDE);
    EXPECT_EQ(buffer[1], 0xAD);
    EXPECT_EQ(buffer[2], 0xBE);
    EXPECT_EQ(buffer[3], 0xEF);
}

TEST_F(LoggerTest, StreamOperatorBinaryDataChaining) {
    Logger logger(buffer, sizeof(buffer));
    
    uint8_t data1[] = {0x01, 0x02};
    uint8_t data2[] = {0x03, 0x04};
    
    logger << std::make_pair(data1, sizeof(data1)) 
           << "test" 
           << std::make_pair(data2, sizeof(data2));
    
    EXPECT_EQ(buffer[0], 0x01);
    EXPECT_EQ(buffer[1], 0x02);
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer + 2), "test");
    EXPECT_EQ(buffer[7], 0x03);
    EXPECT_EQ(buffer[8], 0x04);
}

TEST_F(LoggerTest, BinaryDataStruct) {
    Logger logger(buffer, sizeof(buffer));
    
    uint8_t data[] = {0xCA, 0xFE, 0xBA, 0xBE};
    logger << BinaryData{data, sizeof(data)};
    
    EXPECT_EQ(logger.bytes_written(), 4);
    EXPECT_EQ(buffer[0], 0xCA);
    EXPECT_EQ(buffer[1], 0xFE);
    EXPECT_EQ(buffer[2], 0xBA);
    EXPECT_EQ(buffer[3], 0xBE);
}

TEST_F(LoggerTest, BinaryDataStructChaining) {
    Logger logger(buffer, sizeof(buffer));
    
    uint8_t data[] = {0xFF, 0xEE};
    logger << BinaryData{data, sizeof(data)} << "END";
    
    EXPECT_EQ(buffer[0], 0xFF);
    EXPECT_EQ(buffer[1], 0xEE);
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer + 2), "END");
}

TEST_F(LoggerTest, IntegerFormatDecimal) {
    Logger logger(buffer, sizeof(buffer));
    
    logger.set_int_format(log_buffer::IntFormat::Dec);
    logger.log(255);
    
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "255");
}

TEST_F(LoggerTest, IntegerFormatHexLowercase) {
    Logger logger(buffer, sizeof(buffer));
    
    logger.set_int_format(log_buffer::IntFormat::Hex);
    logger.log(255);
    
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "0xff");
}

TEST_F(LoggerTest, IntegerFormatHexUppercase) {
    Logger logger(buffer, sizeof(buffer));
    
    logger.set_int_format(log_buffer::IntFormat::HEX);
    logger.log(255);
    
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "0XFF");
}

TEST_F(LoggerTest, IntegerFormatOctal) {
    Logger logger(buffer, sizeof(buffer));
    
    logger.set_int_format(log_buffer::IntFormat::Oct);
    logger.log(64);
    
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "0100");
}

TEST_F(LoggerTest, IntegerFormatChaining) {
    Logger logger(buffer, sizeof(buffer));
    
    logger.set_int_format(log_buffer::IntFormat::Hex).log(16);
    
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "0x10");
}

TEST_F(LoggerTest, IntegerFormatMixed) {
    Logger logger(buffer, sizeof(buffer));
    
    logger.set_int_format(log_buffer::IntFormat::Dec);
    logger.log(10);
    
    logger.set_int_format(log_buffer::IntFormat::Hex);
    logger.log(10);
    
    logger.set_int_format(log_buffer::IntFormat::Oct);
    logger.log(10);
    
    const char* ptr = reinterpret_cast<const char*>(buffer);
    EXPECT_STREQ(ptr, "10");
    ptr += sizeof("10");
    EXPECT_STREQ(ptr, "0xa");
    ptr += sizeof("0xa");
    EXPECT_STREQ(ptr, "012");
}

TEST_F(LoggerTest, GetIntFormat) {
    Logger logger(buffer, sizeof(buffer));
    
    EXPECT_EQ(logger.get_int_format(), log_buffer::IntFormat::Dec);
    
    logger.set_int_format(log_buffer::IntFormat::Hex);
    EXPECT_EQ(logger.get_int_format(), log_buffer::IntFormat::Hex);
}

TEST_F(LoggerTest, StreamOperatorWithFormatDecimal) {
    Logger logger(buffer, sizeof(buffer));
    
    logger.set_int_format(log_buffer::IntFormat::Dec);
    logger << 42;
    
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "42");
}

TEST_F(LoggerTest, StreamOperatorWithFormatHex) {
    Logger logger(buffer, sizeof(buffer));
    
    logger.set_int_format(log_buffer::IntFormat::Hex);
    logger << 255;
    
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "0xff");
}

TEST_F(LoggerTest, StreamOperatorWithFormatHexUppercase) {
    Logger logger(buffer, sizeof(buffer));
    
    logger.set_int_format(log_buffer::IntFormat::HEX);
    logger << 255;
    
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "0XFF");
}

TEST_F(LoggerTest, StreamOperatorWithFormatOctal) {
    Logger logger(buffer, sizeof(buffer));
    
    logger.set_int_format(log_buffer::IntFormat::Oct);
    logger << 64;
    
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "0100");
}

TEST_F(LoggerTest, StreamOperatorWithFormatMixedChaining) {
    Logger logger(buffer, sizeof(buffer));
    
    logger.set_int_format(log_buffer::IntFormat::Dec);
    logger << 10;
    
    logger.set_int_format(log_buffer::IntFormat::Hex);
    logger << 10;
    
    logger.set_int_format(log_buffer::IntFormat::Oct);
    logger << 10;
    
    const char* ptr = reinterpret_cast<const char*>(buffer);
    EXPECT_STREQ(ptr, "10");
    ptr += sizeof("10");
    EXPECT_STREQ(ptr, "0xa");
    ptr += sizeof("0xa");
    EXPECT_STREQ(ptr, "012");
}

TEST_F(LoggerTest, StdManipulatorHex) {
    Logger logger(buffer, sizeof(buffer));
    
    logger << std::hex << 255;
    
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "0xff");
}

TEST_F(LoggerTest, StdManipulatorDec) {
    Logger logger(buffer, sizeof(buffer));
    
    logger << std::dec << 42;
    
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "42");
}

TEST_F(LoggerTest, StdManipulatorOct) {
    Logger logger(buffer, sizeof(buffer));
    
    logger << std::oct << 64;
    
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "0100");
}

TEST_F(LoggerTest, StdManipulatorHexUppercase) {
    Logger logger(buffer, sizeof(buffer));
    
    logger << std::hex << std::uppercase << 255;
    
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer), "0XFF");
}

TEST_F(LoggerTest, StdManipulatorChaining) {
    Logger logger(buffer, sizeof(buffer));
    
    logger << std::dec << 10 << std::hex << 16 << std::oct << 8;
    
    const char* ptr = reinterpret_cast<const char*>(buffer);
    EXPECT_STREQ(ptr, "10");
    ptr += sizeof("10");
    EXPECT_STREQ(ptr, "0x10");
    ptr += sizeof("0x10");
    EXPECT_STREQ(ptr, "010");
}

TEST_F(LoggerTest, StdManipulatorMixedWithStrings) {
    Logger logger(buffer, sizeof(buffer));
    
    logger << "Value: " << std::hex << 255 << " End";
    
    // Check each part of the buffer
    const char* ptr = reinterpret_cast<const char*>(buffer);
    EXPECT_STREQ(ptr, "Value: ");
    ptr += sizeof("Value: ");
    EXPECT_STREQ(ptr, "0xff");
    ptr += sizeof("0xff");
    EXPECT_STREQ(ptr, " End");
}
