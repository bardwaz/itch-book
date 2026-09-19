add_executable(bench_order_book bench_order_book.cpp)
target_link_libraries(bench_order_book PRIVATE itch-book-lib benchmark::benchmark benchmark::benchmark_main)
