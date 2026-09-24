#include "../include/joypad.hpp"
#include "../include/bus.hpp"
#include <cstdint>

Joypad::Joypad(Bus& bus) : bus(bus) {
    // Store bits written by the game (0xFF00)
    // Bits 4 and 5 at 1 by default (nothing selected)
    this->selectBits = 0x30;

    // 0 pressed, 1 released
    // 4 LSB [Down, Up, Left, Right] (bit 3: Down, bit 2: Up, bit 1: Left, bit 0: Right)
    this->dpad = 0x0F;

    // 0 pressed, 1 released
    // 4 LSB [Start, Select, B, A] (bit 3: Start, bit 2: Select, bit 1: B, bit 0: A)
    this->buttons = 0x0F;
}

uint8_t Joypad::read() const {
    // On DMG, bits 6 and 7 are always set to 1 (0xC0).
    // Bits 4 and 5 reflect the selection lines written by the game.
    // Bits 0 to 3 are 1 by default (released / pull-up).
    uint8_t result = 0xCF | this->selectBits;

    // Bit 4 == 0 : game reads D-Pad
    if (this->isDpadEn()) {
        result &= (0xF0 | this->dpad);
    }

    // Bit 5 == 0 : game reads action buttons (A, B, Select, Start)
    if (this->isButtonEn()) {
        result &= (0xF0 | this->buttons);
    }

    return result;
}

void Joypad::write(uint8_t data) {
    // Only bits 4 and 5 are writable (selection lines)
    this->selectBits = data & 0x30;
}

void Joypad::keyPressed(JoypadKey input) {
    int keyIndex = static_cast<int>(input);

    if (keyIndex < 4) {
        // Directions: Right(0), Left(1), Up(2), Down(3)
        // 0 = pressed -> force bit to 0
        this->dpad &= ~(1 << keyIndex);
    } else {
        // Action buttons: A(4), B(5), Select(6), Start(7)
        // 0 = pressed -> force bit to 0
        this->buttons &= ~(1 << (keyIndex - 4));
    }

    // Trigger Joypad interrupt (Bit 4 of $FF0F) if the corresponding line is active
    if ((keyIndex < 4 && this->isDpadEn()) || (keyIndex >= 4 && this->isButtonEn())) {
        uint8_t IF = this->bus.read(0xFF0F);
        this->bus.write(0xFF0F, IF | (1 << 4));
    }
}

void Joypad::keyReleased(JoypadKey input) {
    int keyIndex = static_cast<int>(input);

    if (keyIndex < 4) {
        // Directions: Right(0), Left(1), Up(2), Down(3)
        // 1 = released -> force bit to 1
        this->dpad |= (1 << keyIndex);
    } else {
        // Action buttons: A(4), B(5), Select(6), Start(7)
        // 1 = released -> force bit to 1
        this->buttons |= (1 << (keyIndex - 4));
    }
}

bool Joypad::isButtonEn() const {
    // Active LOW: 0 = selected / enabled
    return !(this->selectBits & 0x20);
}

bool Joypad::isDpadEn() const {
    // Active LOW: 0 = selected / enabled
    return !(this->selectBits & 0x10);
}