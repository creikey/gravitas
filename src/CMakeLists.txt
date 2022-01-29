cmake_minimum_required(VERSION 3.0)
project(projectname CXX C)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Setting parameters for raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # or games

if (EMSCRIPTEN)
    set(CMAKE_BUILD_TYPE Release)
    get_filename_component(RESOURCES_PATH resources ABSOLUTE)
    get_filename_component(SHELL_PATH minshell.html ABSOLUTE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s WASM=1 -s ASYNCIFY --preload-file=${RESOURCES_PATH}@resources --shell-file ${SHELL_PATH}")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

add_subdirectory(raylib)

add_executable(projectname
        raylib_game.c
        screen_ending.c
        screen_gameplay.c
        screen_logo.c
        screen_options.c
        screen_title.c
)

target_link_libraries(projectname PRIVATE raylib)