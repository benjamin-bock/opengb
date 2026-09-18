#pragma once

#include <cstdint>

class Bus;

class Timer {
    public:
        Timer(Bus& bus);
        ~Timer() = default;

        uint8_t getClockSelect();
        bool getClockEnable();
        void step(uint8_t cycles);

        // getter functions
        uint8_t getDIV() const;
        uint8_t getTIMA() const;
        uint8_t getTMA() const;
        uint8_t getTAC() const;

        // setter functions;
        void setDIV(uint8_t data);
        void setTIMA(uint8_t data);
        void setTMA(uint8_t data);
        void setTAC(uint8_t data);
    
    private:
        Bus& bus;

        uint16_t divCycleCounter; // use to count cycles before increment DIV
        uint16_t timaCycleCounter; // use to count cycles before increment TIMA
        uint8_t DIV;  // Divider Register
        uint8_t TIMA; // Timer counter
        uint8_t TMA;  // Timer modulo
        uint8_t TAC;  // Timer control

};