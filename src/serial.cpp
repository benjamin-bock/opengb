#include "../include/serial.hpp"
#include "../include/bus.hpp"

#include <iostream>

// A serial transfer takes 8 bits x 8 cycles = 64 cycles on DMG
static constexpr uint16_t SERIAL_TRANSFER_CYCLES = 512;

Serial::Serial(Bus& bus) : bus(bus) {
    this->SB = 0x00;
    this->SC = 0x00;
    this->transferCounter = 0x0000;
    this->transferring = false;
}

void Serial::step(uint8_t cycles) {
    if (!this->transferring) {
        return;
    }

    this->transferCounter += cycles;
    if (this->transferCounter >= SERIAL_TRANSFER_CYCLES) {
        // End of transfer: no peer connected, the shift register reads 0xFF
        this->transferring = false;
        this->transferCounter = 0;
        this->SB = 0xFF;
        this->SC &= 0x7F; // clear the transfer start bit
        // Raise the serial interrupt
        uint8_t IF = this->bus.read(0xFF0F);
        this->bus.write(0xFF0F, IF | (1 << 3));
    }
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
            if (data & 0x80 && (data & 0x01)) {
                // Internal clock transfer requested: start it
                if (data == 0x81) {
                    std::cout << static_cast<char>(this->SB) << std::flush;
                }
                this->transferring = true;
                this->transferCounter = 0;
            }
            return;

        default:     
            return;
    }
}
