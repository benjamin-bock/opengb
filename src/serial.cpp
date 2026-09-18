#include "../include/serial.hpp"

#include <iostream>

Serial::Serial() {
    this->SB = 0x00;
    this->SC = 0x00;
}

uint8_t Serial::read(uint16_t addr) const {
    switch (addr) {
        case 0xFF01: 
            return this->SB;

        case 0xFF02: 
            return this->SC;

        default:     
            return 0xFF;
    }
}

void Serial::write(uint16_t addr, uint8_t data) {
    switch (addr) {
        case 0xFF01: 
            this->SB = data;
            return;

        case 0xFF02: 
            this->SC = data;
            if (data == 0x81) {
                std::cout << static_cast<char>(this->SB) << std::flush;
                this->SC = 0x01;
            }
            return;

        default:     
            return;
    }
}