#include "Chip8.h"

#include <charconv>
#include <cstring>
#include <fstream>
#include <ios>
#include <iosfwd>
#include <iostream>

Chip8::Chip8() {
    pc = 0x200; // 0x000 to 0x1FF are reserved for the interpreter
    for (auto i = 80; i--;) {
        memory[0x50 + i] = font_set[i];
    }
}

bool Chip8::LoadROM(char const *filename) {
    // Read file in binary mode and start the pointer in the end
    if (std::ifstream file(filename, std::ios::binary | std::ios::ate);
        file.is_open()) {

        // Read pointer location to find file size
        const std::streamsize size = file.tellg();

        // Make buffer of that size to place data
        const auto buffer = new char[size];

        // Move pointer back to the beginning
        file.seekg(0, std::ios::beg);

        // Read file into the buffer
        file.read(buffer, size);

        // Load buffer into memory
        for (auto i = size; i--;) {
            memory[0x200 + i] = buffer[i];
        }

        // Free buffer.
        delete[] buffer;
        return true;
    }
    std::cerr << "\033[31mError: Could not open file " << filename << "\033[0m"
              << std::endl;
    return false;
}

void Chip8::Cycle() {
    // Opcodes are 16 bits long: merge 2 bytes
    opcode = (memory[pc] << 8) | memory[pc + 1];
    // Increment by 2 bytes
    pc += 2;

    // The Switch-Case Monolith
    switch (opcode & 0xF000) {
        case 0x0000: {
            switch (opcode & 0x00FF) {
                case 0xE0: {
                    // 00E0: CLS (Clear the display)
                    std::memset(gfx, 0, sizeof(gfx));
                    break;
                }
                case 0xEE: {
                    // 00EE: RET (Return from a subroutine)
                    // TODO
                    break;
                }
                default: {
                    std::cerr << "\033[31mError: Unknown opcode 0x" << std::hex
                              << opcode << std::dec << "\033[0m" << std::endl;
                    break;
                }
            }
            break;
        }
        case 0x1000: {
            // 1nnn: JP addr (Jump to location nnn)
            pc = opcode & 0x0FFF;
            break;
        }
        case 0x6000: {
            // 6xkk: LD Vx, byte (Set Vx = kk)
            registers[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            break;
        }
        case 0xA000: {
            // Annn: LD I, addr (Set I = nnn)
            index = opcode & 0x0FFF;
            break;
        }
        case 0xD000: {
            // Dxyn: DRW Vx, Vy, nibble (Display n-byte sprite starting at
            // memory location I at (Vx, Vy), set VF = collision.)

            const uint8_t vx = (opcode & 0x0F00) >> 8;
            const uint8_t vy = (opcode & 0x00F0) >> 4;

            const uint8_t x = registers[vx];
            const uint8_t y = registers[vy];
            const uint8_t n = opcode & 0x000F;

            registers[0xF] = 0; // Set VF to 0

            for (auto row = 0; row < n; row++) {

                const uint8_t sprite = memory[index + row];

                for (auto col = 0; col < 8; col++) {
                    // 0x80 is 1000 0000, col = 0 -> checks leftmost
                    if (sprite & (0x80 >> col)) {

                        const auto screenX = (x + col) % 64; // Wrap
                        const auto screenY = (y + row) % 32; // Also wrap

                        if (gfx[screenY * 64 + screenX] == 1) {
                            registers[0xF] = 1; // Collision! Set VF to 1
                        }

                        // XOR: 0 ^ 0 = 0, 0 ^ 1 = 1, 1 ^ 0 = 1, 1 ^ 1 = 0
                        gfx[screenY * 64 + screenX] ^= 1;
                    }
                }
            }
            break;
        }
        default: {
            std::cerr << "\033[31mError: Unknown opcode 0x" << std::hex
                      << opcode << std::dec << "\033[0m" << std::endl;
            break;
        }
    }
}
