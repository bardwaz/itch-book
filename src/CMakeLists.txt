# itch-book library
add_library(itch-book-lib STATIC
    parser/itch_parser.cpp
    book/order_book.cpp
    feed/feed_handler.cpp
)

target_include_directories(itch-book-lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(itch-book-lib PUBLIC
    tsl::robin_map
    ZLIB::ZLIB
)

# itch-book executable
add_executable(itch-book main.cpp)
target_link_libraries(itch-book PRIVATE itch-book-lib)
