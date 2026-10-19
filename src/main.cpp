
#include <iostream>
#include <vector>
#include "order_book.hpp"

static Order make(OrderId id, Side side, Price price, Qty qty) {
    return Order{id, price, 0, qty, side};
}

static void submit(OrderBook& book, int n, Order o) {
    std::cout << "\n>>> order " << n << ": "
              << (o.side == Side::Buy ? "BUY " : "SELL ")
              << o.qty << " @ " << o.price << "\n";
    std::vector<Trade> trades = book.add_limit(o);
    if (trades.empty()) std::cout << "trades: none\n";
    for (const Trade& t : trades) {
        std::cout << "TRADE: buyer #" << t.buy_id << ", seller #" << t.sell_id
                  << ", " << t.qty << " @ " << t.price << "\n";
    }
    book.print_book();
}

int main() {
    OrderBook book;
    submit(book, 1, make(1, Side::Buy,  100, 10));
    submit(book, 2, make(2, Side::Buy,  101, 5));
    submit(book, 3, make(3, Side::Sell, 103, 8));
    submit(book, 4, make(4, Side::Sell, 102, 4));
    submit(book, 5, make(5, Side::Sell, 101, 6));
    submit(book, 6, make(6, Side::Buy,  103, 3));
}
