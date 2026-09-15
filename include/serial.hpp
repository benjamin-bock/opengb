#pragma once
#include <atomic>
#include <cstdint>
#include <iostream>

class Serial {
    public:
        Serial();
        uint8_t read(uint16_t addr) const;
        void write(uint16_t addr, uint8_t data);

    private:
        uint8_t SB; // Serial transfer data
        uint8_t SC; // Serial transfer control
};