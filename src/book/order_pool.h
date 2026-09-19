#pragma once
#include "order.h"
#include <vector>
#include <stdexcept>

namespace itch {

// Slab allocator for Order objects.
// Pre-allocates a contiguous block of memory and manages a free list.
// All allocations and deallocations are O(1) with zero heap overhead on the hot path.
class OrderPool {
public:
    explicit OrderPool(size_t capacity = 10'000'000) : allocated_count_(0) {
        storage_.resize(capacity);
        if (capacity > 0) {
            free_head_ = &storage_[0];
            for (size_t i = 0; i < capacity - 1; ++i) {
                storage_[i].next = &storage_[i + 1];
            }
            storage_[capacity - 1].next = nullptr;
        }
    }

    // Allocate an order from the pool. Returns nullptr if pool is exhausted.
    Order* allocate() {
        if (!free_head_) {
            return nullptr;
        }
        Order* order = free_head_;
        free_head_ = free_head_->next;
        order->next = nullptr;
        order->prev = nullptr;
        allocated_count_++;
        return order;
    }

    // Return an order to the pool.
    void deallocate(Order* order) {
        if (!order) return;
        order->next = free_head_;
        free_head_ = order;
        allocated_count_--;
    }

    // Pool stats
    size_t capacity() const { return storage_.size(); }
    size_t allocated() const { return allocated_count_; }
    size_t available() const { return capacity() - allocated_count_; }

private:
    std::vector<Order> storage_;  // Contiguous block of orders
    Order* free_head_ = nullptr;  // Head of free list (uses next pointer)
    size_t allocated_count_ = 0;
};

} // namespace itch
