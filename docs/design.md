# Design Notes

## Overview
The order book stores all resting (unmatched) buy and sell orders. It matches
incoming orders using price-time priority: better price first, and at equal
price, earlier arrival first. Trades execute at the resting order's price.

## Types
- Price: int64 in ticks (not currency units), so price comparisons and map keys
  are exact. Doubles cannot represent most decimals exactly.
- Qty: uint32. Unsigned, so never subtract without checking (use std::min).
- Order members are ordered to minimise padding: sizeof(Order) = 32.

## Containers and why

### bids_ and asks_: std::map<Price, std::list<Order>>
- Orders are grouped by price level. The map keeps levels sorted by price, so
  the best level is always begin() in O(1).
- asks_ uses the default ascending order (lowest price = best ask).
- bids_ uses std::greater<Price> so the highest price comes first, which means
  begin() is the best level on both sides and matching code looks the same.
- find/insert/erase of a level cost O(log P), where P is the number of
  distinct price levels (P is much smaller than N, the number of orders).
- Caveat: operator[] creates a missing key. Use find() for lookups that must
  not create a level, otherwise empty levels break the invariants.

### One std::list<Order> per level (the FIFO queue)
- push_back for new orders, front() / pop_front() for matching: both O(1).
- erase(iterator) removes from the middle in O(1) (needed for cancel).
- List iterators stay valid when other elements are inserted or erased, so we
  can safely store an iterator in the index and use it later.
- Known weakness: every node is a separate heap allocation (malloc cost and
  poor cache locality). This is a Week 5 optimisation target, to be measured.

### index_: std::unordered_map<OrderId, Locator>
- Locator = { side, price, list iterator }.
- Lets cancel jump from an order ID to its exact place without scanning.
- Hash lookup is O(1) on average. Rehashing the table does not invalidate the
  list iterators stored inside Locators.

### Rejected alternatives
- std::vector per level: erasing from the middle is O(n) and invalidates
  stored iterators.
- std::priority_queue: cannot remove an arbitrary order and does not
  guarantee FIFO among equal prices.
- Single sorted list of all orders: finding a level requires scanning.
- unordered_map<Price, Level>: hash tables are unsorted, so finding the best
  price would require scanning all keys.
- Storing only total quantity per level: loses time priority and the ability
  to cancel individual orders.

## Complexity targets (P = number of price levels)
| Operation            | Cost                                                         |
|----------------------|--------------------------------------------------------------|
| Best bid / best ask  | O(1) via begin()                                             |
| Add resting order    | O(log P): find/create level, then O(1) push_back + hash insert |
| Cancel               | O(log P): O(1) hash lookup, O(log P) level lookup by price, O(1) list erase |
| One fill in matching | about O(1); O(log P) extra when a level is exhausted and erased |

Possible Week 5 improvement: store a Level* in Locator so cancel avoids the
O(log P) level lookup. Only do it if profiling shows it matters.

## Invariants (must always be true)
1. best bid < best ask after every operation
2. No empty price levels are stored
3. index_ contains exactly the orders currently resting in the book
4. Every resting order has qty > 0

## Out of scope for v1
Stop orders, iceberg orders, multiple symbols, persistence.








# Design Notes

## Overview
(2-3 sentences: what the order book does.)

## Types
- Price: int64 in ticks because : no float avoid confusion
- Qty: uint32 because : no negative stock
- Order field order chosen to minimise padding: sizeof(Order) = ...32

## Containers and why
- bids_ / asks_: ... greater price, lowest price begin()
- std::list per level: fifo O(1)
- index_: unordered_map 
- Rejected alternatives: vector O(1) invalid priority_queue no fifo

## Complexity targets
(table from Part 3)

## Invariants (must always be true)
1. best bid < best ask after every operation
2. No empty price levels are stored
3. index_ contains exactly the orders currently resting in the book
4. Every resting order has qty > 0

## Out of scope for v1
Stop orders, iceberg orders, multiple symbols, persistence.

## Matching rules (Days 5-6)
1. An incoming BUY crosses a resting ask when buy.price >= ask.price. An incoming SELL
   crosses a resting bid when sell.price <= bid.price. Equal prices cross.
2. Best price matches first. Within a price level, the oldest order matches first.
3. Every trade executes at the resting order's price.
4. A large order sweeps levels in price order and stops at its own limit price.
5. A partially filled resting order keeps its position in the queue.
6. Any unfilled remainder rests at the incoming order's own limit price.
7. Fully filled orders are removed from the book and from index_ (their ids can be reused).
8. Zero-quantity and duplicate-id orders are ignored with no effect on the book.
9. The book is never crossed after any operation.

## Testing approach
- Hand-written tests cover each rule above.
- A model-based test compares the engine with a naive linear-scan engine on random
  adds and cancels, requiring identical trades and identical books.
- Property checks: the book is never crossed, and quantity is conserved.
