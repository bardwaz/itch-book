#include "feed/feed_handler.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>

using namespace itch;

void print_usage() {
    std::cout << "Usage: itch-book <file.itch> [options]\n\n"
              << "Options:\n"
              << "  --symbol <SYM>    Show book for specific symbol (e.g., AAPL)\n"
              << "  --depth <N>       Number of price levels to show (default: 5)\n"
              << "  --bbo             Stream BBO updates to stdout\n"
              << "  --stats           Print processing statistics\n"
              << "  --help            Show this help message\n\n"
              << "Examples:\n"
              << "  ./itch-book data/01302019.NASDAQ_ITCH50\n"
              << "  ./itch-book data/01302019.NASDAQ_ITCH50 --symbol AAPL --depth 10\n"
              << "  ./itch-book data/01302019.NASDAQ_ITCH50 --stats\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string filename = argv[1];
    if (filename == "--help" || filename == "-h") {
        print_usage();
        return 0;
    }

    std::string target_symbol;
    int depth = 5;
    bool show_bbo = false;
    bool show_stats = false;

    // Parse options
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--symbol" && i + 1 < argc) {
            target_symbol = argv[++i];
        } else if (arg == "--depth" && i + 1 < argc) {
            depth = std::stoi(argv[++i]);
        } else if (arg == "--bbo") {
            show_bbo = true;
        } else if (arg == "--stats") {
            show_stats = true;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            print_usage();
            return 1;
        }
    }

    FeedHandler handler;

    if (show_bbo) {
        // Find locate for target symbol if specified
        handler.set_bbo_callback([target_symbol](uint16_t, const std::string& symbol, const BBO& bbo) {
            if (target_symbol.empty() || symbol == target_symbol) {
                // If BBO is invalid/empty, prices will be 0.
                if (bbo.bid_price > 0 || bbo.ask_price > 0) {
                    std::printf("[%s] BBO: %u @ %.4f | %.4f @ %u\n",
                                symbol.c_str(),
                                bbo.bid_size, bbo.bid_price / 10000.0,
                                bbo.ask_price / 10000.0, bbo.ask_size);
                }
            }
        });
    }

    std::cout << "Processing " << filename << "..." << std::endl;
    handler.process_file(filename);

    if (show_stats || (!show_bbo && target_symbol.empty())) {
        handler.stats().print();
    }

    if (!target_symbol.empty()) {
        // Find the locate code for the target symbol
        uint16_t target_locate = 0;
        bool found = false;
        
        // This is a bit inefficient for looking up by symbol, but fine for CLI
        // We iterate through a small range of possible locates (max 65535)
        for (uint16_t i = 1; i < 10000; ++i) {
            if (handler.get_symbol(i) == target_symbol) {
                target_locate = i;
                found = true;
                break;
            }
        }

        if (!found) {
            std::cerr << "Symbol " << target_symbol << " not found in data." << std::endl;
        } else {
            const OrderBook* book = handler.get_book(target_locate);
            if (book) {
                std::printf("\n--- Order Book for %s ---\n", target_symbol.c_str());
                BBO bbo = book->get_bbo();
                auto levels = book->get_depth(depth);
                for (const auto& level : levels) {
                    const char* side_str = (bbo.bid_price > 0 && level.price <= bbo.bid_price) ? "BID" : "ASK";
                    std::printf("%s: %u @ %.4f (%u orders)\n", 
                                side_str, level.total_shares, level.price / 10000.0, level.order_count);
                }
            } else {
                std::cout << "No orders for " << target_symbol << std::endl;
            }
        }
    } else if (!show_bbo && !show_stats) {
        // Default behavior: print top 10 most active symbols (by order count)
        // Note: For a real application, we might want to track activity differently
        // Here we just iterate through all created books
        std::vector<std::pair<std::string, size_t>> activity;
        for (uint16_t i = 1; i < 10000; ++i) {
            const OrderBook* book = handler.get_book(i);
            if (book && !book->empty()) {
                activity.push_back({handler.get_symbol(i), book->order_count()});
            }
        }

        std::sort(activity.begin(), activity.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

        std::cout << "\n--- Top 10 Most Active Symbols ---" << std::endl;
        for (size_t i = 0; i < std::min<size_t>(10, activity.size()); ++i) {
            const auto& sym = activity[i].first;
            // find locate
            uint16_t locate = 0;
            for (uint16_t j = 1; j < 10000; ++j) {
                if (handler.get_symbol(j) == sym) {
                    locate = j;
                    break;
                }
            }
            const OrderBook* book = handler.get_book(locate);
            if (book) {
                BBO bbo = book->get_bbo();
                std::printf("%-8s: %u orders, BBO: %u @ %.4f | %.4f @ %u\n",
                            sym.c_str(), (uint32_t)activity[i].second,
                            bbo.bid_size, bbo.bid_price / 10000.0,
                            bbo.ask_price / 10000.0, bbo.ask_size);
            }
        }
    }

    return 0;
}
