#pragma once

#include <cstdint>
#include <array>
#include <variant>
#include <string>
#include <algorithm>

namespace itch {

enum class MessageType : char {
    SystemEvent = 'S',
    StockDirectory = 'R',
    StockTradingAction = 'H',
    AddOrder = 'A',
    AddOrderMPID = 'F',
    OrderExecuted = 'E',
    OrderExecutedPrice = 'C',
    OrderCancel = 'X',
    OrderDelete = 'D',
    OrderReplace = 'U',
    Trade = 'P',
    CrossTrade = 'Q',
    Unknown = '?'
};

enum class Side : char {
    Buy = 'B',
    Sell = 'S'
};

// Converts space-padded stock tickers to a clean string
inline std::string stock_to_string(const std::array<char, 8>& stock) {
    auto it = std::find_if(stock.rbegin(), stock.rend(), [](char c) { return c != ' '; });
    if (it == stock.rend()) return "";
    return std::string(stock.begin(), it.base());
}

// Below are plain, non-packed structs representing the business logic contents
// of each ITCH message. Deserialization handles the packing and byte-order manually.

struct SystemEvent {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char event_code;
};

struct StockDirectory {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    std::array<char, 8> stock;
    char market_category;
    char financial_status;
    uint32_t round_lot_size;
    char round_lots_only;
    char issue_classification;
    std::array<char, 2> issue_sub_type;
    char authenticity;
    char short_sale_threshold;
    char ipo_flag;
    char luld_ref_price_tier;
    char etp_flag;
    uint32_t etp_leverage_factor;
    char inverse_indicator;
};

struct StockTradingAction {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    std::array<char, 8> stock;
    char trading_state;
    char reserved;
    std::array<char, 4> reason;
};

struct AddOrder {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_ref;
    Side side;
    uint32_t shares;
    std::array<char, 8> stock;
    uint32_t price; // 4 decimal places
};

struct AddOrderMPID {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_ref;
    Side side;
    uint32_t shares;
    std::array<char, 8> stock;
    uint32_t price; // 4 decimal places
    std::array<char, 4> attribution;
};

struct OrderExecuted {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_ref;
    uint32_t executed_shares;
    uint64_t match_number;
};

struct OrderExecutedPrice {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_ref;
    uint32_t executed_shares;
    uint64_t match_number;
    char printable;
    uint32_t execution_price; // 4 decimal places
};

struct OrderCancel {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_ref;
    uint32_t cancelled_shares;
};

struct OrderDelete {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_ref;
};

struct OrderReplace {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t original_order_ref;
    uint64_t new_order_ref;
    uint32_t shares;
    uint32_t price; // 4 decimal places
};

struct Trade {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_ref;
    Side side;
    uint32_t shares;
    std::array<char, 8> stock;
    uint32_t price; // 4 decimal places
    uint64_t match_number;
};

struct CrossTrade {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t shares;
    std::array<char, 8> stock;
    uint32_t cross_price; // 4 decimal places
    uint64_t match_number;
    char cross_type;
};

// Type-safe variant wrapping all parsed messages
using Message = std::variant<
    SystemEvent,
    StockDirectory,
    StockTradingAction,
    AddOrder,
    AddOrderMPID,
    OrderExecuted,
    OrderExecutedPrice,
    OrderCancel,
    OrderDelete,
    OrderReplace,
    Trade,
    CrossTrade
>;

} // namespace itch
