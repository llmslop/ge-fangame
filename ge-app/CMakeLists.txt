add_subdirectory(assets)

add_executable(ge-app)

target_sources(
    ge-app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-app/font.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-app/assets/bgm.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-app/scenes/scene.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-app/scenes/game_scene.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-app/scenes/menu_scene.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-app/scenes/gameover_scene.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-app/game/obstacle.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-app/ui/menu.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ge-app/font.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ge-app/game/water.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ge-app/assets/bgm.cpp
)

target_include_directories(ge-app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ge-app PRIVATE ge-hal ge-assets)
ge_hal_add_link_sources(ge-app)
