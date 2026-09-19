add_executable(test_parser test_parser.cpp)
target_link_libraries(test_parser PRIVATE itch-book-lib GTest::gtest_main)
add_test(NAME test_parser COMMAND test_parser)

# We can ignore test_order_book and test_feed_handler for now if they don't exist, 
# or keep them if they do.
add_executable(test_order_book test_order_book.cpp)
target_link_libraries(test_order_book PRIVATE itch-book-lib GTest::gtest_main)
add_test(NAME test_order_book COMMAND test_order_book)

add_executable(test_feed_handler test_feed_handler.cpp)
target_link_libraries(test_feed_handler PRIVATE itch-book-lib GTest::gtest_main)
add_test(NAME test_feed_handler COMMAND test_feed_handler)

include(GoogleTest)
gtest_discover_tests(test_parser)
gtest_discover_tests(test_order_book)
gtest_discover_tests(test_feed_handler)
