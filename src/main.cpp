#include <iostream>


#include "order_book.hpp"

static Order make(OrderId id, Side side, Price price, Qty qty) {
    return Order{id, price, 0, qty, side};
}
int main() {
    OrderBook book;
// The first four orders of your Day 1 simulation (none of them cross)
    book.add_limit(make(1, Side::Buy,  100, 10));
    book.add_limit(make(2, Side::Buy,  101, 5));
    book.add_limit(make(3, Side::Sell, 103, 8));
    book.add_limit(make(4, Side::Sell, 102, 4));
    book.print_book();

    // Same prices again: they must queue BEHIND the existing orders
    book.add_limit(make(5, Side::Buy,  100, 7));
    book.add_limit(make(6, Side::Sell, 102, 2));
    book.print_book();
}    





