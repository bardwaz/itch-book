#include "itch_parser.h"
#include "endian_utils.h"

namespace itch {

size_t ItchParser::message_size(char type_code) {
    switch (type_code) {
        case 'S': return 11; // 12 - 1 for type
        case 'R': return 38;
        case 'H': return 24;
        case 'A': return 35;
        case 'F': return 39;
        case 'E': return 30;
        case 'C': return 35;
        case 'X': return 22;
        case 'D': return 18;
        case 'U': return 34;
        case 'P': return 43;
        case 'Q': return 39;
        default: return 0;
    }
}

size_t ItchParser::parse_message(const char* buf, size_t len, Message& msg) const {
    if (len < 2) return 0; // Not enough bytes for length prefix

    uint16_t msg_length = read_be16(buf);
    size_t total_length = 2 + msg_length;

    if (len < total_length) return 0; // Not enough bytes for the full message

    if (msg_length == 0) return total_length; // Empty message body?

    const char* p = buf + 2;
    char type_code = *p++;
    
    switch (type_code) {
        case 'S': {
            SystemEvent m;
            m.stock_locate = read_be16(p); p += 2;
            m.tracking_number = read_be16(p); p += 2;
            m.timestamp = read_be48(p); p += 6;
            m.event_code = *p;
            msg = m;
            break;
        }
        case 'R': {
            StockDirectory m;
            m.stock_locate = read_be16(p); p += 2;
            m.tracking_number = read_be16(p); p += 2;
            m.timestamp = read_be48(p); p += 6;
            std::memcpy(m.stock.data(), p, 8); p += 8;
            m.market_category = *p++;
            m.financial_status = *p++;
            m.round_lot_size = read_be32(p); p += 4;
            m.round_lots_only = *p++;
            m.issue_classification = *p++;
            std::memcpy(m.issue_sub_type.data(), p, 2); p += 2;
            m.authenticity = *p++;
            m.short_sale_threshold = *p++;
            m.ipo_flag = *p++;
            m.luld_ref_price_tier = *p++;
            m.etp_flag = *p++;
            m.etp_leverage_factor = read_be32(p); p += 4;
            m.inverse_indicator = *p;
            msg = m;
            break;
        }
        case 'H': {
            StockTradingAction m;
            m.stock_locate = read_be16(p); p += 2;
            m.tracking_number = read_be16(p); p += 2;
            m.timestamp = read_be48(p); p += 6;
            std::memcpy(m.stock.data(), p, 8); p += 8;
            m.trading_state = *p++;
            m.reserved = *p++;
            std::memcpy(m.reason.data(), p, 4);
            msg = m;
            break;
        }
        case 'A': {
            AddOrder m;
            m.stock_locate = read_be16(p); p += 2;
            m.tracking_number = read_be16(p); p += 2;
            m.timestamp = read_be48(p); p += 6;
            m.order_ref = read_be64(p); p += 8;
            m.side = static_cast<Side>(*p++);
            m.shares = read_be32(p); p += 4;
            std::memcpy(m.stock.data(), p, 8); p += 8;
            m.price = read_be32(p);
            msg = m;
            break;
        }
        case 'F': {
            AddOrderMPID m;
            m.stock_locate = read_be16(p); p += 2;
            m.tracking_number = read_be16(p); p += 2;
            m.timestamp = read_be48(p); p += 6;
            m.order_ref = read_be64(p); p += 8;
            m.side = static_cast<Side>(*p++);
            m.shares = read_be32(p); p += 4;
            std::memcpy(m.stock.data(), p, 8); p += 8;
            m.price = read_be32(p); p += 4;
            std::memcpy(m.attribution.data(), p, 4);
            msg = m;
            break;
        }
        case 'E': {
            OrderExecuted m;
            m.stock_locate = read_be16(p); p += 2;
            m.tracking_number = read_be16(p); p += 2;
            m.timestamp = read_be48(p); p += 6;
            m.order_ref = read_be64(p); p += 8;
            m.executed_shares = read_be32(p); p += 4;
            m.match_number = read_be64(p);
            msg = m;
            break;
        }
        case 'C': {
            OrderExecutedPrice m;
            m.stock_locate = read_be16(p); p += 2;
            m.tracking_number = read_be16(p); p += 2;
            m.timestamp = read_be48(p); p += 6;
            m.order_ref = read_be64(p); p += 8;
            m.executed_shares = read_be32(p); p += 4;
            m.match_number = read_be64(p); p += 8;
            m.printable = *p++;
            m.execution_price = read_be32(p);
            msg = m;
            break;
        }
        case 'X': {
            OrderCancel m;
            m.stock_locate = read_be16(p); p += 2;
            m.tracking_number = read_be16(p); p += 2;
            m.timestamp = read_be48(p); p += 6;
            m.order_ref = read_be64(p); p += 8;
            m.cancelled_shares = read_be32(p);
            msg = m;
            break;
        }
        case 'D': {
            OrderDelete m;
            m.stock_locate = read_be16(p); p += 2;
            m.tracking_number = read_be16(p); p += 2;
            m.timestamp = read_be48(p); p += 6;
            m.order_ref = read_be64(p);
            msg = m;
            break;
        }
        case 'U': {
            OrderReplace m;
            m.stock_locate = read_be16(p); p += 2;
            m.tracking_number = read_be16(p); p += 2;
            m.timestamp = read_be48(p); p += 6;
            m.original_order_ref = read_be64(p); p += 8;
            m.new_order_ref = read_be64(p); p += 8;
            m.shares = read_be32(p); p += 4;
            m.price = read_be32(p);
            msg = m;
            break;
        }
        case 'P': {
            Trade m;
            m.stock_locate = read_be16(p); p += 2;
            m.tracking_number = read_be16(p); p += 2;
            m.timestamp = read_be48(p); p += 6;
            m.order_ref = read_be64(p); p += 8;
            m.side = static_cast<Side>(*p++);
            m.shares = read_be32(p); p += 4;
            std::memcpy(m.stock.data(), p, 8); p += 8;
            m.price = read_be32(p); p += 4;
            m.match_number = read_be64(p);
            msg = m;
            break;
        }
        case 'Q': {
            CrossTrade m;
            m.stock_locate = read_be16(p); p += 2;
            m.tracking_number = read_be16(p); p += 2;
            m.timestamp = read_be48(p); p += 6;
            m.shares = read_be64(p); p += 8;
            std::memcpy(m.stock.data(), p, 8); p += 8;
            m.cross_price = read_be32(p); p += 4;
            m.match_number = read_be64(p); p += 8;
            m.cross_type = *p;
            msg = m;
            break;
        }
        default:
            // For unknown message types, we still consume the bytes
            // based on the length prefix, so we don't break the feed.
            // We just don't populate `msg` with anything meaningful.
            break;
    }

    return total_length;
}

} // namespace itch
