#include "feed/feed_handler.h"
#include <gtest/gtest.h>
#include <fstream>
#include <cstring>
#include <vector>

using namespace itch;

// Helper to write big-endian values
namespace {

void write_be16(char* buf, uint16_t val) {
    buf[0] = (val >> 8) & 0xFF;
    buf[1] = val & 0xFF;
}

void write_be32(char* buf, uint32_t val) {
    buf[0] = (val >> 24) & 0xFF;
    buf[1] = (val >> 16) & 0xFF;
    buf[2] = (val >> 8) & 0xFF;
    buf[3] = val & 0xFF;
}

void write_be48(char* buf, uint64_t val) {
    buf[0] = (val >> 40) & 0xFF;
    buf[1] = (val >> 32) & 0xFF;
    buf[2] = (val >> 24) & 0xFF;
    buf[3] = (val >> 16) & 0xFF;
    buf[4] = (val >> 8) & 0xFF;
    buf[5] = val & 0xFF;
}

void write_be64(char* buf, uint64_t val) {
    buf[0] = (val >> 56) & 0xFF;
    buf[1] = (val >> 48) & 0xFF;
    buf[2] = (val >> 40) & 0xFF;
    buf[3] = (val >> 32) & 0xFF;
    buf[4] = (val >> 24) & 0xFF;
    buf[5] = (val >> 16) & 0xFF;
    buf[6] = (val >> 8) & 0xFF;
    buf[7] = val & 0xFF;
}

size_t build_stock_directory(char* buf, uint16_t locate, const char* stock) {
    write_be16(buf, 39);
    buf[2] = 'R';
    write_be16(buf+3, locate);
    write_be16(buf+5, 0);
    write_be48(buf+7, 1000ULL);
    std::memcpy(buf+13, stock, 8);
    std::memset(buf+21, ' ', 18);
    return 41;
}

size_t build_add_order(char* buf, uint16_t locate, uint64_t order_ref,
                       char side, uint32_t shares, const char* stock, uint32_t price) {
    write_be16(buf, 36);
    buf[2] = 'A';
    write_be16(buf+3, locate);
    write_be16(buf+5, 0);
    write_be48(buf+7, 1000000000ULL);
    write_be64(buf+13, order_ref);
    buf[21] = side;
    write_be32(buf+22, shares);
    std::memcpy(buf+26, stock, 8);
    write_be32(buf+34, price);
    return 38;
}

size_t build_order_executed(char* buf, uint16_t locate, uint64_t order_ref,
                            uint32_t executed_shares, uint64_t match_number) {
    write_be16(buf, 31);
    buf[2] = 'E';
    write_be16(buf+3, locate);
    write_be16(buf+5, 0);
    write_be48(buf+7, 2000000000ULL);
    write_be64(buf+13, order_ref);
    write_be32(buf+21, executed_shares);
    write_be64(buf+25, match_number);
    return 33;
}

size_t build_order_delete(char* buf, uint16_t locate, uint64_t order_ref) {
    write_be16(buf, 19);
    buf[2] = 'D';
    write_be16(buf+3, locate);
    write_be16(buf+5, 0);
    write_be48(buf+7, 3000000000ULL);
    write_be64(buf+13, order_ref);
    return 21;
}

} // anonymous namespace

// Integration test: builds a synthetic ITCH file and processes it through the full pipeline
TEST(FeedHandlerTest, ProcessSyntheticFile) {
    const char* filename = "test_feed.itch";
    
    // Build a synthetic ITCH binary file
    {
        std::ofstream out(filename, std::ios::binary);
        char buf[1024];
        size_t len;

        // 1. Stock Directory for AAPL
        len = build_stock_directory(buf, 1, "AAPL    ");
        out.write(buf, len);

        // 2. Add Order: Buy 100 @ $150.0000
        len = build_add_order(buf, 1, 1001, 'B', 100, "AAPL    ", 1500000);
        out.write(buf, len);

        // 3. Add Order: Buy 50 @ $150.0000
        len = build_add_order(buf, 1, 1002, 'B', 50, "AAPL    ", 1500000);
        out.write(buf, len);

        // 4. Add Order: Sell 200 @ $151.0000
        len = build_add_order(buf, 1, 1003, 'S', 200, "AAPL    ", 1510000);
        out.write(buf, len);

        // 5. Execute 50 shares of order 1001
        len = build_order_executed(buf, 1, 1001, 50, 9999);
        out.write(buf, len);

        // 6. Delete order 1002
        len = build_order_delete(buf, 1, 1002);
        out.write(buf, len);
    }

    FeedHandler handler;
    int bbo_updates = 0;
    handler.set_bbo_callback([&bbo_updates](uint16_t, const std::string&, const BBO&) {
        bbo_updates++;
    });

    handler.process_file(filename);

    // Verify processing statistics
    const FeedStats& stats = handler.stats();
    EXPECT_EQ(stats.total_messages, 6u);
    EXPECT_EQ(stats.add_orders, 3u);
    EXPECT_EQ(stats.order_executions, 1u);
    EXPECT_EQ(stats.order_deletes, 1u);

    // Verify book state: orders 1001 (partially filled) and 1003 should remain
    const OrderBook* book = handler.get_book(1);
    ASSERT_NE(book, nullptr);
    EXPECT_EQ(book->order_count(), 2u);

    // Verify BBO
    BBO bbo = book->get_bbo();
    EXPECT_EQ(bbo.bid_price, 1500000u);
    EXPECT_EQ(bbo.bid_size, 50u);    // 100 - 50 executed
    EXPECT_EQ(bbo.ask_price, 1510000u);
    EXPECT_EQ(bbo.ask_size, 200u);

    // Verify BBO callback was invoked
    EXPECT_GT(bbo_updates, 0);

    // Clean up temp file
    std::remove(filename);
}
