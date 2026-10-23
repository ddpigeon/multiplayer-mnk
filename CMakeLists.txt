cmake_minimum_required(VERSION 3.20)
project(Multiplayer-MNK CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags with cross-platform MSVC / GCC safety
if(MSVC)
    add_compile_options(/W4 /O2 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -O3 -march=native)
endif()

# Find Raylib dependency first
find_package(raylib REQUIRED)

# 1. Core Library (Pure Logic + Interfaces)
add_library(mnk_core STATIC
    src/core/board.cpp
    src/core/board.h
    src/core/game.cpp
    src/core/game.h
    src/core/match.cpp
    src/core/match.h
    src/core/launch_settings.cpp
    src/core/launch_settings.h
    src/core/player.h
)

target_include_directories(mnk_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 2. AI Library (MCTS + Bot Variants)
add_library(mnk_ai STATIC
    src/ai/mcts.cpp
    src/ai/mcts.h
    src/ai/trainer.cpp
    src/ai/trainer.h
    src/ai/ai_player_interface.h
    src/ai/10-10-4-3/ai_player.cpp
    src/ai/10-10-4-3/ai_player.h
)

target_include_directories(mnk_ai PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link AI to Core
target_link_libraries(mnk_ai PUBLIC
    mnk_core
)

# 3. UI Library (Raylib Renderers + Human Player)
add_library(mnk_ui STATIC
    src/ui/input.cpp
    src/ui/input.h
    src/ui/window.cpp
    src/ui/window.h
    src/ui/human_player.h
)

target_include_directories(mnk_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link UI to Core and Raylib
target_link_libraries(mnk_ui PUBLIC
    mnk_core
    raylib
)

# 4. Main Executable Wiring
add_executable(mnk_game src/main.cpp)

# Link Executable to UI, AI, and Core
target_link_libraries(mnk_game PRIVATE
    mnk_ui
    mnk_ai                   # <--- Linked AI library
    mnk_core
    raylib
)

