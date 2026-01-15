cmake_minimum_required(VERSION 3.10)
project(r-type)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include_directories(include)
include_directories(gamemodule/include)

add_executable(r-type_server
    src/main.cpp
    src/Server.cpp
    src/NetworkModule.cpp
    src/GameModule.cpp
)

target_include_directories(r-type_server PRIVATE
    ${CMAKE_SOURCE_DIR}/engine/include
)

target_link_libraries(r-type_server engine Threads::Threads)