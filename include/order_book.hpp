#pragma once
#include <functional>
#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>
#include "types.hpp"

class OrderBook {
public:
    // Matches against the opposite side first; any remainder rests.
    // Returns the trades that were produced.
    std::vector<Trade> add_limit(Order order);

    // Cancels a resting order. Returns false if the id is unknown.
    bool cancel(OrderId id);

    std::optional<Price> best_bid() const;
    std::optional<Price> best_ask() const;

    void print_book() const;
 std::size_t order_count() const;
    std::size_t bid_levels() const;
    std::size_t ask_levels() const;
    bool contains(OrderId id) const;
    std::vector<OrderId> ids_at(Side side, Price price) const;  // queue order

private:
    using Level = std::list<Order>;

    struct Locator {
        Side   side;
        Price  price;
        Level::iterator it;
    };

    std::map<Price, Level, std::greater<Price>> bids_;
    std::map<Price, Level>                      asks_;
    std::unordered_map<OrderId, Locator>        index_;
     std::uint64_t                               next_seq_ = 1;
};
