#include "../include/timer.hpp"

#include <array>

#include "../include/bus.hpp"

static constexpr std::array<uint16_t, 4> TIMA_THRESHOLD = {
    1024, 16, 64, 256
};

static constexpr uint16_t DIV_THRESHOLD = 256;

Timer::Timer(Bus& bus) : bus(bus) {

    this->divCycleCounter = 0x00;
    this->timaCycleCounter = 0x00;
    this->DIV = 0x00;
    this->TIMA = 0x00;
    this->TMA = 0x00;
    this->TAC = 0x00;
}

uint8_t Timer::getClockSelect() {
    return this->TAC & 0x03;
}

bool Timer::getClockEnable() {
    return static_cast<bool>((this->TAC & 0x04) >> 2);
}

void Timer::step(uint8_t cycles) {

    // Increment DIV at a 16384 Hz rate = 64 M-cycles = 256 T-cycles
    this->divCycleCounter += cycles;
    while (this->divCycleCounter >= DIV_THRESHOLD) {
        this->DIV++;
        this->divCycleCounter -= DIV_THRESHOLD;
    }

    // Increment TIMA at frequency controlled by TAC
    if (this->getClockEnable()) {
        this->timaCycleCounter += cycles;
        uint16_t threshold = TIMA_THRESHOLD[this->getClockSelect()];

        while (this->timaCycleCounter >= threshold) {
            this->timaCycleCounter -= threshold;

            if (this->TIMA == 0xFF) {
                // Overflow occured; reset to modulo value TMA
                this->TIMA = this->TMA;

                // Trigger an interrupt
                uint8_t currentIF = this->bus.read(0xFF0F); // get Interrupt Flag
                this->bus.write(0xFF0F, currentIF | 0x04); // set bit 2 of IF (0xFF0F)
            }
            else {
                // Normal case: increment TIMA
                this->TIMA++;
            }
        }
    }
    return;
}

// getter functions
uint8_t Timer::getDIV() const {
    return this->DIV;
}

uint8_t Timer::getTIMA() const {
    return this->TIMA;
}

uint8_t Timer::getTMA() const {
    return this->TMA;
}

uint8_t Timer::getTAC() const {
    return this->TAC;
}

// getter functions
void Timer::setDIV(uint8_t data) {
    (void)data;
    this->DIV = 0x00; // Writing to DIV resets the counter to $00
}

void Timer::setTIMA(uint8_t data) {
    this->TIMA = data;
}

void Timer::setTMA(uint8_t data) {
    this->TMA = data;
}

void Timer::setTAC(uint8_t data) {
    this->TAC = data;
}
