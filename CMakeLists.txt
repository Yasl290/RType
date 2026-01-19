cmake_minimum_required(VERSION 3.20)
project(RType VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}")

find_package(SFML REQUIRED COMPONENTS system window graphics audio network)

add_subdirectory(engine)
add_subdirectory(game)
add_subdirectory(server)
add_subdirectory(tests)