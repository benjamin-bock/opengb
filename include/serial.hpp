#pragma once

#include <cstdint>

class Bus;

class Serial {
    public:
        Serial(Bus& bus);
        ~Serial() = default;

        void step(uint8_t cycles);

        uint8_t read(uint16_t addr) const;
        void write(uint16_t addr, uint8_t data);

    private:
        Bus& bus;
        uint8_t SB; // Serial transfer data
        uint8_t SC; // Serial transfer control

        uint16_t transferCounter; // cycles elapsed since transfer start
        bool transferring;
};
