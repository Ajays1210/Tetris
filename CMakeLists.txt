cmake_minimum_required(VERSION 4.1)
project(Tetris)

set(CMAKE_CXX_STANDARD 17)

add_executable(Tetris
        main.cpp
        Game.h
        Game_init.cpp
        Game_Logic.cpp
        Game_Render.cpp
        Game_Input.cpp
)

if(WIN32)
    target_link_libraries(Tetris)
else()
    target_link_libraries(Tetris)
endif()