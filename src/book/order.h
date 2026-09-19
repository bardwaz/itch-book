#pragma once
#include <cstdint>
#include "parser/itch_messages.h"  // For Side enum

namespace itch {

// Represents a single order in the book.
// Part of an intrusive doubly-linked list at each price level.
struct Order {
    uint64_t order_ref = 0;       // ITCH order reference number
    uint32_t price = 0;           // Price in fixed-point (4 decimal places, raw ITCH value)
    uint32_t shares = 0;          // Remaining shares
    Side     side = Side::Buy;
    uint64_t timestamp = 0;       // Nanoseconds since midnight
    uint16_t stock_locate = 0;    // Which book this order belongs to

    // Intrusive doubly-linked list pointers for price-time priority queue
    Order* prev = nullptr;
    Order* next = nullptr;
};

} // namespace itch
