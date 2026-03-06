cmake_minimum_required(VERSION 3.10)
project(minidb VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for pedantic warnings and optimization
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -O3")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /O2")
endif()

# Core library
add_library(minidb src/MiniDB.cpp)
target_include_directories(minidb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# CLI application
add_executable(minidb_cli src/main.cpp)
target_link_libraries(minidb_cli PRIVATE minidb)

# Testing
enable_testing()
add_executable(test_minidb tests/test_minidb.cpp)
target_link_libraries(test_minidb PRIVATE minidb)
add_test(NAME test_minidb COMMAND test_minidb)
