cmake_minimum_required(VERSION 4.0)
project(chip-8)

set(CMAKE_CXX_STANDARD 20)

find_package(Curses REQUIRED)

include_directories(${CURSES_INCLUDE_DIRS}, include)

add_executable(chip-8
        src/main.cpp
        src/Chip8.cpp
)

target_link_libraries(chip-8 ${CURSES_LIBRARIES})