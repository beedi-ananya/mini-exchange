#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <random>
#include <vector>
#include "order_book.hpp"

#ifdef NDEBUG
#error "These tests rely on assert(); do not build them with NDEBUG."
#endif

using Ids = std::vector<OrderId>;

static Order mk(OrderId id, Side side, Price price, Qty qty) {
    return Order{id, price, 0, qty, side};
}

static void test_cancel_first_middle_last() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 100, 5));
    b.add_limit(mk(2, Side::Buy, 100, 5));
    b.add_limit(mk(3, Side::Buy, 100, 5));

    assert(b.cancel(2));                                   // middle
    assert((b.ids_at(Side::Buy, 100) == Ids{1, 3}));

    assert(b.cancel(1));                                   // first
    assert((b.ids_at(Side::Buy, 100) == Ids{3}));

    assert(b.cancel(3));                                   // last (level empties)
    assert(b.ids_at(Side::Buy, 100).empty());
    assert(b.bid_levels() == 0);
    assert(!b.best_bid().has_value());
    assert(b.order_count() == 0);
}

static void test_cancel_unknown_id() {
    OrderBook b;
    assert(!b.cancel(42));                                 // empty book
    b.add_limit(mk(1, Side::Buy, 100, 5));
    assert(!b.cancel(2));                                  // wrong id
    assert(b.order_count() == 1);
}

static void test_cancel_twice() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 100, 5));
    assert(b.cancel(1));
    assert(!b.cancel(1));                                  // already gone
}

static void test_empty_level_removed_best_bid_moves() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 102, 5));
    b.add_limit(mk(2, Side::Buy, 101, 5));
    assert(b.best_bid() == 102);
    assert(b.cancel(1));
    assert(b.best_bid() == 101);
    assert(b.bid_levels() == 1);
}

static void test_sell_side() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 101, 5));
    b.add_limit(mk(2, Side::Sell, 102, 5));
    b.add_limit(mk(3, Side::Sell, 102, 5));
    assert(b.best_ask() == 101);
    assert(b.cancel(1));
    assert(b.best_ask() == 102);
    assert(b.cancel(2));
    assert((b.ids_at(Side::Sell, 102) == Ids{3}));
    assert(b.ask_levels() == 1);
}

static void test_other_levels_untouched() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 100, 5));
    b.add_limit(mk(2, Side::Buy, 100, 5));
    b.add_limit(mk(3, Side::Buy, 101, 5));
    assert(b.cancel(3));
    assert((b.ids_at(Side::Buy, 100) == Ids{1, 2}));
    assert(b.best_bid() == 100);
}

static void test_queue_order_after_cancel() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 100, 5));
    b.add_limit(mk(2, Side::Buy, 100, 5));
    b.add_limit(mk(3, Side::Buy, 100, 5));
    assert(b.cancel(2));
    b.add_limit(mk(4, Side::Buy, 100, 5));                 // joins the BACK
    assert((b.ids_at(Side::Buy, 100) == Ids{1, 3, 4}));
}

static void test_id_reuse_after_cancel() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 100, 5));
    assert(b.cancel(1));
    b.add_limit(mk(1, Side::Sell, 105, 5));                // same id, new order
    assert(b.order_count() == 1);
    assert(b.best_ask() == 105);
    assert(!b.best_bid().has_value());
}

static void test_contains() {
    OrderBook b;
    b.add_limit(mk(7, Side::Sell, 110, 5));
    assert(b.contains(7));
    assert(!b.contains(8));
    assert(b.cancel(7));
    assert(!b.contains(7));
}

// Compare the real book against a trivially-correct model (plain vectors in maps).
static void test_random_add_cancel_matches_model() {
    struct Live { OrderId id; int side; Price price; };

    OrderBook b;
    std::mt19937 rng(12345);                               // fixed seed = reproducible
    std::map<Price, Ids> model[2];                         // [0] = bids, [1] = asks
    std::vector<Live> live;
    OrderId next_id = 1;

    for (int i = 0; i < 2000; ++i) {
        int side = static_cast<int>(rng() % 2);
        Price price = (side == 0) ? 90 + static_cast<Price>(rng() % 10) : 101 + static_cast<Price>(rng() % 10);
        Qty qty = static_cast<Qty>(1 + rng() % 10);
        OrderId id = next_id++;
        b.add_limit(mk(id, side == 0 ? Side::Buy : Side::Sell, price, qty));
        model[side][price].push_back(id);
        live.push_back(Live{id, side, price});
    }

    for (int i = 0; i < 1200; ++i) {
        std::size_t pick = rng() % live.size();
        Live l = live[pick];
        live[pick] = live.back();
        live.pop_back();

        assert(b.cancel(l.id));
        Ids& ids = model[l.side][l.price];
        ids.erase(std::find(ids.begin(), ids.end(), l.id));
        if (ids.empty()) model[l.side].erase(l.price);
    }

    assert(b.order_count() == live.size());
    for (const Live& l : live) assert(b.contains(l.id));

    for (const auto& kv : model[0]) assert(b.ids_at(Side::Buy, kv.first) == kv.second);
    for (const auto& kv : model[1]) assert(b.ids_at(Side::Sell, kv.first) == kv.second);

    assert(b.bid_levels() == model[0].size());
    assert(b.ask_levels() == model[1].size());

    if (model[0].empty()) assert(!b.best_bid().has_value());
    else                  assert(b.best_bid() == model[0].rbegin()->first);
    if (model[1].empty()) assert(!b.best_ask().has_value());
    else                  assert(b.best_ask() == model[1].begin()->first);
}

int main() {
    test_cancel_first_middle_last();               std::cout << "ok cancel_first_middle_last\n";
    test_cancel_unknown_id();                      std::cout << "ok cancel_unknown_id\n";
    test_cancel_twice();                           std::cout << "ok cancel_twice\n";
    test_empty_level_removed_best_bid_moves();     std::cout << "ok empty_level_removed_best_bid_moves\n";
    test_sell_side();                              std::cout << "ok sell_side\n";
    test_other_levels_untouched();                 std::cout << "ok other_levels_untouched\n";
    test_queue_order_after_cancel();               std::cout << "ok queue_order_after_cancel\n";
    test_id_reuse_after_cancel();                  std::cout << "ok id_reuse_after_cancel\n";
    test_contains();                               std::cout << "ok contains\n";
    test_random_add_cancel_matches_model();        std::cout << "ok random_add_cancel_matches_model\n";
    std::cout << "All Day 4 tests passed\n";
}
