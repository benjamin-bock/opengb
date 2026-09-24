#pragma once

#include <cstdint>

class Bus;

enum class JoypadKey {
    Right,
    Left,
    Up,
    Down,
    A,
    B,
    Select,
    Start
};

class Joypad {
    public:
        Joypad(Bus& bus);
        ~Joypad() = default;

        uint8_t read() const;
        void write(uint8_t data);

        // Methods called by SDL
        void keyPressed(JoypadKey input);
        void keyReleased(JoypadKey input);

        bool isButtonEn();
        bool isDpadEn();

    private:
        uint8_t selectBits;
        uint8_t buttons;
        uint8_t dpad;
};