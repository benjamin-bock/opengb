#ifndef CPU_HPP
#define CPU_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <stack>
#include <algorithm>
#include <cmath>
#include <cstdlib>

class CPU {
    public:
        CPU();
    
    private:
        // 8-bit register
        uint8_t A, F, B, C, D, E, H, L;

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

        // 16-bit register
        uint16_t SP, PC; // stack pointer, program counter

        uint8_t step();
        uint8_t fetchByte();
        uint16_t fetchWord();
        uint8_t execute(uint8_t opcode);
}

