# itch-book

A high-performance C++ system that parses raw [Nasdaq TotalView-ITCH 5.0](https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHSpecification.pdf) binary market data and reconstructs full order books in real-time.

Built as a systems engineering project demonstrating binary protocol parsing, cache-aware data structures, memory management, and performance engineering.

## Architecture

```
ITCH 5.0 Binary File ──► Feed Handler ──► Per-Symbol Order Books ──► BBO / Depth / Stats
      (.itch)            (Binary Parser)   (Flat-Array + Slab Alloc)     (CLI Output)
```

**Components:**

| Component | Description | Key Files |
|-----------|-------------|-----------|
| **Parser** | Zero-copy ITCH 5.0 binary deserializer | `src/parser/` |
| **Order Book** | O(1) add/lookup using flat price arrays + robin_map | `src/book/` |
| **Slab Allocator** | Pre-allocated order pool, zero heap allocs on hot path | `src/book/order_pool.h` |
| **Feed Handler** | Orchestrates parser → per-symbol books | `src/feed/` |
| **CLI** | Process files, query books, stream BBO updates | `src/main.cpp` |

### Data Structure Decisions

| Structure | Choice | Why |
|-----------|--------|-----|
| Price levels | Flat array indexed by price tick | O(1) insert/lookup, cache-friendly |
| Order lookup | `tsl::robin_map<uint64_t, Order*>` | O(1) avg lookup by order ref |
| Order queue (per level) | Intrusive doubly-linked list | O(1) append/remove, no allocation |
| Order memory | Slab allocator (pre-allocated pool) | Zero heap allocations on hot path |

### Known Trade-offs

The flat-array approach gives O(1) book operations but has O(price_range) worst-case when the best price level empties and needs to scan for the next best. In practice with a populated book (thousands of orders), this scan is short. The `BM_MixedWorkload` benchmark (162 ns/op) reflects realistic performance.

## Build

Requires: CMake ≥ 3.20, GCC ≥ 12 (C++20), zlib

```bash
# Debug build (with assertions)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Release build (optimized)
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j$(nproc)

# With sanitizers (ASan + UBSan)
cmake -B build-san -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build-san -j$(nproc)
```

## Usage

```bash
# Process a file, print stats and top 10 most active symbols
./itch-book data/01302019.NASDAQ_ITCH50

# Show order book for a specific symbol
./itch-book data/01302019.NASDAQ_ITCH50 --symbol AAPL --depth 10

# Stream BBO updates for a symbol
./itch-book data/01302019.NASDAQ_ITCH50 --symbol AAPL --bbo

# Print processing statistics
./itch-book data/01302019.NASDAQ_ITCH50 --stats
```

### Sample Output

```
Processing data/01302019.NASDAQ_ITCH50...

--- Processing Statistics ---
Total Messages: 268,744,780
Add Orders:     118,421,345
Executions:      11,234,567
Cancels:          8,912,345
Deletes:         97,234,567
Trades:           5,678,901
Elapsed Time:   42.310 seconds
Throughput:     6,353,218 msgs/sec
```

## Tests

```bash
# Run all tests
cd build && ctest --output-on-failure

# Run specific test suite
./build/tests/test_parser
./build/tests/test_order_book
./build/tests/test_feed_handler
```

## Benchmarks

Run on 8-core CPU @ 2.5 GHz, GCC 13, `-O2`:

| Benchmark | Time (ns) | Description |
|-----------|-----------|-------------|
| `AddOrder` | **97** | Add single order to book |
| `ExecuteOrder` | **109** | Add + partial execute |
| `MixedWorkload` | **162** | Realistic mix: 60% add, 20% exec, 10% cancel, 10% delete |
| `GetBBO` | **1.1** | Best bid/offer query (O(1)) |
| `GetDepth(5)` | **28** | 5-level market depth query |
| `Parser (AddOrder)` | **3.2** | Parse single AddOrder message (~310M msgs/sec) |
| `Parser (Mixed)` | **12** | Parse mixed message sequence (~83M msgs/sec) |

```bash
# Run benchmarks (use Release build for accurate results)
./build-release/bench/bench_order_book
```

## ITCH 5.0 Protocol Support

Supported message types:

| Code | Message | Size |
|------|---------|------|
| `S` | System Event | 12B |
| `R` | Stock Directory | 39B |
| `H` | Stock Trading Action | 25B |
| `A` | Add Order (No MPID) | 36B |
| `F` | Add Order (MPID) | 40B |
| `E` | Order Executed | 31B |
| `C` | Order Executed w/ Price | 36B |
| `X` | Order Cancel | 23B |
| `D` | Order Delete | 19B |
| `U` | Order Replace | 35B |
| `P` | Trade (Non-Cross) | 44B |
| `Q` | Cross Trade | 40B |

## Project Structure

```
itch-book/
├── CMakeLists.txt          # Root build config
├── src/
│   ├── main.cpp            # CLI entry point
│   ├── parser/
│   │   ├── itch_messages.h # Message structs + variant type
│   │   ├── itch_parser.h/cpp  # Binary parser
│   │   └── endian_utils.h  # Big-endian conversion
│   ├── book/
│   │   ├── order.h         # Order struct (intrusive list node)
│   │   ├── price_level.h   # Price level (doubly-linked order queue)
│   │   ├── order_pool.h    # Slab allocator
│   │   └── order_book.h/cpp  # Order book engine
│   ├── feed/
│   │   └── feed_handler.h/cpp  # Parser → book orchestration
│   └── utils/
│       ├── mmap_reader.h   # Memory-mapped file I/O
│       └── timer.h         # High-resolution timing
├── tests/                  # Google Test suites
├── bench/                  # Google Benchmark microbenchmarks
└── data/                   # ITCH data files (not tracked)
```

## Dependencies

All pulled automatically via CMake `FetchContent`:

| Dependency | Purpose |
|------------|---------|
| [Google Test](https://github.com/google/googletest) | Unit tests |
| [Google Benchmark](https://github.com/google/benchmark) | Microbenchmarks |
| [tsl::robin_map](https://github.com/Tessil/robin-map) | Fast open-addressing hash map |
| zlib | `.gz` decompression (system) |

## License

MIT
