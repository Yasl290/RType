add_executable(r-type_client
    src/Main.cpp
    src/GameClientHelpers.cpp
    src/Menu.cpp
    src/NetworkClient.cpp
    src/NetworkScoreProvider.cpp
    src/LocalScoreProvider.cpp
    src/CheatManager.cpp
    src/ui/PauseMenu.cpp
    src/ui/LobbyScreen.cpp
    src/ui/GameOverScreen.cpp
    src/ui/LevelTransition.cpp
    src/core/GameManager.cpp
    src/core/GameLoop.cpp
    src/core/MenuLoop.cpp
    src/core/InputState.cpp
    src/managers/NetworkEntityManager.cpp
    src/managers/EventHandler.cpp
    src/initialization/GameInitializer.cpp
    src/levels/LevelAssetManager.cpp
    src/ui/UpgradeMenu.cpp
    src/ui/EndlessGameOverScreen.cpp
    

)

target_include_directories(r-type_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/server/include
)

target_link_libraries(r-type_client PRIVATE
    engine
)

add_custom_command(TARGET r-type_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_BINARY_DIR}/assets
)
