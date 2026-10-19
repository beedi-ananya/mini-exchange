#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
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
// T(buyer, seller, price, qty)
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

// ---------------------------------------------------------------- sweeps

static void test_three_level_sweep() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 4));
    b.add_limit(mk(2, Side::Sell, 103, 3));
    b.add_limit(mk(3, Side::Sell, 104, 5));
    Trades t = b.add_limit(mk(4, Side::Buy, 105, 10));
    assert((same(t, Trades{T(4, 1, 102, 4), T(4, 2, 103, 3), T(4, 3, 104, 3)})));
    assert(b.ask_levels() == 1);
    assert(b.best_ask() == 104);
    assert(b.qty_at(Side::Sell, 104) == 2);
    assert(b.bid_levels() == 0);                 // fully filled, nothing rests
    assert(!b.contains(1));
    assert(!b.contains(2));
    assert(b.contains(3));
    assert(!b.contains(4));
}

static void test_sweep_then_rest_remainder() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 4));
    b.add_limit(mk(2, Side::Sell, 103, 3));
    Trades t = b.add_limit(mk(3, Side::Buy, 103, 10));
    assert((same(t, Trades{T(3, 1, 102, 4), T(3, 2, 103, 3)})));
    assert(b.ask_levels() == 0);
    assert(b.best_bid() == 103);                 // rests at its OWN limit price
    assert(b.qty_at(Side::Buy, 103) == 3);
    assert(b.contains(3));
}

static void test_limit_price_caps_the_sweep() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 2));
    b.add_limit(mk(2, Side::Sell, 103, 3));
    b.add_limit(mk(3, Side::Sell, 104, 5));
    Trades t = b.add_limit(mk(4, Side::Buy, 103, 8));
    assert((same(t, Trades{T(4, 1, 102, 2), T(4, 2, 103, 3)})));   // never touches 104
    assert(b.best_bid() == 103);
    assert(b.qty_at(Side::Buy, 103) == 3);
    assert(b.best_ask() == 104);
    assert(b.qty_at(Side::Sell, 104) == 5);
}

static void test_sell_sweeps_bids() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 101, 2));
    b.add_limit(mk(2, Side::Buy, 100, 5));
    b.add_limit(mk(3, Side::Buy, 99, 4));
    Trades t = b.add_limit(mk(4, Side::Sell, 99, 9));
    assert((same(t, Trades{T(1, 4, 101, 2), T(2, 4, 100, 5), T(3, 4, 99, 2)})));
    assert(b.bid_levels() == 1);
    assert(b.qty_at(Side::Buy, 99) == 2);
    assert(b.ask_levels() == 0);
}

// ---------------------------------------------------------- partial fills

static void test_partial_fill_keeps_queue_position() {
    OrderBook b;
    b.add_limit(mk(1, Side::Buy, 100, 5));
    b.add_limit(mk(2, Side::Buy, 100, 5));

    Trades t = b.add_limit(mk(3, Side::Sell, 100, 2));
    assert((same(t, Trades{T(1, 3, 100, 2)})));
    assert((b.ids_at(Side::Buy, 100) == Ids{1, 2}));   // order 1 is STILL first
    assert(b.qty_at(Side::Buy, 100) == 8);

    t = b.add_limit(mk(4, Side::Sell, 100, 4));
    assert((same(t, Trades{T(1, 4, 100, 3), T(2, 4, 100, 1)})));
    assert((b.ids_at(Side::Buy, 100) == Ids{2}));
    assert(b.qty_at(Side::Buy, 100) == 4);
}

static void test_partially_filled_order_can_be_cancelled() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 5));
    Trades t = b.add_limit(mk(2, Side::Buy, 102, 2));
    assert((same(t, Trades{T(2, 1, 102, 2)})));
    assert(b.contains(1));
    assert(b.cancel(1));
    assert(b.ask_levels() == 0);
    assert(b.order_count() == 0);
}

static void test_fifo_across_a_partial_sweep_of_one_level() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 2));
    b.add_limit(mk(2, Side::Sell, 102, 2));
    b.add_limit(mk(3, Side::Sell, 102, 2));
    Trades t = b.add_limit(mk(4, Side::Buy, 102, 5));
    assert((same(t, Trades{T(4, 1, 102, 2), T(4, 2, 102, 2), T(4, 3, 102, 1)})));
    assert((b.ids_at(Side::Sell, 102) == Ids{3}));
    assert(b.qty_at(Side::Sell, 102) == 1);
}

static void test_exact_fills_of_several_orders() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 3));
    b.add_limit(mk(2, Side::Sell, 102, 3));
    Trades t = b.add_limit(mk(3, Side::Buy, 102, 6));
    assert((same(t, Trades{T(3, 1, 102, 3), T(3, 2, 102, 3)})));
    assert(b.order_count() == 0);
    assert(b.bid_levels() == 0);
    assert(b.ask_levels() == 0);
}

// ----------------------------------------------------------- edge cases

static void test_zero_qty_crossing_order_is_ignored() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 5));
    Trades t = b.add_limit(mk(2, Side::Buy, 105, 0));   // would cross if it had qty
    assert(t.empty());
    assert(b.qty_at(Side::Sell, 102) == 5);
    assert(b.order_count() == 1);
    assert(!b.contains(2));
    assert(b.bid_levels() == 0);
}

static void test_duplicate_id_crossing_order_is_ignored() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 5));
    b.add_limit(mk(2, Side::Buy, 100, 5));
    Trades t = b.add_limit(mk(2, Side::Buy, 105, 5));   // id 2 already exists
    assert(t.empty());
    assert(b.qty_at(Side::Sell, 102) == 5);             // nothing was consumed
    assert(b.order_count() == 2);
    assert(b.qty_at(Side::Buy, 100) == 5);
    assert(b.qty_at(Side::Buy, 105) == 0);
}

static void test_extreme_price_into_empty_side_rests() {
    OrderBook b;
    Trades t = b.add_limit(mk(1, Side::Buy, 1000000, 5));
    assert(t.empty());
    assert(b.best_bid() == 1000000);
    assert(!b.best_ask().has_value());
}

static void test_big_quantities_do_not_wrap() {
    {   // incoming smaller than resting
        OrderBook b;
        b.add_limit(mk(1, Side::Sell, 102, 4000000000u));
        Trades t = b.add_limit(mk(2, Side::Buy, 102, 3000000000u));
        assert((same(t, Trades{T(2, 1, 102, 3000000000u)})));
        assert(b.qty_at(Side::Sell, 102) == 1000000000u);
    }
    {   // incoming larger than resting
        OrderBook b;
        b.add_limit(mk(1, Side::Sell, 102, 3000000000u));
        Trades t = b.add_limit(mk(2, Side::Buy, 102, 4000000000u));
        assert((same(t, Trades{T(2, 1, 102, 3000000000u)})));
        assert(b.ask_levels() == 0);
        assert(b.qty_at(Side::Buy, 102) == 1000000000u);
    }
}

static void test_index_consistent_after_sweep() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 4));
    b.add_limit(mk(2, Side::Sell, 103, 3));
    b.add_limit(mk(3, Side::Sell, 104, 5));
    b.add_limit(mk(4, Side::Buy, 105, 10));
    assert(!b.cancel(1));                // filled, so cancel must fail
    assert(!b.cancel(2));
    assert(!b.cancel(4));
    assert(b.cancel(3));                 // partially filled leftover can be cancelled
    assert(b.order_count() == 0);
    assert(b.ask_levels() == 0);
}

static void test_level_recreated_after_exhaustion() {
    OrderBook b;
    b.add_limit(mk(1, Side::Sell, 102, 3));
    Trades t = b.add_limit(mk(2, Side::Buy, 102, 3));
    assert((same(t, Trades{T(2, 1, 102, 3)})));
    assert(b.ask_levels() == 0);

    b.add_limit(mk(5, Side::Sell, 102, 2));              // same price again
    assert(b.ask_levels() == 1);
    t = b.add_limit(mk(6, Side::Buy, 102, 1));
    assert((same(t, Trades{T(6, 5, 102, 1)})));
    assert(b.qty_at(Side::Sell, 102) == 1);
}

// ------------------------------------------- model-based comparison test
// A deliberately naive matching engine: a plain vector of resting orders,
// scanned linearly for the best crossing order every time. Too simple to be
// wrong, so any disagreement with OrderBook points at a bug in OrderBook.

struct M {
    OrderId       id;
    Side          side;
    Price         price;
    Qty           qty;
    std::uint64_t seq;
};

static Trades model_add(std::vector<M>& resting, std::uint64_t& next_seq,
                        OrderId id, Side side, Price price, Qty qty) {
    Trades trades;
    const std::uint64_t seq = next_seq++;
    while (qty > 0) {
        int best = -1;
        for (int i = 0; i < static_cast<int>(resting.size()); ++i) {
            const M& r = resting[i];
            if (r.side == side) continue;
            const bool crosses = (side == Side::Buy) ? (r.price <= price)
                                                     : (r.price >= price);
            if (!crosses) continue;
            if (best < 0) { best = i; continue; }
            const M& cur = resting[best];
            bool better;
            if (side == Side::Buy) {
                better = (r.price < cur.price) ||
                         (r.price == cur.price && r.seq < cur.seq);
            } else {
                better = (r.price > cur.price) ||
                         (r.price == cur.price && r.seq < cur.seq);
            }
            if (better) best = i;
        }
        if (best < 0) break;

        M& r = resting[best];
        const Qty fill = std::min(qty, r.qty);
        if (side == Side::Buy) trades.push_back(Trade{id, r.id, r.price, fill});
        else                   trades.push_back(Trade{r.id, id, r.price, fill});
        qty   -= fill;
        r.qty -= fill;
        if (r.qty == 0) resting.erase(resting.begin() + best);
    }
    if (qty > 0) resting.push_back(M{id, side, price, qty, seq});
    return trades;
}

static void check_same_book(const OrderBook& b, const std::vector<M>& resting) {
    assert(b.order_count() == resting.size());

    std::size_t bid_levels = 0, ask_levels = 0;
    for (Price p = 90; p <= 110; ++p) {
        for (int s = 0; s < 2; ++s) {
            const Side side = (s == 0) ? Side::Buy : Side::Sell;
            std::vector<const M*> at;
            for (const M& r : resting) {
                if (r.side == side && r.price == p) at.push_back(&r);
            }
            std::sort(at.begin(), at.end(),
                      [](const M* x, const M* y) { return x->seq < y->seq; });
            Ids expect;
            Qty total = 0;
            for (const M* r : at) { expect.push_back(r->id); total += r->qty; }

            assert(b.ids_at(side, p) == expect);       // same orders, same queue order
            assert(b.qty_at(side, p) == total);
            if (!at.empty()) { if (s == 0) ++bid_levels; else ++ask_levels; }
        }
    }
    assert(b.bid_levels() == bid_levels);              // no empty levels left behind
    assert(b.ask_levels() == ask_levels);

    bool has_bid = false, has_ask = false;
    Price best_bid = 0, best_ask = 0;
    for (const M& r : resting) {
        if (r.side == Side::Buy) {
            if (!has_bid || r.price > best_bid) { best_bid = r.price; has_bid = true; }
        } else {
            if (!has_ask || r.price < best_ask) { best_ask = r.price; has_ask = true; }
        }
    }
    assert(b.best_bid().has_value() == has_bid);
    if (has_bid) { assert(*b.best_bid() == best_bid); }
    assert(b.best_ask().has_value() == has_ask);
    if (has_ask) { assert(*b.best_ask() == best_ask); }
    if (has_bid && has_ask) { assert(best_bid < best_ask); }   // never crossed
}

static void test_matches_naive_model() {
    const unsigned seeds[] = {1u, 2u, 3u, 42u, 2024u};
    long long total_trades = 0;
    long long total_cancels = 0;

    for (unsigned seed : seeds) {
        OrderBook b;
        std::mt19937 rng(seed);
        std::vector<M> resting;
        std::uint64_t model_seq = 1;
        OrderId next_id = 1;

        for (int step = 1; step <= 5000; ++step) {
            if (next_id > 1 && rng() % 100 < 20) {
                // 20%: try to cancel a random id that was submitted earlier.
                // It may already be filled or cancelled, and both engines must agree.
                const OrderId id = static_cast<OrderId>(1 + rng() % (next_id - 1));
                auto it = std::find_if(resting.begin(), resting.end(),
                                       [id](const M& m) { return m.id == id; });
                const bool live = (it != resting.end());
                const bool ok = b.cancel(id);
                assert(ok == live);
                if (live) { resting.erase(it); ++total_cancels; }
            } else {
                const Side  side  = (rng() % 2 == 0) ? Side::Buy : Side::Sell;
                const Price price = 95 + static_cast<Price>(rng() % 11);   // 95..105
                const Qty   qty   = static_cast<Qty>(1 + rng() % 30);
                const OrderId id  = next_id++;

                Trades got      = b.add_limit(mk(id, side, price, qty));
                Trades expected = model_add(resting, model_seq, id, side, price, qty);
                assert(same(got, expected));            // identical trades, identical order
                total_trades += static_cast<long long>(got.size());
            }
            if (step % 250 == 0) check_same_book(b, resting);
        }
        check_same_book(b, resting);
    }
    assert(total_trades > 1000);     // the test really exercised matching
    assert(total_cancels > 100);     // ...and cancelling
}

int main() {
    test_three_level_sweep();                       std::cout << "ok three_level_sweep\n";
    test_sweep_then_rest_remainder();               std::cout << "ok sweep_then_rest_remainder\n";
    test_limit_price_caps_the_sweep();              std::cout << "ok limit_price_caps_the_sweep\n";
    test_sell_sweeps_bids();                        std::cout << "ok sell_sweeps_bids\n";
    test_partial_fill_keeps_queue_position();       std::cout << "ok partial_fill_keeps_queue_position\n";
    test_partially_filled_order_can_be_cancelled(); std::cout << "ok partially_filled_order_can_be_cancelled\n";
    test_fifo_across_a_partial_sweep_of_one_level();std::cout << "ok fifo_across_a_partial_sweep_of_one_level\n";
    test_exact_fills_of_several_orders();           std::cout << "ok exact_fills_of_several_orders\n";
    test_zero_qty_crossing_order_is_ignored();      std::cout << "ok zero_qty_crossing_order_is_ignored\n";
    test_duplicate_id_crossing_order_is_ignored();  std::cout << "ok duplicate_id_crossing_order_is_ignored\n";
    test_extreme_price_into_empty_side_rests();     std::cout << "ok extreme_price_into_empty_side_rests\n";
    test_big_quantities_do_not_wrap();              std::cout << "ok big_quantities_do_not_wrap\n";
    test_index_consistent_after_sweep();            std::cout << "ok index_consistent_after_sweep\n";
    test_level_recreated_after_exhaustion();        std::cout << "ok level_recreated_after_exhaustion\n";
    test_matches_naive_model();                     std::cout << "ok matches_naive_model\n";
    std::cout << "All Day 6 tests passed\n";
}
