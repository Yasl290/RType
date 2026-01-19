cmake_minimum_required(VERSION 3.20)

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(rtype_tests
    test_registry.cpp
    test_sparse_set.cpp
    test_components.cpp
    test_systems.cpp
    test_protocol.cpp
    test_game_module.cpp
    ${CMAKE_SOURCE_DIR}/server/src/GameModule.cpp
    ${CMAKE_SOURCE_DIR}/server/src/GameModule_Players.cpp
    ${CMAKE_SOURCE_DIR}/server/src/GameModule_Combat.cpp
    ${CMAKE_SOURCE_DIR}/server/src/levels/LevelManager.cpp
    ${CMAKE_SOURCE_DIR}/server/src/levels/Level1.cpp
    ${CMAKE_SOURCE_DIR}/server/src/levels/Level2.cpp 
)

target_link_libraries(rtype_tests
    engine
    gtest
    gtest_main
)

target_include_directories(rtype_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/engine/include
    ${CMAKE_SOURCE_DIR}/server/include
)

include(GoogleTest)
gtest_discover_tests(rtype_tests)