#include "Chip8.h"

#include "cpp-terminal/terminal.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "\033[31mUsage: " << argv[0] << " <filepath>\033[0m" << std::endl;
        return 1;
    }
    std::string file_path = argv[1];

    std::cout << "Loading ROM..." << std::endl;
    Chip8 chip;
    if (!chip.LoadROM(argv[1])) {
        return 1;
    }

    std::cout << "Starting interpreter..." << std::endl;
    while (true) {
        for (auto _ = 10; _--;) {
            chip.Cycle();
        }
    }

    return 0;
}