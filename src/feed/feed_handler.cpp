#include "feed/feed_handler.h"
#include "utils/mmap_reader.h"
#include "utils/timer.h"
#include <cstdio>
#include <iostream>

namespace itch {

double FeedStats::messages_per_second() const {
    if (elapsed_seconds <= 0.0) return 0.0;
    return static_cast<double>(total_messages) / elapsed_seconds;
}

void FeedStats::print() const {
    std::printf("\n--- Processing Statistics ---\n");
    std::printf("Total Messages: %llu\n", (unsigned long long)total_messages);
    std::printf("Add Orders:     %llu\n", (unsigned long long)add_orders);
    std::printf("Executions:     %llu\n", (unsigned long long)order_executions);
    std::printf("Cancels:        %llu\n", (unsigned long long)order_cancels);
    std::printf("Deletes:        %llu\n", (unsigned long long)order_deletes);
    std::printf("Replaces:       %llu\n", (unsigned long long)order_replaces);
    std::printf("Trades:         %llu\n", (unsigned long long)trades);
    std::printf("Unknown/Other:  %llu\n", (unsigned long long)unknown_messages);
    std::printf("Elapsed Time:   %.3f seconds\n", elapsed_seconds);
    std::printf("Throughput:     %.0f msgs/sec\n", messages_per_second());
}

void FeedHandler::set_bbo_callback(BBOCallback cb) {
    bbo_callback_ = std::move(cb);
}

const FeedStats& FeedHandler::stats() const {
    return stats_;
}

const OrderBook* FeedHandler::get_book(uint16_t locate) const {
    auto it = books_.find(locate);
    if (it != books_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::string FeedHandler::get_symbol(uint16_t locate) const {
    auto it = symbol_map_.find(locate);
    if (it != symbol_map_.end()) {
        return it->second;
    }
    return "UNKNOWN";
}

void FeedHandler::process_file(const std::string& path) {
    MmapReader reader(path);
    if (!reader.data()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return;
    }

    const char* buf = reader.data();
    size_t len = reader.size();
    size_t offset = 0;
    Message msg;

    ScopedTimer timer("FeedHandler");

    while (offset < len) {
        size_t parsed = parser_.parse_message(buf + offset, len - offset, msg);
        if (parsed == 0) {
            break; // Error or incomplete message
        }
        dispatch(msg);
        offset += parsed;
    }

    stats_.elapsed_seconds = timer.elapsed_us() / 1000000.0;
}

void FeedHandler::dispatch(const Message& msg) {
    stats_.total_messages++;
    
    std::visit([this](const auto& m) {
        using T = std::decay_t<decltype(m)>;

        if constexpr (std::is_same_v<T, StockDirectory>) {
            std::string sym = stock_to_string(m.stock);
            symbol_map_[m.stock_locate] = sym;
            if (books_.find(m.stock_locate) == books_.end()) {
                books_[m.stock_locate] = std::make_unique<OrderBook>();
            }
        } 
        else if constexpr (std::is_same_v<T, AddOrder> || std::is_same_v<T, AddOrderMPID>) {
            stats_.add_orders++;
            if (books_.find(m.stock_locate) == books_.end()) {
                books_[m.stock_locate] = std::make_unique<OrderBook>();
            }
            auto& book = books_[m.stock_locate];
            book->add_order(m.order_ref, m.side, m.price, m.shares, m.timestamp);
            
            if (bbo_callback_) {
                bbo_callback_(m.stock_locate, get_symbol(m.stock_locate), book->get_bbo());
            }
        }
        else if constexpr (std::is_same_v<T, OrderExecuted> || std::is_same_v<T, OrderExecutedPrice>) {
            stats_.order_executions++;
            auto it = books_.find(m.stock_locate);
            if (it != books_.end()) {
                it->second->execute_order(m.order_ref, m.executed_shares);
                if (bbo_callback_) {
                    bbo_callback_(m.stock_locate, get_symbol(m.stock_locate), it->second->get_bbo());
                }
            }
        }
        else if constexpr (std::is_same_v<T, OrderCancel>) {
            stats_.order_cancels++;
            auto it = books_.find(m.stock_locate);
            if (it != books_.end()) {
                it->second->cancel_order(m.order_ref, m.cancelled_shares);
                if (bbo_callback_) {
                    bbo_callback_(m.stock_locate, get_symbol(m.stock_locate), it->second->get_bbo());
                }
            }
        }
        else if constexpr (std::is_same_v<T, OrderDelete>) {
            stats_.order_deletes++;
            auto it = books_.find(m.stock_locate);
            if (it != books_.end()) {
                it->second->delete_order(m.order_ref);
                if (bbo_callback_) {
                    bbo_callback_(m.stock_locate, get_symbol(m.stock_locate), it->second->get_bbo());
                }
            }
        }
        else if constexpr (std::is_same_v<T, OrderReplace>) {
            stats_.order_replaces++;
            auto it = books_.find(m.stock_locate);
            if (it != books_.end()) {
                it->second->replace_order(m.original_order_ref, m.new_order_ref, m.price, m.shares, m.timestamp);
                if (bbo_callback_) {
                    bbo_callback_(m.stock_locate, get_symbol(m.stock_locate), it->second->get_bbo());
                }
            }
        }
        else if constexpr (std::is_same_v<T, Trade> || std::is_same_v<T, CrossTrade>) {
            stats_.trades++;
        }
        else if constexpr (std::is_same_v<T, SystemEvent>) {
            // Log or ignore system events
        }
        else {
            stats_.unknown_messages++;
        }
    }, msg);
}

} // namespace itch
