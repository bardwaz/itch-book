#include "feed/feed_handler.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

using namespace itch;

// Helper to write big-endian values
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
    write_be16(buf, 39);  // message length
    buf[2] = 'R';         // type
    write_be16(buf+3, locate);
    write_be16(buf+5, 0); // tracking
    write_be48(buf+7, 1000ULL); // timestamp
    std::memcpy(buf+13, stock, 8);
    // fill rest with dummy data
    std::memset(buf+21, ' ', 18);
    return 41;
}

size_t build_add_order(char* buf, uint16_t locate, uint64_t order_ref,
                       char side, uint32_t shares, const char* stock, uint32_t price) {
    write_be16(buf, 36);  // message length
    buf[2] = 'A';         // type
    write_be16(buf+3, locate);
    write_be16(buf+5, 0); // tracking
    write_be48(buf+7, 1000000000ULL); // timestamp
    write_be64(buf+13, order_ref);
    buf[21] = side;
    write_be32(buf+22, shares);
    std::memcpy(buf+26, stock, 8);
    write_be32(buf+34, price);
    return 38; // 2 + 36
}

size_t build_order_executed(char* buf, uint16_t locate, uint64_t order_ref, uint32_t executed_shares, uint64_t match_number) {
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

void test_feed_handler() {
    std::cout << "Running FeedHandler integration tests..." << std::endl;
    
    // Create a temporary ITCH file
    const char* filename = "test_feed.itch";
    std::ofstream out(filename, std::ios::binary);
    
    char buf[1024];
    
    // 1. Stock Directory for AAPL
    size_t len = build_stock_directory(buf, 1, "AAPL    ");
    out.write(buf, len);
    
    // 2. Add Order Buy 100 @ $150
    len = build_add_order(buf, 1, 1001, 'B', 100, "AAPL    ", 1500000);
    out.write(buf, len);

    // 3. Add Order Buy 50 @ $150
    len = build_add_order(buf, 1, 1002, 'B', 50, "AAPL    ", 1500000);
    out.write(buf, len);

    // 4. Add Order Sell 200 @ $151
    len = build_add_order(buf, 1, 1003, 'S', 200, "AAPL    ", 1510000);
    out.write(buf, len);

    // 5. Execute 50 shares of order 1001
    len = build_order_executed(buf, 1, 1001, 50, 9999);
    out.write(buf, len);

    // 6. Delete order 1002
    len = build_order_delete(buf, 1, 1002);
    out.write(buf, len);

    out.close();

    FeedHandler handler;
    int bbo_updates = 0;
    handler.set_bbo_callback([&bbo_updates](uint16_t, const std::string&, const BBO&) {
        bbo_updates++;
    });

    handler.process_file(filename);

    // Verify stats
    const FeedStats& stats = handler.stats();
    assert(stats.total_messages == 6);
    assert(stats.add_orders == 3);
    assert(stats.order_executions == 1);
    assert(stats.order_deletes == 1);

    // Verify book state
    const OrderBook* book = handler.get_book(1);
    assert(book != nullptr);
    assert(book->order_count() == 2); // 1001 (partially executed) and 1003

    BBO bbo = book->get_bbo();
    assert(bbo.bid_price == 1500000);
    assert(bbo.bid_size == 50); // 100 - 50 executed
    assert(bbo.ask_price == 1510000);
    assert(bbo.ask_size == 200);

    assert(bbo_updates > 0);

    std::remove(filename);
    std::cout << "FeedHandler tests passed!" << std::endl;
}

int main() {
    test_feed_handler();
    return 0;
}
