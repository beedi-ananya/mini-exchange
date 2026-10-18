#include <iostream>


#include "order_book.hpp"

static Order make(OrderId id, Side side, Price price, Qty qty) {
    return Order{id, price, 0, qty, side};
}
int main() {
    std::cout << std::boolalpha;
    OrderBook book;

    book.add_limit(make(1, Side::Buy,  100, 10));
    book.add_limit(make(2, Side::Buy,  100, 7));
    book.add_limit(make(3, Side::Buy,  100, 4));
    book.add_limit(make(4, Side::Buy,   99, 5));
    book.add_limit(make(5, Side::Sell, 103, 8));
    book.add_limit(make(6, Side::Sell, 102, 6));
    std::cout << "Initial book:\n";
    book.print_book();

    std::cout << "\ncancel(2)   -> " << book.cancel(2)   << "   (middle of the 100 queue)\n";
    book.print_book();

    std::cout << "\ncancel(2)   -> " << book.cancel(2)   << "  (already gone)\n";
    std::cout << "cancel(999) -> "   << book.cancel(999) << "  (never existed)\n";

    std::cout << "\ncancel(4)   -> " << book.cancel(4)   << "   (only order at 99: level disappears)\n";
    std::cout << "cancel(6)   -> "   << book.cancel(6)   << "   (best ask moves 102 -> 103)\n";
    book.print_book();
}





