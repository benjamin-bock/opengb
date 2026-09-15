#pragma once

#include <cstdint>
#include <cstdlib>
#include <sys/types.h>

#include "bus.hpp"

class CPU {
    public:
        CPU(Bus& bus);
        
        // CPU methods
        uint8_t step();
        uint8_t fetchByte();
        uint16_t fetchWord();
        void writeByte(uint16_t addr, uint8_t byte);
        void writeWord(uint16_t addr, uint16_t word);
        uint8_t execute(uint8_t opcode);
        uint8_t executePrefix(uint8_t opcode);
    private:
        Bus& bus;

        // 1-bit IME (interrupt master enable)
        bool IME = false;
        // 8-bit register
        uint8_t A, F, B, C, D, E, H, L;
        uint8_t getReg(uint8_t index);
        void setReg(uint8_t index, uint8_t data);

        // 16-bit register getter
        uint16_t getAF();
        uint16_t getBC();
        uint16_t getDE();
        uint16_t getHL();

        // 16-bit register setter
        void setAF(uint16_t value);
        void setBC(uint16_t value);
        void setDE(uint16_t value);
        void setHL(uint16_t value);

        // 1-bit flag setter
        void setZ(bool value);
        void setN(bool value);
        void setH(bool value);
        void setC(bool value);

        // 1-bit flag getter
        bool getZ();
        bool getN();
        bool getH();
        bool getC();

        // 16-bit register
        uint16_t SP, PC; // stack pointer, program counter


        // CPU instructions helper
        // Arithmetic operations
        uint8_t ADD(uint8_t reg); // comparison are always between the accumulator and the register
        uint8_t SUB(uint8_t reg);

        uint8_t ADC(uint8_t reg);
        uint8_t SBC(uint8_t reg); 

        // Comparison operations
        uint8_t AND(uint8_t reg);
        uint8_t XOR(uint8_t reg);
        uint8_t OR(uint8_t reg);

        uint8_t CP(uint8_t reg);

        // Stack operations
        void PUSH(uint16_t reg);
        uint16_t POP();


};