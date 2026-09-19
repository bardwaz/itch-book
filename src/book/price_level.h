#pragma once
#include "order.h"
#include <cstdint>

namespace itch {

// A single price level in the order book.
// Contains a doubly-linked list of orders, maintained in time priority (FIFO).
struct PriceLevel {
    uint32_t total_shares = 0;   // Aggregate quantity at this level
    uint16_t order_count = 0;    // Number of orders at this level
    Order* head = nullptr;       // Oldest order (first in queue)
    Order* tail = nullptr;       // Newest order (last in queue)

    // Add an order to the tail (preserves time priority)
    void push_back(Order* order) {
        order->next = nullptr;
        order->prev = tail;
        
        if (tail) {
            tail->next = order;
        } else {
            head = order;
        }
        tail = order;
        
        total_shares += order->shares;
        order_count++;
    }

    // Remove a specific order (O(1) given pointer)
    void remove(Order* order) {
        if (order->prev) {
            order->prev->next = order->next;
        } else {
            head = order->next;
        }
        
        if (order->next) {
            order->next->prev = order->prev;
        } else {
            tail = order->prev;
        }
        
        order->next = nullptr;
        order->prev = nullptr;
        
        total_shares -= order->shares;
        order_count--;
    }

    // Is this level empty?
    bool empty() const { return head == nullptr; }
};

} // namespace itch
