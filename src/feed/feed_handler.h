#pragma once
#include "parser/itch_parser.h"
#include "book/order_book.h"
#include <string>
#include <unordered_map>
#include <array>
#include <functional>
#include <memory>

namespace itch {

// Processing statistics
struct FeedStats {
    uint64_t total_messages = 0;
    uint64_t add_orders = 0;
    uint64_t order_executions = 0;
    uint64_t order_cancels = 0;
    uint64_t order_deletes = 0;
    uint64_t order_replaces = 0;
    uint64_t trades = 0;
    uint64_t unknown_messages = 0;
    double elapsed_seconds = 0.0;

    double messages_per_second() const;
    void print() const;
};

// Callback type for BBO updates
using BBOCallback = std::function<void(uint16_t locate, const std::string& symbol, const BBO& bbo)>;

class FeedHandler {
public:
    // Process an entire ITCH file
    void process_file(const std::string& path);

    // Access a specific book by locate code
    const OrderBook* get_book(uint16_t locate) const;

    // Get symbol for a locate code
    std::string get_symbol(uint16_t locate) const;

    // Get processing stats
    const FeedStats& stats() const;

    // Set a callback for BBO updates (optional)
    void set_bbo_callback(BBOCallback cb);

private:
    ItchParser parser_;
    
    // Stock locate -> symbol mapping (built from StockDirectory messages)
    std::unordered_map<uint16_t, std::string> symbol_map_;

    // Per-stock order books, keyed by locate code.
    std::unordered_map<uint16_t, std::unique_ptr<OrderBook>> books_;

    FeedStats stats_;
    BBOCallback bbo_callback_;

    // Dispatch a parsed message to the appropriate book
    void dispatch(const Message& msg);
};

} // namespace itch
