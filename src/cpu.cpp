#include "../include/cpu.hpp"
#include <cstdint>
#include <sys/types.h>

CPU::CPU(Bus& bus) : bus(bus){
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
    this->A = static_cast<uint8_t>(value >> 8); // get the most significant byte
    this->F = static_cast<uint8_t>(value & 0x00FF); // get the least significant byte
    return;
}

void CPU::setBC(uint16_t value) {
    this->B = static_cast<uint8_t>(value >> 8);
    this->C = static_cast<uint8_t>(value & 0x00FF);
    return;
}

void CPU::setDE(uint16_t value) {
    this->D = static_cast<uint8_t>(value >> 8);
    this->E = static_cast<uint8_t>(value & 0x00FF);
    return;
}

void CPU::setHL(uint16_t value) {
    this->H = static_cast<uint8_t>(value >> 8); // get the most significant byte
    this->L = static_cast<uint8_t>(value & 0x00FF); // get the least significant byte   
    return;
}

void CPU::setZ(bool value) {
    if(value) {
        this->F = (this->F | 0x80);
    }
    else {
        this->F = (this->F & 0x7F);
    }
    return;
}

void CPU::setN(bool value) {
    if(value) {
        this->F = (this->F | 0x40);
    }
    else {
        this->F = (this->F & 0xBF);
    }
    return;
}

void CPU::setH(bool value) {
    if(value) {
        this->F = (this->F | 0x20);
    }
    else {
        this->F = (this->F & 0xDF);
    }
    return;
}

void CPU::setC(bool value) {
    if(value) {
        this->F = (this->F | 0x10);
    }
    else {
        this->F = (this->F & 0xEF);
    }
    return;
}

bool CPU::getZ() {
    return (this->F & 0x80) != 0;
}

bool CPU::getN() {
    return (this->F & 0x40) != 0;
}

bool CPU::getH() {
    return (this->F & 0x20) != 0;
}

bool CPU::getC() {
    return (this->F & 0x10) != 0;
}

uint8_t CPU::step() {
    return 0;
}

uint8_t CPU::fetchByte() {
    uint8_t opcode = this->bus.read(this->PC++);
    return opcode;
}

uint16_t CPU::fetchWord() {
    uint8_t lower = this->fetchByte();
    uint8_t upper = this->fetchByte();
    return static_cast<uint16_t>((upper << 8) | lower);
}

void CPU::writeByte(uint16_t addr, uint8_t byte) {
    this->bus.write(addr, byte);
    return;
}

void CPU::writeWord(uint16_t addr, uint16_t word) {
    uint8_t lower = static_cast<uint8_t>(word & 0x00FF);
    uint8_t upper = static_cast<uint8_t>(word & 0xFF00) >> 8;
    this->writeByte(addr, lower);
    this->writeByte(addr + 1, upper);
    return;
}

uint8_t CPU::execute(uint8_t opcode) {
    // Exécuter l'action et renvoyer le nombre de cycles d'horloge consommés.
    switch (opcode) {
        case 0x00: // NOP
            return 4;

        case 0x01: // LD BC,u16
            this->setBC(this->fetchWord());
            return 12;

        case 0x02: // LD (BC),A
            this->bus.write(this->getBC(), this->A);
            return 8;

        case 0x03: // INC BC
            this->setBC(this->getBC() + 1);
            return 8;
        
        case 0x04: // INC B
            this->setH((this->B & 0x0F) == 0x0F);
            this->B++;
            this->setZ(this->B == 0);
            this->setN(false);
            return 4;

        case 0x05: // DEC B
            this->setH((this->B & 0x0F) == 0x0F);
            this->B--;
            this->setZ(this->B == 0);
            this->setN(true);
            return 4;

        case 0x06: // LD B,u8
            this->B = this->fetchByte();
            return 8;

        case 0x07: // RLCA
            this->setC(static_cast<bool>((0x80 & this->A) >> 8)); // set A_7 to C flag
            this->A <<= 1; // left bit shift
            if (this->getC()) {
                this->A |= 0x01; // set A_0 to 1
            }
            this->setZ(false);
            this->setH(false);
            this->setN(false);
            return 4;
        
        case 0x08: // LD (u16),SP
            this->writeWord(this->fetchWord(), this->SP);
            return 20;  

        case 0x09: {// ADD HL,BC
            uint16_t hl = getHL();
            uint16_t bc = getBC();
            this->setH((hl & 0x0FFF) + (bc & 0x0FFF) > 0x0FFF);
            this->setC(static_cast<uint32_t>(bc) + static_cast<uint32_t>(hl) > 0xFFFF);
            this->setHL(bc + hl);
            this->setN(false);
            return 8;
        }

        case 0x0A: // LD A,(BC)
        this->A = this->bus.read(this->getBC());
        return 8;

        case 0x0B: // DEC BC
        this->setBC(this->getBC() - 1);
        return 8;

        case 0x0C: // INC C
            this->setH((this->C & 0x0F) == 0x0F);
            this->C++;
            this->setZ(this->C == 0);
            this->setN(false);
            return 4;

        case 0x0D: // DEC C
            this->setH((this->C & 0x0F) == 0x00);
            this->C--;
            this->setZ(this->C == 0);
            this->setN(true);
            return 4;

        case 0x0E: // LD C,u8
            this->C = fetchByte();
            return 8;        
            
        case 0x0F: { // RRCA
            bool A_0 = static_cast<bool>(this->A & 0x01); // LSB of A
            this->setC(A_0); // C = A_0
            this->setH(false);
            this->A >>= 1;
            this->A |= static_cast<uint8_t>(A_0 << 7);
            this->setZ(false);
            this->setN(false);
            return 4;
        }

        case 0x10: // STOP
            this->fetchByte(); // ignore next instruction 0x00
            // puts the GB into low power standby mode
            return 4;

        case 0x11: // LD DE,u16
            this->setDE(this->fetchWord());
            return 12;

        case 0x12: // LD (DE),A
            this->bus.write(this->getDE(), this->A);
            return 8;

        case 0x13: // INC DE
            this->setDE(this->getDE() + 1);
            return 8;
        
        case 0x14: // INC D
            this->setH((this->D & 0x0F) == 0x0F);
            this->D++;
            this->setZ(this->D == 0);
            this->setN(false);
            return 4;

        case 0x15: // DEC D
            this->setH((this->D & 0x0F) == 0x00);
            this->D--;
            this->setZ(this->D == 0);
            this->setN(true);
            return 4;

        case 0x16: // LD D,u8
            this->D = this->fetchByte();
            return 8;

        case 0x17: { // RLA
            bool newCarry = static_cast<bool>((this->A & 0x80) >> 7); // save MSB of A
            this->A <<= 1; // left bit shift
            this->A |= this->getC(); // put C at A_0 position
            this->setH(false);
            this->setZ(false);
            this->setN(false);
            this->setC(newCarry);
            return 4;
        }
        
        case 0x18: { // JR i8
            int8_t offset = static_cast<int8_t>(this->fetchByte());
            this->PC += offset;
            return 12;  
        }
        
        case 0x19: {// ADD HL,DE
            uint16_t hl = getHL();
            uint16_t de = getDE();
            this->setH((hl & 0x0FFF) + (de & 0x0FFF) > 0x0FFF);
            this->setC(static_cast<uint32_t>(de) + static_cast<uint32_t>(hl) > 0xFFFF);
            this->setHL(de + hl);
            this->setN(false);
            return 8;
        }

        case 0x1A: // LD A,(DE)
        this->A = this->bus.read(this->getDE());
        return 8;

        case 0x1B: // DEC DE
        this->setDE(this->getDE() - 1);
        return 8;

        case 0x1C: // INC E
            this->setH((this->E & 0x0F) == 0x0F);
            this->E++;
            this->setZ(this->E == 0);
            this->setN(false);
            return 4;

        case 0x1D: // DEC E
            this->setH((this->E & 0x0F) == 0x00);
            this->E--;
            this->setZ(this->E == 0);
            this->setN(true);
            return 4;

        case 0x1E: // LD E,u8
            this->E = fetchByte();
            return 8;        
            
        case 0x1F: { // RRA
            bool newCarry = static_cast<bool>(this->A & 0x01); // save LSB of A
            this->A >>= 1; // right bit shift
            this->A |= static_cast<uint8_t>(this->getC() << 7); // put Carry at A_7 position
            this->setH(false);
            this->setZ(false);
            this->setN(false);
            this->setC(newCarry);
            return 4;
        }

        case 0x20: { // JR NZ,i8
            int8_t offset = static_cast<int8_t>(this->fetchByte());
            if (!getZ()) { // Z is false, jump
                this->PC += offset;
                return 12;
            }
            else { // Z is true, no jump
                return 8;
            }
        }
        
        case 0x21: // LD HL,u16
            this->setHL(this->fetchWord());
            return 12;

        case 0x22: { // LD (HL+),A
            uint16_t hl = this->getHL();
            this->bus.write(hl, this->A);
            this->setHL(hl + 1);
            return 8;
        }

        case 0x23: // INC HL
            this->setHL(this->getHL() + 1);
            return 8;
        
        case 0x24: // INC H
            this->setH((this->H & 0x0F) == 0x0F);
            this->H++;
            this->setZ(this->H == 0);
            this->setN(false);
            return 4;

        case 0x25: // DEC H
            this->setH((this->H & 0x0F) == 0x00);
            this->H--;
            this->setZ(this->H == 0);
            this->setN(true);
            return 4;

        case 0x26: // LD H,u8
            this->H = this->fetchByte();
            return 8;

        case 0x27: { // DAA (Decimal Adjust Accumulator)
            uint8_t adjust = 0;
            bool carry = this->getC();
            bool substract = this->getN();

            // lower nibble
            if (this->getH() || (!substract && ( (this->A & 0x0F) > 0x09) )) {
                adjust |= 0x06;
            }

            // upper nibble
            if (carry || (!substract && (this->A > 0x99) )) {
                adjust |= 0x60;
                carry = true;
            }

            // apply the adjustement, + or - depending on N
            if (substract) { 
                this->A -= adjust;
            }
            else {
                this->A += adjust;
            }

            setH(false);
            setZ(this->A == 0);
            setC(carry);
            return 4;
        }
        
        case 0x28: { // JR Z,i8
            int8_t offset = static_cast<int8_t>(this->fetchByte());
            if (getZ()) { // Z is true, jump
                this->PC += offset;
                return 12;
            }
            else { // Z is false, no jump
                return 8;
            }
        }

        case 0x29: {// ADD HL,HL
            uint16_t hl = getHL();
            this->setH((hl & 0x0FFF) + (hl & 0x0FFF) > 0x0FFF);
            this->setC(static_cast<uint32_t>(hl) + static_cast<uint32_t>(hl) > 0xFFFF);
            this->setHL(hl + hl);
            this->setN(false);
            return 8;
        }

        case 0x2A: {// LD A,(HL+)
            uint16_t hl = this->getHL();
            this->A = this->bus.read(hl);
            this->setHL(hl + 1); // increment HL by 1
            return 8;
        }

        case 0x2B: // DEC HL
        this->setHL(this->getHL() - 1);
        return 8;

        case 0x2C: // INC L
            this->setH((this->L & 0x0F) == 0x0F);
            this->L++;
            this->setZ(this->L == 0);
            this->setN(false);
            return 4;

        case 0x2D: // DEC L
            this->setH((this->L & 0x0F) == 0x00);
            this->L--;
            this->setZ(this->L == 0);
            this->setN(true);
            return 4;

        case 0x2E: // LD L,u8
            this->L = fetchByte();
            return 8;        
          
        case 0x2F: { // CPL
            this->A ^= 0xFF; // complement is equivalent to XOR 0xFF
            this->setH(true);
            this->setN(true);
            return 4;
        }

        case 0x30: { // JR NC,i8
            int8_t offset = static_cast<int8_t>(this->fetchByte());
            if (!getC()) { // C is false, jump
                this->PC += offset;
                return 12;
            }
            else { // Z is true, no jump
                return 8;
            }
        }

        case 0x31: // LD SP,u16
            this->SP = this->fetchWord(); // lower
            return 12;

        case 0x32: { // LD (HL-),A
            uint16_t hl = this->getHL();
            this->bus.write(hl, this->A);
            this->setHL(hl - 1);
            return 8;
        }

        case 0x33: // INC SP
            this->SP++;
            return 8;
        
        case 0x34: { // INC (HL)
            uint16_t hl = this->getHL();
            uint16_t data = this->bus.read(hl);

            this->setH((data++ & 0x0F) == 0x0F);
            this->bus.write(hl, data);
            this->setZ(data == 0);
            this->setN(false);
            return 12;
        }
        case 0x35: { // DEC (HL)
            uint16_t hl = this->getHL();
            uint16_t data = this->bus.read(hl);

            this->setH((data-- & 0x0F) == 0x00);
            this->bus.write(hl, data);
            this->setZ(data == 0);
            this->setN(false);
            return 12;
        }

        case 0x36: { // LD (HL),u8
            uint16_t hl = this->getHL();

            this->bus.write(hl,this->fetchByte());
            return 12;
        }

        case 0x37: { // SCF (Set Carry Flag)
            this->setH(false);
            this->setN(false);
            this->setC(true);
            return 4;
        }
        
        case 0x38: { // JR C,i8
            int8_t offset = static_cast<int8_t>(this->fetchByte());
            if (getC()) { // C is true, jump
                this->PC += offset;
                return 12;
            }
            else { // Z is false, no jump
                return 8;
            }
        }
/* TO_DO
        case 0x39: {// ADD HL,HL
            uint16_t hl = getHL();
            this->setH((hl & 0x0FFF) + (hl & 0x0FFF) > 0x0FFF);
            this->setC(static_cast<uint32_t>(hl) + static_cast<uint32_t>(hl) > 0xFFFF);
            this->setHL(hl + hl);
            this->setN(false);
            return 8;
        }

        case 0x3A: {// LD A,(HL+)
            uint16_t hl = this->getHL();
            this->A = this->bus.read(hl);
            this->setHL(hl + 1); // increment HL by 1
            return 8;
        }

        case 0x3B: // DEC HL
        this->setHL(this->getHL() - 1);
        return 8;

        case 0x3C: // INC L
            this->setH((this->L & 0x0F) == 0x0F);
            this->L++;
            this->setZ(this->L == 0);
            this->setN(false);
            return 4;

        case 0x3D: // DEC L
            this->setH((this->L & 0x0F) == 0x00);
            this->L--;
            this->setZ(this->L == 0);
            this->setN(true);
            return 4;

        case 0x3E: // LD L,u8
            this->L = fetchByte();
            return 8;        
*/         
        case 0x3F: { // CCF (Complement Carry Flag)
            this->setH(false);
            this->setN(false);
            this->setC(!this->getC()); // complement C
            return 4;
        }

        case 0xC3: // JP u16
            this->PC = this->fetchWord(); // PC -> u16
            return 16;

        default:
            return 0;
    }
}