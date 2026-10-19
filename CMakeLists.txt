cmake_minimum_required(VERSION 3.14)
project(mini_exchange CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

# Core library: everything except main() and tests
add_library(book src/order_book.cpp)
target_include_directories(book PUBLIC include)

add_executable(engine src/main.cpp)
target_link_libraries(engine PRIVATE book)

enable_testing()

add_executable(test_day3 tests/test_day3.cpp)
target_link_libraries(test_day3 PRIVATE book)
add_test(NAME day3 COMMAND test_day3)

add_executable(test_day4 tests/test_day4.cpp)
target_link_libraries(test_day4 PRIVATE book)
add_test(NAME day4 COMMAND test_day4)

add_executable(test_day5 tests/test_day5.cpp)
target_link_libraries(test_day5 PRIVATE book)
add_test(NAME day5 COMMAND test_day5)

add_executable(test_day6 tests/test_day6.cpp)
target_link_libraries(test_day6 PRIVATE book)
add_test(NAME day6 COMMAND test_day6)
