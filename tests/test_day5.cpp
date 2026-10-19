#include <cassert>
#include <iostream>
#include <random>
#include <vector>
#include "order_book.hpp"

#ifdef NDEBUG
#error "These tests rely on assert(); do not build them with NDEBUG."
#endif

using Ids    = std::vector<OrderId>;
using Trades = std::vector<Trade>;

static Order mk(OrderId id, Side side, Price price, Qty qty) {
    return Order{id, price, 0, qty, side};
}
static Trade T(OrderId buy, OrderId sell, Price price, Qty qty) {
    return Trade{buy, sell, price, qty};
}
static bool same(const Trade& a, const Trade& b) {
    return a.buy_id == b.buy_id && a.sell_id == b.sell_id &&
           a.price == b.price && a.qty == b.qty;
}
static bool same(const Trades& a, const Trades& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (!same(a[i], b[i])) return false;
    }
    return true;
}

static void test_no_cross_rests() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 5));
    auto t = b.add_limit(mk(2, Side::Buy, 101, 5));
    assert(t.empty());
    assert(b.best_bid() == 101);
    assert(b.best_ask() == 102);
    assert(b.order_count() == 2);
}

static void test_exact_fill_clears_book() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 5));
    auto t = b.add_limit(mk(2, Side::Buy, 102, 5));   // equal price crosses
    assert((same(t, Trades{T(2, 1, 102, 5)})));
    assert(b.order_count() == 0);
    assert(b.bid_levels() == 0);
    assert(b.ask_levels() == 0);
}

static void test_trade_at_resting_price_buy_side() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 5));
    auto t = b.add_limit(mk(2, Side::Buy, 105, 5));   // willing to pay 105, pays 102
    assert((same(t, Trades{T(2, 1, 102, 5)})));
}

static void test_trade_at_resting_price_sell_side() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 101, 5));
    auto t = b.add_limit(mk(2, Side::Sell, 99, 5));   // willing to take 99, gets 101
    assert((same(t, Trades{T(1, 2, 101, 5)})));
}

static void test_incoming_smaller_than_resting() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 5));
    auto t = b.add_limit(mk(2, Side::Buy, 102, 3));
    assert((same(t, Trades{T(2, 1, 102, 3)})));
    assert(b.contains(1));
    assert(!b.contains(2));
    assert(b.qty_at(Side::Sell, 102) == 2);
    assert(b.bid_levels() == 0);
}

static void test_incoming_larger_rests_remainder_at_own_price() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 3));
    auto t = b.add_limit(mk(2, Side::Buy, 104, 5));
    assert((same(t, Trades{T(2, 1, 102, 3)})));
    assert(b.ask_levels() == 0);
    assert(b.best_bid() == 104);                      // rests at ITS price, not 102
    assert(b.qty_at(Side::Buy, 104) == 2);
    assert(b.contains(2));
    assert(!b.contains(1));
}

static void test_time_priority_within_level() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 5));
    b.add_limit(mk(2, Side::Sell, 102, 5));
    auto t = b.add_limit(mk(3, Side::Buy, 102, 5));
    assert((same(t, Trades{T(3, 1, 102, 5)})));       // oldest first
    assert((b.ids_at(Side::Sell, 102) == Ids{2}));
}

static void test_price_priority_beats_arrival() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 103, 5));           // arrived first, worse price
    b.add_limit(mk(2, Side::Sell, 102, 5));           // arrived later, better price
    auto t = b.add_limit(mk(3, Side::Buy, 103, 5));
    assert((same(t, Trades{T(3, 2, 102, 5)})));
    assert(b.contains(1));
}

static void test_filled_order_leaves_index() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 5));
    b.add_limit(mk(2, Side::Buy, 102, 5));
    assert(!b.contains(1));
    assert(!b.cancel(1));                             // already filled
    b.add_limit(mk(1, Side::Buy, 100, 5));            // id 1 is free to reuse
    assert(b.order_count() == 1);
}

static void test_exhausted_level_is_erased() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 5));
    b.add_limit(mk(2, Side::Sell, 103, 5));
    b.add_limit(mk(3, Side::Buy, 102, 5));
    assert(b.ask_levels() == 1);
    assert(b.best_ask() == 103);
}

// The six orders of your Day 1 hand simulation. Do your own by hand FIRST.
static void test_day1_simulation() {
    OrderBook b;
    Trades t;

    t = b.add_limit(mk(1, Side::Buy, 100, 10));
    assert(t.empty());
    t = b.add_limit(mk(2, Side::Buy, 101, 5));
    assert(t.empty());
    t = b.add_limit(mk(3, Side::Sell, 103, 8));
    assert(t.empty());
    t = b.add_limit(mk(4, Side::Sell, 102, 4));
    assert(t.empty());

    t = b.add_limit(mk(5, Side::Sell, 101, 6));
    assert((same(t, Trades{T(2, 5, 101, 5)})));
    assert(b.qty_at(Side::Sell, 101) == 1);           // leftover 1 rests as an ask

    t = b.add_limit(mk(6, Side::Buy, 103, 3));
    assert((same(t, Trades{T(6, 5, 101, 1), T(6, 4, 102, 2)})));

    assert(b.best_bid() == 100);
    assert(b.qty_at(Side::Buy, 100) == 10);
    assert(b.bid_levels() == 1);
    assert(b.ask_levels() == 2);
    assert(b.qty_at(Side::Sell, 102) == 2);
    assert(b.qty_at(Side::Sell, 103) == 8);
    assert(!b.contains(5));
}

// 3000 random orders in a tight price band so lots of them cross.
// After EVERY order: the book must not be crossed.
// At the end: quantity is conserved (every trade removes qty from both sides).
static void test_random_never_crossed_and_conserves_quantity() {
    OrderBook b;
    std::mt19937 rng(777);
    long long submitted = 0;
    long long traded = 0;

    for (int i = 1; i <= 3000; ++i) {
        Side side  = (rng() % 2 == 0) ? Side::Buy : Side::Sell;
        Price price = 95 + static_cast<Price>(rng() % 11);   // 95..105
        Qty qty     = static_cast<Qty>(1 + rng() % 10);

        Trades t = b.add_limit(mk(static_cast<OrderId>(i), side, price, qty));
        submitted += qty;
        for (const Trade& tr : t) {
            assert(tr.qty > 0);
            assert(tr.price >= 95 && tr.price <= 105);
            traded += tr.qty;
        }
        if (b.best_bid() && b.best_ask()) {
            assert(*b.best_bid() < *b.best_ask());           // never crossed
        }
    }

    long long resting = 0;
    for (Price p = 90; p <= 110; ++p) {
        resting += b.qty_at(Side::Buy, p) + b.qty_at(Side::Sell, p);
    }
    assert(submitted == 2 * traded + resting);
    assert(traded > 0);                                      // the test really exercised matching
}

int main() {
    test_no_cross_rests();                                   std::cout << "ok no_cross_rests\n";
    test_exact_fill_clears_book();                           std::cout << "ok exact_fill_clears_book\n";
    test_trade_at_resting_price_buy_side();                  std::cout << "ok trade_at_resting_price_buy_side\n";
    test_trade_at_resting_price_sell_side();                 std::cout << "ok trade_at_resting_price_sell_side\n";
    test_incoming_smaller_than_resting();                    std::cout << "ok incoming_smaller_than_resting\n";
    test_incoming_larger_rests_remainder_at_own_price();     std::cout << "ok incoming_larger_rests_remainder_at_own_price\n";
    test_time_priority_within_level();                       std::cout << "ok time_priority_within_level\n";
    test_price_priority_beats_arrival();                     std::cout << "ok price_priority_beats_arrival\n";
    test_filled_order_leaves_index();                        std::cout << "ok filled_order_leaves_index\n";
    test_exhausted_level_is_erased();                        std::cout << "ok exhausted_level_is_erased\n";
    test_day1_simulation();                                  std::cout << "ok day1_simulation\n";
    test_random_never_crossed_and_conserves_quantity();      std::cout << "ok random_never_crossed_and_conserves_quantity\n";
    std::cout << "All Day 5 tests passed\n";
}
