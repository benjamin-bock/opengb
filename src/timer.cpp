#include "../include/timer.hpp"

#include <array>

#include "../include/bus.hpp"

// Bit of the internal 16-bit divider selected by each TAC clock setting.
// A falling edge on that bit increments TIMA.
static constexpr std::array<uint8_t, 4> TIMA_BIT = { 9, 3, 5, 7 };

// T-cycles the TIMA reload is delayed after an overflow (1 M-cycle)
static constexpr uint8_t OVERFLOW_DELAY = 4;

Timer::Timer(Bus& bus) : bus(bus) {
    this->divider = 0x0000;
    this->TIMA = 0x00;
    this->TMA = 0x00;
    this->TAC = 0x00;
    this->overflowDelay = 0;
    this->reloadPending = false;
}

uint8_t Timer::getClockSelect() {
    return this->TAC & 0x03;
}

bool Timer::getClockEnable() {
    return static_cast<bool>((this->TAC & 0x04) >> 2);
}

void Timer::incrementTIMA() {
    if (this->TIMA == 0xFF) {
        // Overflow: TIMA stays 0xFF for OVERFLOW_DELAY cycles, then is
        // reloaded with TMA and the timer interrupt is requested.
        this->overflowDelay = OVERFLOW_DELAY;
        this->reloadPending = true;
    } else {
        this->TIMA++;
    }
}

void Timer::step(uint8_t cycles) {
    for (uint8_t i = 0; i < cycles; i++) {
        // count down the post-overflow delay
        if (this->overflowDelay > 0) {
            this->overflowDelay--;
            if (this->overflowDelay == 0) {
                if (this->reloadPending) {
                    this->TIMA = this->TMA;
                    this->reloadPending = false;
                }
                // the timer interrupt is requested even if the reload was cancelled
                uint8_t currentIF = this->bus.read(0xFF0F);
                this->bus.write(0xFF0F, currentIF | 0x04); // set bit 2 of IF (0xFF0F)
            }
        }

        // clock the internal 16-bit divider (DIV is its upper byte)
        uint16_t before = this->divider;
        this->divider++;

        // TIMA increments on the falling edge of the selected divider bit
        if (this->getClockEnable()) {
            uint8_t bit = TIMA_BIT[this->getClockSelect()];
            bool wasSet = (before >> bit) & 1;
            bool isSet = (this->divider >> bit) & 1;
            if (wasSet && !isSet) {
                this->incrementTIMA();
            }
        }
    }
    return;
}

// getter functions
uint8_t Timer::getDIV() const {
    return static_cast<uint8_t>(this->divider >> 8);
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

// setter functions
void Timer::setDIV(uint8_t data) {
    (void)data;
    // writing to DIV resets the whole internal divider (this can also
    // clock TIMA if the selected bit falls)
    this->divider = 0x0000;
}

void Timer::setTIMA(uint8_t data) {
    this->TIMA = data;
    // a write during the overflow delay cancels the reload (the interrupt still fires)
    this->reloadPending = false;
}

void Timer::setTMA(uint8_t data) {
    this->TMA = data;
}

void Timer::setTAC(uint8_t data) {
    this->TAC = data;
}
