#include "order_book.hpp"

#include <cassert>
#include <iostream>
#include <iterator>

namespace {

// Prints one price level: "  102 : [#4 x4] [#6 x2]"
template <typename LevelT>
void print_level(Price price, const LevelT& level) {
    std::cout << "  " << price << " : ";
    bool first = true;
    for (const Order& o : level) {
        if (!first) std::cout << ' ';
        first = false;
        std::cout << "[#" << o.id << " x" << o.qty << "]";
    }
    std::cout << '\n';
}

// Removes one order from a side's map and deletes the price level if it
// becomes empty. Works for both bids_ and asks_ (different comparators).
template <typename SideMap>
void erase_resting(SideMap& side_map, Price price,
                   std::list<Order>::iterator it) {
    auto lvl = side_map.find(price);   // find(), never operator[]
    assert(lvl != side_map.end() && "index_ pointed at a missing level");
    lvl->second.erase(it);             // O(1): we hold the exact iterator
    if (lvl->second.empty()) {
        side_map.erase(lvl);           // keep invariant: no empty levels
    }
}

}  // namespace

std::vector<Trade> OrderBook::add_limit(Order order) {
    // Minimal guards. Day 9 replaces these with proper reject events.
    if (order.qty == 0 || index_.count(order.id) != 0) {
        return {};
    }

    order.seq = next_seq_++;

    // operator[] is intentional here: we WANT the level created if missing.
    Level& level = (order.side == Side::Buy) ? bids_[order.price]
                                             : asks_[order.price];

    level.push_back(order);
    auto it = std::prev(level.end());

    index_.emplace(order.id, Locator{order.side, order.price, it});
    return {};
}

bool OrderBook::cancel(OrderId id) {
    auto found = index_.find(id);
    if (found == index_.end()) {
        return false;                          // unknown or already gone
    }

    const Locator loc = found->second;         // COPY before we erase the entry

    if (loc.side == Side::Buy) {
        erase_resting(bids_, loc.price, loc.it);
    } else {
        erase_resting(asks_, loc.price, loc.it);
    }

    index_.erase(found);                       // loc.it is dead now; drop it
    return true;
}

std::optional<Price> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<Price> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::size_t OrderBook::order_count() const { return index_.size(); }
std::size_t OrderBook::bid_levels() const  { return bids_.size(); }
std::size_t OrderBook::ask_levels() const  { return asks_.size(); }

bool OrderBook::contains(OrderId id) const { return index_.count(id) != 0; }

std::vector<OrderId> OrderBook::ids_at(Side side, Price price) const {
    std::vector<OrderId> out;
    if (side == Side::Buy) {
        auto lvl = bids_.find(price);
        if (lvl != bids_.end()) {
            for (const Order& o : lvl->second) out.push_back(o.id);
        }
    } else {
        auto lvl = asks_.find(price);
        if (lvl != asks_.end()) {
            for (const Order& o : lvl->second) out.push_back(o.id);
        }
    }
    return out;
}

void OrderBook::print_book() const {
    std::cout << "======== ORDER BOOK ========\n";

    std::cout << "ASKS (highest -> lowest)\n";
    if (asks_.empty()) std::cout << "  (empty)\n";
    for (auto it = asks_.rbegin(); it != asks_.rend(); ++it) {
        print_level(it->first, it->second);
    }

    if (best_bid() && best_ask()) {
        Price spread = *best_ask() - *best_bid();
        std::cout << "---- spread: " << spread;
        if (spread <= 0) std::cout << " (CROSSED: matching not implemented yet)";
        std::cout << " ----\n";
    } else {
        std::cout << "----------------------------\n";
    }

    std::cout << "BIDS (highest -> lowest)\n";
    if (bids_.empty()) std::cout << "  (empty)\n";
    for (const auto& entry : bids_) {
        print_level(entry.first, entry.second);
    }

    std::cout << "============================\n";
}

