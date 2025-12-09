#include "Chip8.h"

#include <fstream>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <ncurses.h>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

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

    std::cerr << "Error: Could not open file " << filename << std::endl;
    return false;
}

void Chip8::HandleOpcode() {
    // Opcodes are 16 bits long: merge 2 bytes
    opcode = (memory[pc] << 8) | memory[pc + 1];
    // Increment by 2 bytes
    pc += 2;

    const uint16_t nnn = opcode & 0x0FFF;
    const uint8_t kk = opcode & 0x00FF;
    const uint8_t x = (opcode & 0x0F00) >> 8;
    const uint8_t y = (opcode & 0x00F0) >> 4;
    const uint8_t n = opcode & 0x000F;

    // The Switch-Case Monolith
    switch (opcode & 0xF000) {
        case 0x0000: {
            switch (kk) {
                case 0xE0: {
                    // 00E0: CLS (Clear the display)
                    std::memset(gfx, 0, sizeof(gfx));
                    draw_flag = true;
                    break;
                }
                case 0xEE: {
                    // 00EE: RET (Return from a subroutine)
                    pc = stack[--sp];
                    break;
                }
                default: {
                    // 0nnn: SYS addr (Jump to a machine code routine at nnn)
                    // Modern interpreters ignores this opcode.
                    break;
                }
            }
            break;
        }
        case 0x1000: {
            // 1nnn: JP addr (Jump to location nnn)
            pc = nnn;
            break;
        }
        case 0x2000: {
            // 2nnn: CALL addr (Call subroutine at nnn)
            stack[sp++] = pc;
            pc = nnn;
            break;
        }
        case 0x3000: {
            // 3xkk: SE Vx, byte (Skip next instruction if Vx = kk)
            if (v[x] == kk) {
                pc += 2;
            }
            break;
        }
        case 0x4000: {
            // 4xkk: SNE Vx, byte (Skip next instruction if Vx != kk)
            if (v[x] != kk) {
                pc += 2;
            }
            break;
        }
        case 0x5000: {
            // 5xy0 - SE Vx, Vy (Skip next instruction if Vx = Vy)
            if (v[x] == v[y]) {
                pc += 2;
            }
            break;
        }
        case 0x6000: {
            // 6xkk: LD Vx, byte (Set Vx = kk)
            v[x] = kk;
            break;
        }
        case 0x7000: {
            // 7xkk: ADD Vx, byte (Set Vx = Vx + kk)
            v[x] += kk;
            break;
        }
        case 0x8000: {
            switch (n) {
                case (0x0): {
                    // 8xy0: LD Vx, Vy (Set Vx = Vy)
                    v[x] = v[y];
                    break;
                }
                case (0x1): {
                    // 8xy1: OR Vx, Vy (Set Vx = Vx OR Vy)
                    v[x] |= v[y];
                    break;
                }
                case (0x2): {
                    // 8xy2: AND Vx, Vy (Set Vx = Vx AND Vy)
                    v[x] &= v[y];
                    break;
                }
                case (0x3): {
                    // 8xy3: XOR Vx, Vy (Set Vx = Vx XOR Vy)
                    v[x] ^= v[y];
                    break;
                }
                case (0x4): {
                    // 8xy4: ADD Vx, Vy (Set Vx = Vx + Vy, set VF = carry)
                    uint8_t val_x = v[x], val_y = v[y];
                    uint16_t sum = val_x + val_y;
                    v[x] = sum & 0xFF;
                    v[0xF] = sum > 0xFF;
                    break;
                }
                case (0x5): {
                    // 8xy5: SUB Vx, Vy (Set Vx = Vx - Vy, set VF = NOT borrow)
                    uint8_t val_y = v[y], val_x = v[x];
                    v[x] = val_x - val_y;
                    v[0xF] = val_x >= val_y;
                    break;
                }
                case (0x6): {
                    // 8xy6: SHR Vx {, Vy} (Set Vx = Vx SHR 1, set VF to the
                    // least significant bit of Vx before shift
                    // TODO: Config for quirk
                    bool quirk_8xy6 =
                        true; // TRUE for vX = vY >> 1, FALSE for vX = vX >> 1
                    uint8_t val = quirk_8xy6 ? v[y] : v[x];
                    v[x] = val >> 1;
                    v[0xF] = val & 0x01;
                    break;
                }
                case (0x7): {
                    // 8xy7: SUBN Vx, Vy (Set Vx = Vy - Vx, set VF = NOT borrow)
                    uint8_t val_y = v[y], val_x = v[x];
                    v[x] = val_y - val_x;
                    v[0xF] = val_y >= val_x;
                    break;
                }
                case (0xE): {
                    // 8xyE: SHL Vx {, Vy} (Set Vx = Vx SHL 1, set VF to the
                    // most significant bit of Vx before shift)
                    bool quirk_8xyE =
                        true; // TRUE for vX = vY >> 1, FALSE for vX = vX >> 1
                    uint8_t val = quirk_8xyE ? v[y] : v[x];
                    v[x] = (val << 1) & 0xFF;
                    v[0xF] = (val & 0x80) >> 7;
                    break;
                }
                default: {
                    std::stringstream ss;
                    ss << "Unknown opcode [0x8000 family]: 0x" << std::hex
                       << opcode;
                    throw std::runtime_error(ss.str());
                }
            }
            break;
        }
        case 0x9000: {
            // 9xy0: SNE Vx, Vy (Skip next instruction if Vx != Vy)
            if (v[x] != v[y]) {
                pc += 2;
            }
            break;
        }
        case 0xA000: {
            // Annn: LD I, addr (Set I = nnn)
            index = nnn;
            break;
        }
        case 0xB000: {
            // Bnnn : JP V0, addr (Jump to location nnn + V0)
            pc = nnn + v[0x0];
            break;
        }
        case 0xC000: {
            // Cxkk: RND Vx, byte (Set Vx = random byte AND kk)
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dist(0x00, 0xFF);
            v[x] = dist(gen) & kk;
            break;
        }
        case 0xD000: {
            // Dxyn: DRW Vx, Vy, nibble (Display n-byte sprite starting at
            // memory location I at (Vx, Vy), set VF = collision.)

            v[0xF] = 0; // Set VF to 0

            for (auto row = 0; row < n; row++) {

                const uint8_t sprite = memory[index + row];

                for (auto col = 0; col < 8; col++) {
                    // 0x80 is 1000 0000, col = 0 -> checks leftmost
                    if (sprite & (0x80 >> col)) {

                        const auto screenX = (v[x] + col) % 64; // Wrap
                        const auto screenY = (v[y] + row) % 32; // Also wrap

                        if (gfx[screenX][screenY] == 1) {
                            v[0xF] = 1; // Collision! Set VF to 1
                        }

                        // XOR: 0 ^ 0 = 0, 0 ^ 1 = 1, 1 ^ 0 = 1, 1 ^ 1 = 0
                        gfx[screenX][screenY] ^= 1;
                    }
                }
            }

            draw_flag = true;
            break;
        }
        case 0xE000: {
            switch (kk) {
                case 0x9E: {
                    // Ex9E: SKP Vx (Skip next instruction if key with the value
                    // of Vx is pressed)
                    if (keypad[v[x]]) {
                        pc += 2;
                    }
                    break;
                }
                case 0xA1: {
                    // ExA1: SKNP Vx (Skip next instruction if key with the
                    // value of Vx is not pressed)
                    if (!keypad[v[x]]) {
                        pc += 2;
                    }
                    break;
                }
                default: {
                    std::stringstream ss;
                    ss << "Unknown opcode [0xE000 family]: 0x" << std::hex
                       << opcode;
                    throw std::runtime_error(ss.str());
                }
            }
            break;
        }
        case 0xF000: {
            switch (kk) {
                case 0x07: {
                    // Fx07: LD Vx, DT (Set Vx = delay timer value)
                    v[x] = delay_timer;
                    break;
                }
                case 0x0A: {
                    // Fx0A: LD Vx, K (Wait for a key press, store the value of
                    // the key in Vx.)
                    uint8_t i;
                    for (i = 0x0; i <= 0xF; i++) {
                        if (keypad[i]) {
                            v[x] = i;
                            break;
                        }
                    }
                    // Repeat opcode if not found
                    if (i == 0x10) {
                        pc -= 2;
                    }
                    break;
                }
                case 0x15: {
                    // Fx15: LD DT, Vx (Set delay timer = Vx)
                    delay_timer = v[x];
                    break;
                }
                case 0x18: {
                    // Fx18: LD ST, Vx (Set sound timer = Vx)
                    sound_timer = v[x];
                    break;
                }
                case 0x1E: {
                    // Fx1E: ADD I, Vx (Set I = I + Vx)
                    index += v[x];
                    break;
                }
                case 0x29: {
                    // Fx29: LD F, Vx (Set I = location of sprite for digit Vx)
                    index = 0x50 + v[x] * 5;
                    break;
                }
                case 0x33: {
                    // Fx33: LD B, Vx (Store BCD representation of Vx in memory
                    // locations I, I+1, and I+2)
                    memory[index] = (v[x] / 100) % 10;
                    memory[index + 1] = (v[x] / 10) % 10;
                    memory[index + 2] = (v[x] / 1) % 10;
                    break;
                }
                case 0x55: {
                    // Fx55: LD [I], Vx (Store registers V0 through Vx in memory
                    // starting at location I)
                    for (auto i = 0x0; i <= x; i++) {
                        memory[index + i] = v[i];
                    }
                    // QUIRK: INCREMENT INDEX OR NOT
                    // index += x + 1;
                    break;
                }
                case 0x65: {
                    // Fx65: LD Vx, [I] (Read registers V0 through Vx from
                    // memory starting at location I.
                    for (auto i = 0x0; i <= x; i++) {
                        v[i] = memory[index + i];
                    }
                    // QUIRK: INCREMENT INDEX OR NOT
                    // index += x + 1;
                    break;
                }
                default: {
                    std::stringstream ss;
                    ss << "Unknown opcode [0xF000 family]: 0x" << std::hex
                       << opcode;
                    throw std::runtime_error(ss.str());
                }
            }
            break;
        }
        default: {
            std::stringstream ss;
            ss << "Unknown or unimplemented opcode: 0x" << std::hex << opcode;
            throw std::runtime_error(ss.str());
        }
    }
}