#include <gtest/gtest.h>
#include "../src/book/order_pool.h"
#include "../src/book/price_level.h"
#include "../src/book/order_book.h"

using namespace itch;

TEST(OrderPoolTest, AllocateAndDeallocate) {
    OrderPool pool(10);
    EXPECT_EQ(pool.capacity(), 10);
    EXPECT_EQ(pool.allocated(), 0);

    Order* o1 = pool.allocate();
    ASSERT_NE(o1, nullptr);
    EXPECT_EQ(pool.allocated(), 1);
    
    pool.deallocate(o1);
    EXPECT_EQ(pool.allocated(), 0);
}

TEST(OrderPoolTest, ExhaustPool) {
    OrderPool pool(2);
    Order* o1 = pool.allocate();
    Order* o2 = pool.allocate();
    Order* o3 = pool.allocate();
    
    EXPECT_NE(o1, nullptr);
    EXPECT_NE(o2, nullptr);
    EXPECT_EQ(o3, nullptr);
}

TEST(PriceLevelTest, PushAndRemove) {
    PriceLevel level;
    EXPECT_TRUE(level.empty());
    
    Order o1;
    o1.shares = 100;
    level.push_back(&o1);
    
    EXPECT_FALSE(level.empty());
    EXPECT_EQ(level.total_shares, 100);
    EXPECT_EQ(level.order_count, 1);
    
    level.remove(&o1);
    EXPECT_TRUE(level.empty());
    EXPECT_EQ(level.total_shares, 0);
}

TEST(PriceLevelTest, FIFOOrder) {
    PriceLevel level;
    Order o1, o2;
    o1.shares = 100; o2.shares = 200;
    
    level.push_back(&o1);
    level.push_back(&o2);
    
    EXPECT_EQ(level.head, &o1);
    EXPECT_EQ(level.tail, &o2);
}

TEST(OrderBookTest, AddOrder_UpdatesBBO) {
    OrderBook book;
    book.add_order(1, Side::Buy, 10000, 100, 0);
    
    BBO bbo = book.get_bbo();
    EXPECT_EQ(bbo.bid_price, 10000);
    EXPECT_EQ(bbo.bid_size, 100);
    EXPECT_EQ(bbo.ask_size, 0);
}

TEST(OrderBookTest, AddBidAndAsk) {
    OrderBook book;
    book.add_order(1, Side::Buy, 10000, 100, 0);
    book.add_order(2, Side::Sell, 10005, 200, 0);
    
    BBO bbo = book.get_bbo();
    EXPECT_EQ(bbo.bid_price, 10000);
    EXPECT_EQ(bbo.ask_price, 10005);
}

TEST(OrderBookTest, DeleteOrder_UpdatesBBO) {
    OrderBook book;
    book.add_order(1, Side::Buy, 10000, 100, 0);
    book.add_order(2, Side::Buy, 9995, 200, 0);
    book.delete_order(1);
    
    BBO bbo = book.get_bbo();
    EXPECT_EQ(bbo.bid_price, 9995);
}

TEST(OrderBookTest, ExecuteOrder_PartialFill) {
    OrderBook book;
    book.add_order(1, Side::Buy, 10000, 100, 0);
    book.execute_order(1, 40);
    
    BBO bbo = book.get_bbo();
    EXPECT_EQ(bbo.bid_size, 60);
}

TEST(OrderBookTest, ExecuteOrder_FullFill) {
    OrderBook book;
    book.add_order(1, Side::Buy, 10000, 100, 0);
    book.execute_order(1, 100);
    
    EXPECT_TRUE(book.empty());
    BBO bbo = book.get_bbo();
    EXPECT_EQ(bbo.bid_size, 0);
}

TEST(OrderBookTest, CancelOrder_PartialCancel) {
    OrderBook book;
    book.add_order(1, Side::Buy, 10000, 100, 0);
    book.cancel_order(1, 40);
    
    BBO bbo = book.get_bbo();
    EXPECT_EQ(bbo.bid_size, 60);
}

TEST(OrderBookTest, CancelOrder_FullCancel) {
    OrderBook book;
    book.add_order(1, Side::Buy, 10000, 100, 0);
    book.cancel_order(1, 100);
    
    EXPECT_TRUE(book.empty());
}

TEST(OrderBookTest, ReplaceOrder) {
    OrderBook book;
    book.add_order(1, Side::Buy, 10000, 100, 0);
    book.replace_order(1, 2, 10005, 200, 0);
    
    BBO bbo = book.get_bbo();
    EXPECT_EQ(bbo.bid_price, 10005);
    EXPECT_EQ(bbo.bid_size, 200);
}

TEST(OrderBookTest, GetDepth) {
    OrderBook book;
    book.add_order(1, Side::Buy, 10000, 100, 0);
    book.add_order(2, Side::Buy, 9995, 200, 0);
    
    auto depth = book.get_depth(2);
    ASSERT_EQ(depth.size(), 2);
    EXPECT_EQ(depth[0].price, 10000);
    EXPECT_EQ(depth[1].price, 9995);
}

TEST(OrderBookTest, EmptyBook) {
    OrderBook book;
    EXPECT_TRUE(book.empty());
}

TEST(OrderBookTest, MultipleOrdersSamePrice) {
    OrderBook book;
    book.add_order(1, Side::Buy, 10000, 100, 0);
    book.add_order(2, Side::Buy, 10000, 200, 0);
    
    BBO bbo = book.get_bbo();
    EXPECT_EQ(bbo.bid_size, 300);
}

TEST(OrderBookTest, StressTest_1000Orders) {
    OrderBook book;
    for (int i = 0; i < 1000; i++) {
        book.add_order(i, Side::Buy, 10000, 10, 0);
    }
    EXPECT_EQ(book.order_count(), 1000);
    BBO bbo = book.get_bbo();
    EXPECT_EQ(bbo.bid_size, 10000);
}
