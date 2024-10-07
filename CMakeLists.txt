cmake_minimum_required(VERSION 3.16)
project(Orderbook)

set(CMAKE_CXX_STANDARD 20 )
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS OFF)


add_library(mylib lib/Orderbook.cpp)
target_include_directories(mylib PUBLIC lib)

add_executable(app app/main.cpp)
target_link_libraries(app mylib)


include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

add_executable(mytest test/test_orderbook.cpp)
target_link_libraries(mytest gtest_main mylib )

