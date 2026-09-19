#pragma once
#include "order.h"
#include "price_level.h"
#include "order_pool.h"
#include <tsl/robin_map.h>
#include <vector>
#include <cstdint>
#include <optional>
#include <algorithm>

namespace itch {

// Best Bid and Offer
struct BBO {
    uint32_t bid_price = 0;
    uint32_t bid_size = 0;
    uint32_t ask_price = 0;
    uint32_t ask_size = 0;
};

// Depth level for market depth queries
struct DepthLevel {
    uint32_t price = 0;
    uint32_t total_shares = 0;
    uint16_t order_count = 0;
};

// Maximum price levels. ITCH prices have 4 decimal places.
// We'll use 2,000,000 as max (covers $200.00 = 200 * 10000 ticks for most stocks).
constexpr uint32_t MAX_PRICE_LEVELS = 2'000'000;

class OrderBook {
public:
    OrderBook();

    // Order operations
    void add_order(uint64_t order_ref, Side side, uint32_t price,
                   uint32_t shares, uint64_t timestamp);
    void execute_order(uint64_t order_ref, uint32_t shares);
    void cancel_order(uint64_t order_ref, uint32_t shares);
    void delete_order(uint64_t order_ref);
    void replace_order(uint64_t old_ref, uint64_t new_ref,
                       uint32_t new_price, uint32_t new_shares, uint64_t timestamp);

    // Queries
    BBO get_bbo() const;
    std::vector<DepthLevel> get_depth(int levels) const;

    // Stats
    size_t order_count() const;
    bool empty() const;

private:
    std::vector<PriceLevel> bid_levels_;
    std::vector<PriceLevel> ask_levels_;

    uint32_t best_bid_ = 0;                    
    uint32_t best_ask_ = MAX_PRICE_LEVELS;     

    tsl::robin_map<uint64_t, Order*> order_map_;

    OrderPool pool_;

    void remove_order_from_book(Order* order);
    void update_best_bid_after_remove(uint32_t removed_price);
    void update_best_ask_after_remove(uint32_t removed_price);
};

} // namespace itch
