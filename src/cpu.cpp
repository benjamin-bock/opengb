include "cpu.hpp"

CPU::CPU() {
    A = 0x01; // hardware checks wich console is running (GB, GBC, GBA, etc.)
    F = 0xB0; // 1011 0000 -> Z = 1, N = 0, H = 1, C = 1
    B = 0x00;
    C = 0x13; 
    D = 0x00;
    E = 0xD8;
    H = 0x01;
    L = 0x4D;
    SP = 0xFFFE; // stack pointer pointing to the top of the stack
    PC = 0x0100; // program counter pointing to the start of the program
}

uint16_t CPU::getAF() {
    return (A << 8) | F;
}

uint16_t CPU::getBC() {
    return (B << 8) | C;
}

uint16_t CPU::getDE() {
    return (D << 8) | E;
}

uint16_t CPU::getHL() {
    return (H << 8) | L;
}

void CPU::setAF(uint16_t value) {
    this->A = (uint8_t)(value >> 8); // get the most significant byte
    this->F = (uint8_t)(value & 0x00FF); // get the least significant byte
    return;
}

void CPU::setBC(uint16_t value) {
    this->B = (uint8_t)(value >> 8);
    this->C = (uint8_t)(value & 0x00FF);
    return;
}

void CPU::setDE(uint16_t value) {
    this->D = (uint8_t)(value >> 8);
    this->E = (uint8_t)(value & 0x00FF);
    return;
}

void CPU::setHL(uint16_t value) {
    this->H = (uint8_t)(value >> 8);
    this->L = (uint8_t)(value & 0x00FF);
    return;
}

uint8_t CPU::step() {
    return 0;
}

uint8_t CPU::fetchByte() {
    uint8_t opcode = Bus::read(this->PC++);
    return opcode;
}

uint16_t CPU::fetchWord() {
    uint8_t lower = this->fetchByte();
    uint8_t upper = this->fetchByte();
    return (uint16_t)(upper << 8) | lower;
}

uint8_t CPU::execute(uint8_t opcode) {
    // Exécuter l'action et renvoyer le nombre de cycles d'horloge consommés.
    switch (opcode) {
        case 0x00: // NOP
            return 4;

        case 0x01: // LD BC,u16
            this->setBC(this->fetchWord());
            return 12;

        case 0x11: // LD DE,u16
            this->setDE(this->fetchWord());
            return 12;
        
        case 0x21: // LD HL,u16
            this->setHL(this->fetchWord());
            return 12;
        
        case 0x31: // LD SP,u16
            this->SP = this->fetchWord(); // lower
            return 12;

        case 0xC3: // JP u16
            this->PC = this->fetchWord(); // PC -> u16
            return 16;

        default:
            return 0;
    }
}