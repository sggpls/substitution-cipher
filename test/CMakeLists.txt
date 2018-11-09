cmake_minimum_required(VERSION 2.8)

project(test_cipher)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS} "../src")

add_executable(test_cipher test_cipher.cpp)
target_link_libraries (test_cipher ${GTEST_LIBRARIES} pthread)
