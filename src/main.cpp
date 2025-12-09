#include "Chip8.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <ncurses.h>
#include <stdexcept>
#include <thread>

int main(const int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filepath>" << std::endl;
        return 1;
    }

    // Create instance
    Chip8 chip;

    // Load ROM
    if (!chip.LoadROM(argv[1])) {
        return 1;
    }

    // Initialize ncurses
    initscr();             // Init screen
    noecho();              // Don't print key presses to screen
    raw();                 // Pass everything typed directly into program
    nodelay(stdscr, true); // Make input reading non-blocking
    curs_set(0);           // Hide cursor
    keypad(stdscr, TRUE);  // Enable function keys

    // Use independent timers for CPU and 60Hz events for maximum responsiveness.
    constexpr auto cpu_cycle_delay =
        std::chrono::nanoseconds(1000000000 / 700); // 700 Hz CPU Speed
    constexpr auto timer_cycle_delay =
        std::chrono::microseconds(1000000 / 60); // 60 Hz Timers
    auto last_cpu_cycle = std::chrono::high_resolution_clock::now();
    auto last_timer_cycle = std::chrono::high_resolution_clock::now();

    try {
        while (!chip.stop_flag) {
            int ch;
            while ((ch = getch()) != ERR) {
                constexpr uint8_t KEY_PRESS_TIMEOUT = 30;
                switch (ch) {
                    case KEY_RESIZE: {
                        chip.draw_flag = true;
                        break;
                    }
                    case '1':
                        chip.keypad_timers[0x1] = KEY_PRESS_TIMEOUT;
                        break;
                    case '2':
                        chip.keypad_timers[0x2] = KEY_PRESS_TIMEOUT;
                        break;
                    case '3':
                        chip.keypad_timers[0x3] = KEY_PRESS_TIMEOUT;
                        break;
                    case '4':
                        chip.keypad_timers[0xC] = KEY_PRESS_TIMEOUT;
                        break;
                    case 'q':
                        chip.keypad_timers[0x4] = KEY_PRESS_TIMEOUT;
                        break;
                    case 'w':
                        chip.keypad_timers[0x5] = KEY_PRESS_TIMEOUT;
                        break;
                    case 'e':
                        chip.keypad_timers[0x6] = KEY_PRESS_TIMEOUT;
                        break;
                    case 'r':
                        chip.keypad_timers[0xD] = KEY_PRESS_TIMEOUT;
                        break;
                    case 'a':
                        chip.keypad_timers[0x7] = KEY_PRESS_TIMEOUT;
                        break;
                    case 's':
                        chip.keypad_timers[0x8] = KEY_PRESS_TIMEOUT;
                        break;
                    case 'd':
                        chip.keypad_timers[0x9] = KEY_PRESS_TIMEOUT;
                        break;
                    case 'f':
                        chip.keypad_timers[0xE] = KEY_PRESS_TIMEOUT;
                        break;
                    case 'z':
                        chip.keypad_timers[0xA] = KEY_PRESS_TIMEOUT;
                        break;
                    case 'x':
                        chip.keypad_timers[0x0] = KEY_PRESS_TIMEOUT;
                        break;
                    case 'c':
                        chip.keypad_timers[0xB] = KEY_PRESS_TIMEOUT;
                        break;
                    case 'v':
                        chip.keypad_timers[0xF] = KEY_PRESS_TIMEOUT;
                        break;
                    case 3:  // SIGINT (Ctrl + C)
                    case 27: // Escape key
                    {
                        chip.stop_flag = true;
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }

            auto current_time = std::chrono::high_resolution_clock::now();
            auto delta_timer_time = current_time - last_timer_cycle;
            auto delta_cpu_time = current_time - last_cpu_cycle;

            if (delta_timer_time >= timer_cycle_delay) {
                last_timer_cycle = current_time;

                // Decrement timers
                if (chip.delay_timer > 0) {
                    chip.delay_timer--;
                }
                if (chip.sound_timer > 0) {
                    chip.sound_timer--;
                    if (chip.sound_timer == 0) {
                        beep();
                    }
                }
                for (int i = 0; i < 16; ++i) {
                    if (chip.keypad_timers[i] > 0) {
                        chip.keypad_timers[i]--;
                    }
                }
            }

            if (delta_cpu_time >= cpu_cycle_delay) {
                last_cpu_cycle = current_time;

                // Derive keypad state from timers
                for (int i = 0; i < 16; ++i) {
                    chip.keypad[i] = (chip.keypad_timers[i] > 0);
                }

                // Execute a single Opcode
                chip.HandleOpcode();

                // Render
                if (chip.draw_flag) {
                    clear();
                    int term_y, term_x;
                    getmaxyx(stdscr, term_y, term_x);
                    const int start_y = (term_y - 32) / 2;
                    const int start_x = (term_x - 64) / 2;

                    for (auto y = 0; y < 32; y++) {
                        for (auto x = 0; x < 64; x++) {
                            if (chip.gfx[x][y]) {
                                mvaddch(start_y + y, start_x + x, '#');
                            } else {
                                mvaddch(start_y + y, start_x + x, ' ');
                            }
                        }
                    }
                    refresh();
                    chip.draw_flag = false;
                }
            }

            // Prevent 100% CPU
            std::this_thread::sleep_for(std::chrono::nanoseconds(100));
        }
    } catch (const std::runtime_error &e) {
        endwin();
        std::cerr << "\n" << e.what() << std::endl;
        return 1;
    }

    endwin();
    return 0;
}