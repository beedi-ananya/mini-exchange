#include <cassert>
#include <iostream>
#include <vector>
#include "order_book.hpp"

#ifdef NDEBUG
#error "These tests rely on assert(); do not build them with NDEBUG."
#endif

static Order mk(OrderId id, Side side, Price price, Qty qty) {
    return Order{id, price, 0, qty, side};
}

static void test_empty_book() {
    OrderBook b;
    assert(!b.best_bid().has_value());
    assert(!b.best_ask().has_value());
    assert(b.order_count() == 0);
}

static void test_single_buy_rests() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 100, 10));
    assert(b.best_bid() == 100);
    assert(!b.best_ask().has_value());
    assert(b.order_count() == 1);
}

static void test_best_bid_is_highest() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 100, 1));
    b.add_limit(mk(2, Side::Buy, 102, 1));
    b.add_limit(mk(3, Side::Buy, 101, 1));
    assert(b.best_bid() == 102);
}

static void test_best_ask_is_lowest() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 103, 1));
    b.add_limit(mk(2, Side::Sell, 101, 1));
    b.add_limit(mk(3, Side::Sell, 102, 1));
    assert(b.best_ask() == 101);
}

static void test_fifo_within_level() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 100, 5));
    b.add_limit(mk(2, Side::Buy, 100, 5));
    b.add_limit(mk(3, Side::Buy, 100, 5));
    // extra parentheses: the braces contain commas that would split assert's argument
    assert((b.ids_at(Side::Buy, 100) == std::vector<OrderId>{1, 2, 3}));
}

static void test_levels_are_counted() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 100, 5));
    b.add_limit(mk(2, Side::Buy, 100, 5));
    b.add_limit(mk(3, Side::Buy, 101, 5));
    assert(b.bid_levels() == 2);
    assert(b.ask_levels() == 0);
}

static void test_duplicate_id_ignored() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 100, 5));
    b.add_limit(mk(1, Side::Sell, 105, 5));
    assert(b.order_count() == 1);
    assert(!b.best_ask().has_value());
}

static void test_zero_qty_ignored() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 100, 0));
    assert(b.order_count() == 0);
    assert(b.bid_levels() == 0);   // and no empty level was created
}

static void test_add_returns_no_trades_yet() {
    OrderBook b;
    auto t = b.add_limit(mk(1, Side::Buy, 100, 5));
    assert(t.empty());
}

// TEMPORARY: documents today's behaviour (no matching).
// On Day 5 this must be deleted or inverted, because a crossing order will trade.
static void test_crossed_book_allowed_today() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 105, 5));
    b.add_limit(mk(2, Side::Sell, 100, 5));
    assert(b.order_count() == 2);
    assert(*b.best_bid() > *b.best_ask());
}

int main() {
    test_empty_book();                 std::cout << "ok empty_book\n";
    test_single_buy_rests();           std::cout << "ok single_buy_rests\n";
    test_best_bid_is_highest();        std::cout << "ok best_bid_is_highest\n";
    test_best_ask_is_lowest();         std::cout << "ok best_ask_is_lowest\n";
    test_fifo_within_level();          std::cout << "ok fifo_within_level\n";
    test_levels_are_counted();         std::cout << "ok levels_are_counted\n";
    test_duplicate_id_ignored();       std::cout << "ok duplicate_id_ignored\n";
    test_zero_qty_ignored();           std::cout << "ok zero_qty_ignored\n";
    test_add_returns_no_trades_yet();  std::cout << "ok add_returns_no_trades_yet\n";
    test_crossed_book_allowed_today(); std::cout << "ok crossed_book_allowed_today\n";
    std::cout << "All Day 3 tests passed\n";
}
