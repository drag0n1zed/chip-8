cmake_minimum_required(VERSION 4.0)
project(chip-8)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(cpp-terminal)

add_executable(chip-8
        src/main.cpp
        src/Chip8.cpp
)

target_include_directories(chip-8 PUBLIC include)

target_link_libraries(chip-8 PRIVATE cpp-terminal::cpp-terminal)