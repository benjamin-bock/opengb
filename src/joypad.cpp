#include "../include/joypad.hpp"
#include <cstdint>

Joypad::Joypad(Bus& bus) {
    // Store bits written by the game (OxFF00)
    this->selectBits = 0x30;

    // 0 pressed, 1 released
    // 4 LSB [Down, Up, Left, Right]
    this->buttons = 0x0F;

    // 0 pressed, 1 released
    // 4 LSB [Start, Select, B, A]    
    this->dpad = 0x0F;
}

uint8_t Joypad::read() const {
    return this->selectBits;
}

void Joypad::write(uint8_t data) {
    this->selectBits = data & 0xF0;
}

void Joypad::keyPressed(JoypadKey input) {
    if (this->isDpadEn()) {
        this->dpad ^= (1 << static_cast<int>(input));
    }
    if (this->isButtonEn()) {
        this->buttons ^= (1 << (static_cast<int>(input) - 4));
    }
    this->selectBits = 0x3F & ((this->isButtonEn() ? 0x20 : 0x00) | (this->isDpadEn() ? 0x10 : 0x00) | (this->dpad & 0x0F));
}

void Joypad::keyReleased(JoypadKey input) {
    if (this->isDpadEn()) {
        this->dpad &= ~(1 << static_cast<int>(input));
    }
    if (this->isButtonEn()) {
        this->buttons &= ~(1 << (static_cast<int>(input) - 4));
    }
    this->selectBits = 0x3F & ((this->isButtonEn() ? 0x20 : 0x00) | (this->isDpadEn() ? 0x10 : 0x00) | (this->dpad & 0x0F));
}