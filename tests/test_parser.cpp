#include <gtest/gtest.h>
#include "../src/parser/endian_utils.h"
#include "../src/parser/itch_parser.h"
#include <vector>

using namespace itch;

// Helper to write big-endian to a buffer
void write_be16(char* buf, uint16_t val) {
    val = __builtin_bswap16(val);
    std::memcpy(buf, &val, sizeof(val));
}

void write_be32(char* buf, uint32_t val) {
    val = __builtin_bswap32(val);
    std::memcpy(buf, &val, sizeof(val));
}

void write_be64(char* buf, uint64_t val) {
    val = __builtin_bswap64(val);
    std::memcpy(buf, &val, sizeof(val));
}

void write_be48(char* buf, uint64_t val) {
    buf[0] = (val >> 40) & 0xFF;
    buf[1] = (val >> 32) & 0xFF;
    buf[2] = (val >> 24) & 0xFF;
    buf[3] = (val >> 16) & 0xFF;
    buf[4] = (val >> 8) & 0xFF;
    buf[5] = val & 0xFF;
}

TEST(EndianUtilsTest, BasicConversions) {
    char buf16[2];
    write_be16(buf16, 0x1234);
    EXPECT_EQ(read_be16(buf16), 0x1234);

    char buf32[4];
    write_be32(buf32, 0x12345678);
    EXPECT_EQ(read_be32(buf32), 0x12345678);

    char buf64[8];
    write_be64(buf64, 0x1234567890ABCDEF);
    EXPECT_EQ(read_be64(buf64), 0x1234567890ABCDEF);

    char buf48[6];
    write_be48(buf48, 0x1234567890AB);
    EXPECT_EQ(read_be48(buf48), 0x1234567890AB);
}

TEST(ParserTest, AddOrderMessage) {
    std::vector<char> buf(38);
    char* p = buf.data();
    
    // Length prefix
    write_be16(p, 36); p += 2;
    *p++ = 'A';
    write_be16(p, 1); p += 2; // stock locate
    write_be16(p, 2); p += 2; // tracking number
    write_be48(p, 1234567890123); p += 6; // timestamp
    write_be64(p, 42); p += 8; // order ref
    *p++ = 'B'; // side
    write_be32(p, 100); p += 4; // shares
    std::memcpy(p, "AAPL    ", 8); p += 8; // stock
    write_be32(p, 1500000); // price (150.0000)

    ItchParser parser;
    Message msg;
    size_t consumed = parser.parse_message(buf.data(), buf.size(), msg);
    
    EXPECT_EQ(consumed, 38);
    ASSERT_TRUE(std::holds_alternative<AddOrder>(msg));
    
    const auto& order = std::get<AddOrder>(msg);
    EXPECT_EQ(order.stock_locate, 1);
    EXPECT_EQ(order.tracking_number, 2);
    EXPECT_EQ(order.timestamp, 1234567890123);
    EXPECT_EQ(order.order_ref, 42);
    EXPECT_EQ(order.side, Side::Buy);
    EXPECT_EQ(order.shares, 100);
    EXPECT_EQ(stock_to_string(order.stock), "AAPL");
    EXPECT_EQ(order.price, 1500000);
}

TEST(ParserTest, OrderDeleteMessage) {
    std::vector<char> buf(21);
    char* p = buf.data();
    
    write_be16(p, 19); p += 2; // length
    *p++ = 'D'; // type
    write_be16(p, 10); p += 2; // stock locate
    write_be16(p, 20); p += 2; // tracking number
    write_be48(p, 111222333); p += 6; // timestamp
    write_be64(p, 999); p += 8; // order ref

    ItchParser parser;
    Message msg;
    size_t consumed = parser.parse_message(buf.data(), buf.size(), msg);
    
    EXPECT_EQ(consumed, 21);
    ASSERT_TRUE(std::holds_alternative<OrderDelete>(msg));
    
    const auto& del = std::get<OrderDelete>(msg);
    EXPECT_EQ(del.stock_locate, 10);
    EXPECT_EQ(del.order_ref, 999);
}

TEST(ParserTest, BufferTooShort) {
    std::vector<char> buf = { 0, 10, 'A', 0, 1 }; // Only 5 bytes
    
    ItchParser parser;
    Message msg;
    size_t consumed = parser.parse_message(buf.data(), buf.size(), msg);
    EXPECT_EQ(consumed, 0); // Not enough bytes to even parse
}

TEST(ParserTest, UnknownMessage) {
    std::vector<char> buf(12);
    char* p = buf.data();
    
    write_be16(p, 10); p += 2; // length = 10
    *p++ = 'Z'; // Unknown type
    // fill rest with junk
    
    ItchParser parser;
    Message msg;
    size_t consumed = parser.parse_message(buf.data(), buf.size(), msg);
    
    // Should consume the exact length (2 prefix + 10 body = 12)
    EXPECT_EQ(consumed, 12);
}
