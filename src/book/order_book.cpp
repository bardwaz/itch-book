#include "order_book.h"
#include <stdexcept>

namespace itch {

OrderBook::OrderBook() {
    bid_levels_.resize(MAX_PRICE_LEVELS);
    ask_levels_.resize(MAX_PRICE_LEVELS);
}

void OrderBook::add_order(uint64_t order_ref, Side side, uint32_t price,
                          uint32_t shares, uint64_t timestamp) {
    if (price >= MAX_PRICE_LEVELS) {
        return; // Ignore orders outside supported price range
    }

    Order* order = pool_.allocate();
    if (!order) {
        return; // Out of memory in pool
    }

    order->order_ref = order_ref;
    order->side = side;
    order->price = price;
    order->shares = shares;
    order->timestamp = timestamp;

    order_map_[order_ref] = order;

    if (side == Side::Buy) {
        bid_levels_[price].push_back(order);
        if (price > best_bid_) {
            best_bid_ = price;
        }
    } else {
        ask_levels_[price].push_back(order);
        if (price < best_ask_) {
            best_ask_ = price;
        }
    }
}

void OrderBook::remove_order_from_book(Order* order) {
    if (order->side == Side::Buy) {
        bid_levels_[order->price].remove(order);
        if (bid_levels_[order->price].empty() && order->price == best_bid_) {
            update_best_bid_after_remove(order->price);
        }
    } else {
        ask_levels_[order->price].remove(order);
        if (ask_levels_[order->price].empty() && order->price == best_ask_) {
            update_best_ask_after_remove(order->price);
        }
    }
}

void OrderBook::update_best_bid_after_remove(uint32_t removed_price) {
    for (int32_t p = removed_price; p > 0; --p) {
        if (!bid_levels_[p].empty()) {
            best_bid_ = p;
            return;
        }
    }
    best_bid_ = 0;
}

void OrderBook::update_best_ask_after_remove(uint32_t removed_price) {
    for (uint32_t p = removed_price; p < MAX_PRICE_LEVELS; ++p) {
        if (!ask_levels_[p].empty()) {
            best_ask_ = p;
            return;
        }
    }
    best_ask_ = MAX_PRICE_LEVELS;
}

void OrderBook::execute_order(uint64_t order_ref, uint32_t shares) {
    auto it = order_map_.find(order_ref);
    if (it == order_map_.end()) return;
    
    Order* order = it->second;
    uint32_t exec_shares = std::min(order->shares, shares);
    order->shares -= exec_shares;
    
    if (order->side == Side::Buy) {
        bid_levels_[order->price].total_shares -= exec_shares;
    } else {
        ask_levels_[order->price].total_shares -= exec_shares;
    }

    if (order->shares == 0) {
        remove_order_from_book(order);
        order_map_.erase(it);
        pool_.deallocate(order);
    }
}

void OrderBook::cancel_order(uint64_t order_ref, uint32_t shares) {
    execute_order(order_ref, shares); // Logic is identical to execute_order
}

void OrderBook::delete_order(uint64_t order_ref) {
    auto it = order_map_.find(order_ref);
    if (it == order_map_.end()) return;

    Order* order = it->second;
    remove_order_from_book(order);
    order_map_.erase(it);
    pool_.deallocate(order);
}

void OrderBook::replace_order(uint64_t old_ref, uint64_t new_ref,
                              uint32_t new_price, uint32_t new_shares, uint64_t timestamp) {
    auto it = order_map_.find(old_ref);
    if (it != order_map_.end()) {
        Side side = it->second->side;
        delete_order(old_ref);
        add_order(new_ref, side, new_price, new_shares, timestamp);
    }
}

BBO OrderBook::get_bbo() const {
    BBO bbo;
    if (best_bid_ > 0) {
        bbo.bid_price = best_bid_;
        bbo.bid_size = bid_levels_[best_bid_].total_shares;
    }
    if (best_ask_ < MAX_PRICE_LEVELS) {
        bbo.ask_price = best_ask_;
        bbo.ask_size = ask_levels_[best_ask_].total_shares;
    }
    return bbo;
}

std::vector<DepthLevel> OrderBook::get_depth(int levels) const {
    std::vector<DepthLevel> depth;
    depth.reserve(levels * 2);

    int count = 0;
    for (int32_t p = best_bid_; p > 0 && count < levels; --p) {
        if (!bid_levels_[p].empty()) {
            depth.push_back({(uint32_t)p, bid_levels_[p].total_shares, bid_levels_[p].order_count});
            count++;
        }
    }

    count = 0;
    for (uint32_t p = best_ask_; p < MAX_PRICE_LEVELS && count < levels; ++p) {
        if (!ask_levels_[p].empty()) {
            depth.push_back({p, ask_levels_[p].total_shares, ask_levels_[p].order_count});
            count++;
        }
    }

    return depth;
}

size_t OrderBook::order_count() const {
    return order_map_.size();
}

bool OrderBook::empty() const {
    return order_map_.empty();
}

} // namespace itch
