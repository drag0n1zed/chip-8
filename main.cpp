#include <iostream>

int main() {
    unsigned short opcode; // Current operation being run
    unsigned char memory[4096]; // 4K memory
    unsigned char V[16]; // V0 -> VF, but don't touch V[15] or VF cuz it's for carry flag

    unsigned short I; // Index register
    unsigned short pc; // Program counter

    unsigned char gfx[64 * 32]; // Graphics, 2048 pixels

    // Timer registers, will count down to zero when set to a positive value
    unsigned char delay_timer;
    unsigned char sound_timer; // When reaches zero, buzzer sounds

    unsigned short stack[16];
    unsigned short sp; // Stack pointer

    unsigned char key[16]; // Current state of keypad (keys pressed)

    return 0;
}