#include <benchmark/benchmark.h>
#include "book/order_book.h"
#include "parser/itch_parser.h"
#include "parser/itch_messages.h"
#include "parser/endian_utils.h"
#include <random>
#include <vector>
#include <cstring>

using namespace itch;

// Helper functions to build binary messages for parser benchmarks
void write_be16(char* buf, uint16_t val) {
    val = __builtin_bswap16(val);
    std::memcpy(buf, &val, 2);
}

void write_be32(char* buf, uint32_t val) {
    val = __builtin_bswap32(val);
    std::memcpy(buf, &val, 4);
}

void write_be64(char* buf, uint64_t val) {
    val = __builtin_bswap64(val);
    std::memcpy(buf, &val, 8);
}

void write_be48(char* buf, uint64_t val) {
    uint64_t bswapped = __builtin_bswap64(val);
    std::memcpy(buf, reinterpret_cast<char*>(&bswapped) + 2, 6);
}

// 1. BM_OrderBook_AddOrder: Measures time to add a single order to an empty-ish book.
static void BM_OrderBook_AddOrder(benchmark::State& state) {
    state.PauseTiming();
    OrderBook book;
    uint64_t ref = 1;
    state.ResumeTiming();

    for (auto _ : state) {
        book.add_order(ref++, Side::Buy, 1500000, 100, 123456789);
    }
}
BENCHMARK(BM_OrderBook_AddOrder)->Unit(benchmark::kNanosecond);

// 2. BM_OrderBook_AddDelete: Full order lifecycle (add then immediately delete).
static void BM_OrderBook_AddDelete(benchmark::State& state) {
    state.PauseTiming();
    OrderBook book;
    uint64_t ref = 1;
    state.ResumeTiming();

    for (auto _ : state) {
        book.add_order(ref, Side::Buy, 1500000, 100, 123456789);
        book.delete_order(ref);
        ref++;
    }
}
BENCHMARK(BM_OrderBook_AddDelete)->Unit(benchmark::kNanosecond);

// 3. BM_OrderBook_ExecuteOrder: Add an order then partially execute it.
static void BM_OrderBook_ExecuteOrder(benchmark::State& state) {
    state.PauseTiming();
    OrderBook book;
    uint64_t ref = 1;
    state.ResumeTiming();

    for (auto _ : state) {
        book.add_order(ref, Side::Buy, 1500000, 100, 123456789);
        book.execute_order(ref, 50);
        ref++;
    }
}
BENCHMARK(BM_OrderBook_ExecuteOrder)->Unit(benchmark::kNanosecond);

// 4. BM_OrderBook_MixedWorkload: Realistic workload mix.
static void BM_OrderBook_MixedWorkload(benchmark::State& state) {
    state.PauseTiming();
    OrderBook book;
    std::mt19937 gen(42);
    std::uniform_int_distribution<uint32_t> price_dist(1490000, 1510000);
    std::uniform_int_distribution<uint32_t> size_dist(1, 100);
    std::uniform_int_distribution<int> type_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);

    std::vector<uint64_t> live_orders;
    live_orders.reserve(20000);

    uint64_t ref = 1;

    for (int i = 0; i < 10000; ++i) {
        Side side = side_dist(gen) == 0 ? Side::Buy : Side::Sell;
        book.add_order(ref, side, price_dist(gen), size_dist(gen) * 100, 123456789);
        live_orders.push_back(ref);
        ref++;
    }
    state.ResumeTiming();

    for (auto _ : state) {
        int op = type_dist(gen);
        if (op <= 60 || live_orders.empty()) {
            Side side = side_dist(gen) == 0 ? Side::Buy : Side::Sell;
            book.add_order(ref, side, price_dist(gen), size_dist(gen) * 100, 123456789);
            live_orders.push_back(ref);
            ref++;
        } else if (op <= 80) { // Execute full (simulated via delete)
            size_t idx = gen() % live_orders.size();
            uint64_t target = live_orders[idx];
            book.delete_order(target);
            live_orders[idx] = live_orders.back();
            live_orders.pop_back();
        } else if (op <= 90) { // Cancel partial
            size_t idx = gen() % live_orders.size();
            uint64_t target = live_orders[idx];
            book.cancel_order(target, 10);
        } else { // Delete
            size_t idx = gen() % live_orders.size();
            uint64_t target = live_orders[idx];
            book.delete_order(target);
            live_orders[idx] = live_orders.back();
            live_orders.pop_back();
        }
    }
}
BENCHMARK(BM_OrderBook_MixedWorkload)->Unit(benchmark::kNanosecond);

// 5. BM_OrderBook_GetBBO: BBO query after book is populated.
static void BM_OrderBook_GetBBO(benchmark::State& state) {
    state.PauseTiming();
    OrderBook book;
    std::mt19937 gen(42);
    std::uniform_int_distribution<uint32_t> price_dist(1490000, 1510000);
    uint64_t ref = 1;
    for (int i = 0; i < 10000; ++i) {
        book.add_order(ref++, Side::Buy, price_dist(gen), 100, 123456789);
        book.add_order(ref++, Side::Sell, price_dist(gen) + 20000, 100, 123456789);
    }
    state.ResumeTiming();

    for (auto _ : state) {
        BBO bbo = book.get_bbo();
        benchmark::DoNotOptimize(bbo);
    }
}
BENCHMARK(BM_OrderBook_GetBBO)->Unit(benchmark::kNanosecond);

// 6. BM_OrderBook_GetDepth: Depth query after book is populated.
static void BM_OrderBook_GetDepth(benchmark::State& state) {
    state.PauseTiming();
    OrderBook book;
    std::mt19937 gen(42);
    std::uniform_int_distribution<uint32_t> price_dist(1490000, 1510000);
    uint64_t ref = 1;
    for (int i = 0; i < 10000; ++i) {
        book.add_order(ref++, Side::Buy, price_dist(gen), 100, 123456789);
        book.add_order(ref++, Side::Sell, price_dist(gen) + 20000, 100, 123456789);
    }
    state.ResumeTiming();

    for (auto _ : state) {
        auto depth = book.get_depth(5);
        benchmark::DoNotOptimize(depth);
    }
}
BENCHMARK(BM_OrderBook_GetDepth)->Unit(benchmark::kNanosecond);

// 7. BM_Parser_AddOrder: Parse a single AddOrder message from a pre-built binary buffer.
static void BM_Parser_AddOrder(benchmark::State& state) {
    state.PauseTiming();
    ItchParser parser;
    char buf[38] = {0};
    write_be16(buf, 36); // Length
    buf[2] = 'A'; // Message Type
    write_be16(buf + 3, 1); // Stock Locate
    write_be16(buf + 5, 0); // Tracking Number
    write_be48(buf + 7, 123456789); // Timestamp
    write_be64(buf + 13, 12345); // Order Ref
    buf[21] = 'B'; // Side
    write_be32(buf + 22, 100); // Shares
    std::memcpy(buf + 26, "AAPL    ", 8); // Stock
    write_be32(buf + 34, 1500000); // Price
    
    Message msg;
    state.ResumeTiming();

    for (auto _ : state) {
        parser.parse_message(buf + 2, 36, msg);
        benchmark::DoNotOptimize(msg);
    }
}
BENCHMARK(BM_Parser_AddOrder)->Unit(benchmark::kNanosecond);

// 8. BM_Parser_MixedMessages: Parse a sequence of different message types.
static void BM_Parser_MixedMessages(benchmark::State& state) {
    state.PauseTiming();
    ItchParser parser;
    char buf[128] = {0};
    size_t offset = 0;
    
    // Add order (36 bytes)
    buf[offset] = 'A';
    offset += 36;
    
    // Execute order (31 bytes)
    buf[offset] = 'E';
    offset += 31;
    
    // Delete order (19 bytes)
    buf[offset] = 'D';
    offset += 19;
    
    // Replace order (35 bytes)
    buf[offset] = 'U';
    offset += 35;
    
    Message msg;
    state.ResumeTiming();

    for (auto _ : state) {
        parser.parse_message(buf, 36, msg);
        parser.parse_message(buf + 36, 31, msg);
        parser.parse_message(buf + 67, 19, msg);
        parser.parse_message(buf + 86, 35, msg);
        benchmark::DoNotOptimize(msg);
    }
}
BENCHMARK(BM_Parser_MixedMessages)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
