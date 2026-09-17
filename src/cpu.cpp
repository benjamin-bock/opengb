#include "../include/cpu.hpp"

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

uint8_t CPU::getReg(uint8_t index) {
    switch (index) {
        case 0: return this->B;
        case 1: return this->C;
        case 2: return this->D;
        case 3: return this->E;
        case 4: return this->H;
        case 5: return this->L;
        case 6: return this->bus.read(this->getHL());
        case 7: return this->A;

        default:
            return 0xFF;
    }
}

void CPU::setReg(uint8_t index, uint8_t data) {
    switch (index) {
        case 0: this->B = data; break;
        case 1: this->C = data; break;
        case 2: this->D = data; break;
        case 3: this->E = data; break;
        case 4: this->H = data; break;
        case 5: this->L = data; break;
        case 6: this->bus.write(this->getHL(), data); break;
        case 7: this->A = data; break;

        default:
            return;
    }
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
    this->F = static_cast<uint8_t>(value & 0x00F0); // get the least significant byte and force the last 4 bits to 0
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
    uint8_t cycles = 0;

    // Check and handle any pending interrupts
    cycles += this->handleInterrupts(); 

    if (this->isHalted) {
        // If the CPU is halted, we skip the instruction fetch and execution
        return 4;
    }

    // Fetch the next instruction and execute it
    uint8_t opcode = this->fetchByte();
    cycles += this->execute(opcode);
    
    // Handle the EI delay if it's active
    if (this->eiDelay > 0) {
        this->eiDelay--;
        if (this->eiDelay == 0) {
            this->IME = true; // Enable interrupts after the delay
        }
    }

    return cycles;
}

uint8_t CPU::fetchByte() {
    uint8_t instr = this->bus.read(this->PC++);
    return instr;
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
    uint8_t upper = static_cast<uint8_t>((word >> 8) & 0x00FF);
    this->writeByte(addr, lower);
    this->writeByte(addr + 1, upper);
    return;
}

uint8_t CPU::execute(uint8_t instr) {
    // Exécuter l'action et renvoyer le nombre de cycles d'horloge consommés.
    switch (instr) {
        case 0x00: // NOP
            return 4;

        case 0x01: // LD BC,u16
            this->setBC(this->fetchWord());
            return 12;

        case 0x02: // LD (BC),A
            this->writeByte(this->getBC(), this->A);
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
            this->setH((this->B & 0x0F) == 0x00);
            this->B--;
            this->setZ(this->B == 0);
            this->setN(true);
            return 4;

        case 0x06: // LD B,u8
            this->B = this->fetchByte();
            return 8;

        case 0x07: { // RLCA
            bool bit7 = (this->A & 0x80) != 0;
            this->A = (this->A << 1) | (bit7 ? 1 : 0); // left bit shift and copy bit7 into bit0
            this->setC(bit7); // set A_7 to C flag
            this->setZ(false);
            this->setH(false);
            this->setN(false);
            return 4;
        }

        case 0x08: { // LD (u16),SP
            uint16_t word = this->fetchWord();
            this->writeWord(word, this->SP);
            return 20;  
        }

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
            this->writeByte(this->getDE(), this->A);
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
            uint16_t hl = this->getHL();
            uint16_t de = this->getDE();
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
            if (!this->getZ()) { // Z is false, jump
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
            this->writeByte(hl, this->A);
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
            int a = this->A;

            if (!this->getN()) { // addition
                // lower nibble
                if (this->getH() || (a & 0x0F) > 0x09) {
                    a += 0x06;
                }
                
                // upper nibble
                if (this->getC() || this->A > 0x99) {
                    a += 0x60;
                    this->setC(true);
                }
            }
            else { // substraction
                // lower nibble
                if (this->getH()) {
                    a -= 0x06;
                }
    
                // upper nibble
                if (this->getC()) {
                    a -= 0x60;
                }
            }
            
            this->setH(false);
            this->A = static_cast<uint8_t>(a & 0xFF);
            this->setZ(this->A == 0);
            return 4;
        }
        
        case 0x28: { // JR Z,i8
            int8_t offset = static_cast<int8_t>(this->fetchByte());
            if (this->getZ()) { // Z is true, jump
                this->PC += offset;
                return 12;
            }
            else { // Z is false, no jump
                return 8;
            }
        }

        case 0x29: {// ADD HL,HL
            uint16_t hl = this->getHL();
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
            if (!this->getC()) { // C is false, jump
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
            this->writeByte(hl, this->A);
            this->setHL(hl - 1);
            return 8;
        }

        case 0x33: // INC SP
            this->SP++;
            return 8;
        
        case 0x34: { // INC (HL)
            uint16_t hl = this->getHL();
            uint8_t data = this->bus.read(hl);

            this->setH((data & 0x0F) == 0x0F);
            data++;
            this->writeByte(hl, data);
            this->setZ(data == 0);
            this->setN(false);
            return 12;
        }
        case 0x35: { // DEC (HL)
            uint16_t hl = this->getHL();
            uint8_t data = this->bus.read(hl);

            this->setH((data & 0x0F) == 0x00);
            data--;
            this->writeByte(hl, data);
            this->setZ(data == 0);
            this->setN(true);
            return 12;
        }

        case 0x36: { // LD (HL),u8
            uint16_t hl = this->getHL();

            this->writeByte(hl,this->fetchByte());
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
            if (this->getC()) { // C is true, jump
                this->PC += offset;
                return 12;
            }
            else { // Z is false, no jump
                return 8;
            }
        }
        case 0x39: {// ADD HL,SP
            uint16_t hl = this->getHL();
            this->setH((hl & 0x0FFF) + (this->SP & 0x0FFF) > 0x0FFF);
            this->setC(hl + this->SP > 0xFFFF);
            this->setHL(hl + this->SP);
            this->setN(false);
            return 8;
            }
            
        case 0x3A: {// LD A,(HL-)
            uint16_t hl = this->getHL();
            this->A = this->bus.read(hl);
            this->setHL(hl - 1); // decrement HL by 1
            return 8;
            }
            
        case 0x3B: // DEC SP
            this->SP--;
            return 8;
        
        case 0x3C: // INC A
            this->setH((this->A & 0x0F) == 0x0F);
            this->A++;
            this->setZ(this->A == 0);
            this->setN(false);
            return 4;
        
        case 0x3D: // DEC A
            this->setH((this->A & 0x0F) == 0x00);
            this->A--;
            this->setZ(this->A == 0);
            this->setN(true);
            return 4;

        case 0x3E: // LD A,u8
            this->A = fetchByte();
            return 8;        
         
        case 0x3F: { // CCF (Complement Carry Flag)
            this->setH(false);
            this->setN(false);
            this->setC(!this->getC()); // complement C
            return 4;
        }

        case 0x40: // LD B,B
            // this->B = this->B; is not possible in C++, equivalent to NOP
            return 4;

        case 0x41: // LD B,C
            this->B = this->C;
            return 4;

        case 0x42: // LD B,D
            this->B = this->D;
            return 4;
  
        case 0x43: // LD B,E
            this->B = this->E;
            return 4;
  
        case 0x44: // LD B,H
            this->B = this->H;
            return 4;
  
        case 0x45: // LD B,L
            this->B = this->L;
            return 4;
  
        case 0x46: // LD B,(HL)
            this->B = this->bus.read(this->getHL());
            return 8;

        case 0x47: // LD B,A
            this->B = this->A;
            return 4;

        case 0x48: // LD C,B
            this->C = this->B;
            return 4;

        case 0x49: // LD C,C
            // this->C = this->B; is not possible in C++, equivalent to NOP
            return 4;

        case 0x4A: // LD C,D
            this->C = this->D;
            return 4;
  
        case 0x4B: // LD C,E
            this->C = this->E;
            return 4;
  
        case 0x4C: // LD C,H
            this->C = this->H;
            return 4;
  
        case 0x4D: // LD C,L
            this->C = this->L;
            return 4;
  
        case 0x4E: // LD C,(HL)
            this->C = this->bus.read(this->getHL());
            return 8;

        case 0x4F: // LD C,A
            this->C = this->A;
            return 4;

        case 0x50: // LD D,B
            this->D = this->B;
            return 4;

        case 0x51: // LD D,C
            this->D = this->C;
            return 4;

        case 0x52: // LD D,D
            // this->D = this->D; is not possible in C++, equivalent to NOP
            return 4;
  
        case 0x53: // LD D,E
            this->D = this->E;
            return 4;
  
        case 0x54: // LD D,H
            this->D = this->H;
            return 4;
  
        case 0x55: // LD D,L
            this->D = this->L;
            return 4;
  
        case 0x56: // LD D,(HL)
            this->D = this->bus.read(this->getHL());
            return 8;

        case 0x57: // LD D,A
            this->D = this->A;
            return 4;

        case 0x58: // LD E,B
            this->E = this->B;
            return 4;

        case 0x59: // LD E,C
            this->E = this->C;
            return 4;

        case 0x5A: // LD E,D
            this->E = this->D;
            return 4;
  
        case 0x5B: // LD E,E
            // this->E = this->E; is not possible in C++, equivalent to NOP
            return 4;
  
        case 0x5C: // LD E,H
            this->E = this->H;
            return 4;
  
        case 0x5D: // LD E,L
            this->E = this->L;
            return 4;
  
        case 0x5E: // LD E,(HL)
            this->E = this->bus.read(this->getHL());
            return 8;

        case 0x5F: // LD E,A
            this->E = this->A;
            return 4;

        case 0x60: // LD H,B
            this->H = this->B;
            return 4;

        case 0x61: // LD H,C
            this->H = this->C;
            return 4;

        case 0x62: // LD H,D
            this->H = this->D; 
            return 4;
  
        case 0x63: // LD H,E
            this->H = this->E;
            return 4;
  
        case 0x64: // LD H,H
            // this->H = this->H; is not possible in C++, equivalent to NOP
            return 4;
  
        case 0x65: // LD H,L
            this->H = this->L;
            return 4;
  
        case 0x66: // LD H,(HL)
            this->H = this->bus.read(this->getHL());
            return 8;

        case 0x67: // LD H,A
            this->H = this->A;
            return 4;

        case 0x68: // LD L,B
            this->L = this->B;
            return 4;

        case 0x69: // LD L,C
            this->L = this->C;
            return 4;

        case 0x6A: // LD L,D
            this->L = this->D;
            return 4;
  
        case 0x6B: // LD L,E
            this->L = this->E;
            return 4;
  
        case 0x6C: // LD L,H
            this->L = this->H;
            return 4;
  
        case 0x6D: // LD L,L
            // this->L = this->L; is not possible in C++, equivalent to NOP
            return 4;
  
        case 0x6E: // LD L,(HL)
            this->L = this->bus.read(this->getHL());
            return 8;

        case 0x6F: // LD L,A
            this->L = this->A;
            return 4;

        case 0x70: // LD (HL),B
            this->writeByte(getHL(), this->B);
            return 8;
  
        case 0x71: // LD (HL),C
            this->writeByte(getHL(), this->C);
            return 8;
    
        case 0x72: // LD (HL),D
            this->writeByte(getHL(), this->D);
            return 8;
  
        case 0x73: // LD (HL),E
            this->writeByte(getHL(), this->E);
            return 8;
  
        case 0x74: // LD (HL),H
            this->writeByte(getHL(), this->H);
            return 8;
  
        case 0x75: // LD (HL),L
            this->writeByte(getHL(), this->L);
            return 8;
  
        case 0x76: // HALT
            // stops the execution of the program without changing the clock frequency
            this->isHalted = true;
            return 4;
  
        case 0x77: // LD (HL),A
            this->writeByte(getHL(), this->A);
            return 8;

        case 0x78: // LD A,B
            this->A = this->B;
            return 4;

        case 0x79: // LD A,C
            this->A = this->C;
            return 4;

        case 0x7A: // LD A,D
            this->A = this->D;
            return 4;
  
        case 0x7B: // LD A,E
            this->A = this->E;
            return 4;
  
        case 0x7C: // LD A,H
            this->A = this->H;
            return 4;
  
        case 0x7D: // LD A,L
            this->A = this->L; 
            return 4;
  
        case 0x7E: // LD A,(HL)
            this->A = this->bus.read(this->getHL());
            return 8;

        case 0x7F: // LD A,A
            // this->A = this->A; is not possible in C++, equivalent to NOP
            return 4;

        case 0x80: // ADD A,B
            return this->ADD(this->B);
  
        case 0x81: // ADD A,C
            return this->ADD(this->C);
  
        case 0x82: // ADD A,D
            return this->ADD(this->D);
  
        case 0x83: // ADD A,E
            return this->ADD(this->E);
 
        case 0x84: // ADD A,H
            return this->ADD(this->H);

        case 0x85: // ADD A,L
            return this->ADD(this->L);
 
        case 0x86: { // ADD A,(HL)
            uint8_t data = this->bus.read(this->getHL());
            return this->ADD(data) + 4;
        }

        case 0x87: // ADD A,A
            return this->ADD(this->A);
 
        case 0x88: // ADC A,B
            return this->ADC(this->B);
  
        case 0x89: // ADC A,C
            return this->ADC(this->C);
  
        case 0x8a: // ADC A,D
            return this->ADC(this->D);
  
        case 0x8B: // ADC A,E
            return this->ADC(this->E);
  
        case 0x8C: // ADC A,H
            return this->ADC(this->H);
  
        case 0x8D: // ADC A,L
            return this->ADC(this->L);
  
        case 0x8E: { // ADC A,(HL)
            uint8_t data = this->bus.read(this->getHL());
            return this->ADC(data) + 4;
        }

        case 0x8F: // ADC A,A
            return this->ADC(this->A);
 
        case 0x90: // SUB A,B
            return this->SUB(this->B);
  
        case 0x91: // SUB A,C
            return this->SUB(this->C);
  
        case 0x92: // SUB A,D
            return this->SUB(this->D);
  
        case 0x93: // SUB A,E
            return this->SUB(this->E);
 
        case 0x94: // SUB A,H
            return this->SUB(this->H);

        case 0x95: // SUB A,L
            return this->SUB(this->L);
 
        case 0x96: { // SUB A,(HL)
            uint8_t data = this->bus.read(this->getHL());
            return this->SUB(data) + 4;
        }

        case 0x97: // SUB A,A
            return this->SUB(this->A);
 
        case 0x98: // SBC A,B
            return this->SBC(this->B);
  
        case 0x99: // SBC A,C
            return this->SBC(this->C);
  
        case 0x9a: // SBC A,D
            return this->SBC(this->D);
  
        case 0x9B: // SBC A,E
            return this->SBC(this->E);
  
        case 0x9C: // SBC A,H
            return this->SBC(this->H);
  
        case 0x9D: // SBC A,L
            return this->SBC(this->L);
  
        case 0x9E: { // SBC A,(HL)
            uint8_t data = this->bus.read(this->getHL());
            return this->SBC(data) + 4;
        }

        case 0x9F: // SBC A,A
            return this->SBC(this->A);

        case 0xA0: // AND A,B
            return this->AND(this->B);
  
        case 0xA1: // AND A,C
            return this->AND(this->C);
  
        case 0xA2: // AND A,D
            return this->AND(this->D);
  
        case 0xA3: // AND A,E
            return this->AND(this->E);
  
        case 0xA4: // AND A,H
            return this->AND(this->H);
  
        case 0xA5: // AND A,L
            return this->AND(this->L);
  
        case 0xA6: { // AND A,(HL)
            uint8_t data = this->bus.read(this->getHL());
            return this->AND(data) + 4;
        }
  
        case 0xA7: // AND A,A
            return this->AND(this->A);
  
        case 0xA8: // XOR A,B
            return this->XOR(this->B);
  
        case 0xA9: // XOR A,C
            return this->XOR(this->C);
  
        case 0xAa: // XOR A,D
            return this->XOR(this->D);
  
        case 0xAB: // XOR A,E
            return this->XOR(this->E);
  
        case 0xAC: // XOR A,H
            return this->XOR(this->H);
  
        case 0xAD: // XOR A,L
            return this->XOR(this->L);
  
        case 0xAE: { // XOR A,(HL)
            uint8_t data = this->bus.read(this->getHL());
            return this->XOR(data) + 4;
        }
  
        case 0xAF: // XOR A,A
            return this->XOR(this->A);
  
        case 0xB0: // OR A,B
            return this->OR(this->B);
  
        case 0xB1: // OR A,C
            return this->OR(this->C);
  
        case 0xB2: // OR A,D
            return this->OR(this->D);
  
        case 0xB3: // OR A,E
            return this->OR(this->E);
  
        case 0xB4: // OR A,H
            return this->OR(this->H);
  
        case 0xB5: // OR A,L
            return this->OR(this->L);
  
        case 0xB6: { // OR A,(HL)
            uint8_t data = this->bus.read(this->getHL());
            return this->OR(data) + 4;
        }
  
        case 0xB7: // OR A,A
            return this->OR(this->A);
  
        case 0xB8: // CP A,B
            return this->CP(this->B);
  
        case 0xB9: // CP A,C
            return this->CP(this->C);
  
        case 0xBA: // CP A,D
            return this->CP(this->D);
  
        case 0xBB: // CP A,E
            return this->CP(this->E);
  
        case 0xBC: // CP A,H
            return this->CP(this->H);
  
        case 0xBD: // CP A,L
            return this->CP(this->L);
  
        case 0xBE: { // CP A,(HL)
            uint8_t data = this->bus.read(this->getHL());
            return this->CP(data) + 4;
        }
  
        case 0xBF: // CP A,A
            return this->CP(this->A);
  
        case 0xC0: // RET NZ
            if (!this->getZ()) {
                this->PC = this->POP();
                return 20;
            } else {
                return 8;
            }
  
        case 0xC1: // POP BC
            this->setBC(this->POP());
            return 12;

        case 0xC2: { // JP NZ,u16
            uint16_t word = this->fetchWord();
            if (!this->getZ()) {
                this->PC = word;
                return 16;
            } else {
                return 12;
            }
        }

        case 0xC3: // JP u16
            this->PC = this->fetchWord(); // PC -> u16
            return 16;
  
        case 0xC4: { // CALL NZ,u16
            uint16_t word = this->fetchWord();
            if (!this->getZ()) {
                this->PUSH(this->PC);
                this->PC = word;
                return 24;
            } else {
                return 12;
            }
        }

        case 0xC5: // PUSH BC
            this->PUSH(this->getBC());
            return 16;
  
        case 0xC6: // ADD A,u8
            return this->ADD(this->fetchByte()) + 4;

        case 0xC7: // RST 00h
            this->PUSH(this->PC);
            this->PC = 0x0000;
            return 16;
  
        case 0xC8: // RET Z
            if (this->getZ()) {
                this->PC = this->POP();
                return 20;
            } else {
                return 8;
            }

        case 0xC9: // RET
            this->PC = this->POP();
            return 16;            
  
        case 0xCA: { // JP Z,u16
            uint16_t word = this->fetchWord();
            if (this->getZ()) {
                this->PC = word;
                return 16;
            } else {
                return 12;
            }
        }

        case 0xCB: // PREFIX CB
            return this->executePrefix(this->fetchByte());
  
        case 0xCC: { // CALL Z,u16
            uint16_t word = this->fetchWord();
            if (this->getZ()) {
                this->PUSH(this->PC);
                this->PC = word;
                return 24;
            } else {
                return 12;
            }
        }
        case 0xCD: { // CALL u16
            uint16_t word = this->fetchWord();
            this->PUSH(this->PC);
            this->PC = word;
            return 24;            
        }

        case 0xCE: // ADC A,u8
            return this->ADC(this->fetchByte()) + 4;

        case 0xCF: // RST 08h
            this->PUSH(this->PC);
            this->PC = 0x0008;
            return 16;
  
        case 0xD0: // RET NC
            if (!this->getC()) {
                this->PC = POP();
                return 20;
            } else {
                return 8;
            }
        case 0xD1: // POP DE
            this->setDE(this->POP());
            return 12;

        case 0xD2: { // JP NC,u16
            uint16_t word = this->fetchWord();
            if (!this->getC()) {
                this->PC = word;
                return 16;
            } else {
                return 12;
            }
        }

        case 0xD3: // void
  
        case 0xD4: { // CALL NC,u16
            uint16_t word = this->fetchWord();
            if (!this->getC()) {
                this->PUSH(this->PC);
                this->PC = word;
                return 24;
            } else {
                return 12;
            }
        }

        case 0xD5: // PUSH DE
            this->PUSH(this->getDE());
            return 16;
  
        case 0xD6: // SUB A,u8
            return this->SUB(this->fetchByte()) + 4;

        case 0xD7: // RST 10h
            this->PUSH(this->PC);
            this->PC = 0x0010;
            return 16;
  
        case 0xD8: // RET C
            if (this->getC()) {
                this->PC = this->POP();
                return 20;
            } else {
                return 8;
            }

        case 0xD9: // RETI
            this->PC = this->POP();
            this->eiDelay = 2; // enable interrupts after 2 cycles;
            return 16;            
  
        case 0xDA: { // JP C,u16
            uint16_t word = this->fetchWord();
            if (this->getC()) {
                this->PC = word;
                return 16;
            } else {
                return 12;
            }
        }

        case 0xDB: // void
  
        case 0xDC: { // CALL C,u16
            uint16_t word = this->fetchWord();
            if (this->getC()) {
                this->PUSH(this->PC);
                this->PC = word;
                return 24;
            } else {
                return 12;
            }
        }
        case 0xDD: // void

        case 0xDE: // SBC A,u8
            return this->SBC(this->fetchByte()) + 4;

        case 0xDF: // RST 18h
            this->PUSH(this->PC);
            this->PC = 0x0018;
            return 16;

        case 0xE0: { // LD (FF00+u8),A
            uint8_t byte = this->fetchByte();
            this->writeByte((0xFF00 + byte), this->A);
            return 12;
        }

        case 0xE1: // POP HL
            this->setHL(this->POP());
            return 12;

        case 0xE2: // LD (FF00+C),A
            this->writeByte(0xFF00 + this->C, this->A);
            return 8;
  
        case 0xE3: // void
  
        case 0xE4: // void
  
        case 0xE5: // PUSH HL
            this->PUSH(this->getHL());
            return 16;
  
        case 0xE6: // AND A,u8
            return this->AND(this->fetchByte()) + 4;
  
        case 0xE7: // RST 20h
            this->PUSH(this->PC);
            this->PC = 0x0020;
            return 16;
            
        case 0xE8: { // ADD SP,i8
            uint8_t rawByte = this->fetchByte();
            // get 8-bit signed integer i8
            int8_t byte = static_cast<int8_t>(rawByte);

            this->setZ(false);
            this->setN(false);  
            this->setH((this->SP & 0x0F) + (rawByte & 0x0F) > 0x0F);
            this->setC((this->SP & 0xFF) + rawByte > 0xFF);

            this->SP += static_cast<int16_t>(byte);
            return 16;
        }

        case 0xE9: // JP HL
            this->PC = this->getHL();
            return 4;
  
        case 0xEA: { // LD (u16),A
            uint16_t word = this->fetchWord();
            this->writeByte(word, this->A);
            return 16;
        }

        case 0xEB: // void
  
        case 0xEC: // void
  
        case 0xED: // void
  
        case 0xEE: // XOR A,u8
            return this->XOR(this->fetchByte()) + 4;
            
        case 0xEF: // RST 28h
            this->PUSH(this->PC);
            this->PC = 0x0028;
            return 16;
            
        case 0xF0: { // LD A,(FF00+u8)
            uint8_t byte = this->fetchByte();
            uint8_t val = this->bus.read(0xFF00 + byte);
            this->A = val;
            return 12;
        }
  
        case 0xF1: // POP AF
            this->setAF(this->POP()); // lowest nibble of F is only zeros
            return 12;

        case 0xF2: { // LD A,(FF00+C)
            uint8_t val = this->bus.read(0xFF00 + this->C);
            this->A = val;
            return 12;
        }
  
        case 0xF3: // DI
            this->IME = false;
            this->eiDelay = 0; // disable interrupts immediately
            return 4;
  
        case 0xF4: // void
  
        case 0xF5: // PUSH AF
            this->PUSH(this->getAF() & 0xFFF0); // lowest nibble of F is only zeros
            return 16;
  
        case 0xF6: // OR A,u8
            return this->OR(this->fetchByte()) + 4;
  
        case 0xF7: // RST 30h
            this->PUSH(this->PC);
            this->PC = 0x0030;
            return 16;
  
        case 0xF8: { // LD HL,SP+i8
            uint8_t rawByte = this->fetchByte();
            int8_t byte = static_cast<int8_t>(rawByte);

            this->setZ(false);
            this->setN(false);  
            this->setH((this->SP & 0x000F) + (rawByte & 0x0F) > 0x0F);
            this->setC((this->SP & 0x00FF) + rawByte > 0xFF);

            this->setHL(this->SP + byte);
            return 12;
        }

        case 0xF9: // LD SP,HL
            this->SP = this->getHL();
            return 8;
  
        case 0xFA: { // LD A,(u16)
            uint16_t word = this->fetchWord();
            this->A = this->bus.read(word);
            return 16;
        }

        case 0xFB: // EI
            this->eiDelay = 2; // enable interrupts after 2 cycles;
            return 4;
  
        case 0xFC: // void
  
        case 0xFD: // void
  
        case 0xFE: // CP A,u8
            return this->CP(this->fetchByte()) + 4;
        
        case 0xFF: // RST 38h
            this->PUSH(this->PC);
            this->PC = 0x0038;
            return 16;

        default:
            std::cout << "Unhandled opcode: 0x" << std::hex << static_cast<int>(instr) << " at PC=0x" << (this->PC - 1) << std::dec << std::endl;
            return 0;
    }
}


uint8_t CPU::executePrefix(uint8_t instr) {

    // Separate the switch in 4 groups (SHIFT, BIT, RES, SET)
    uint8_t group = (instr >> 6) & 0x03; // keep 2 MSB of instr (bit7-6)
    uint8_t bit = (instr >> 3) & 0x07; // only (bit5-3)
    uint8_t reg = instr & 0x07; // keep 3 LSB of instr to get B,C,D,E,H,L,(HL),A (bit2-0)

    uint8_t data = this->getReg(reg);

    // Compute new registers and update flags
    switch (group) {
        case 0: { // SHIFT
            uint8_t res = 0;
            bool carry = false;

            switch (bit) {
                case 0: // RLC
                    carry = (data & 0x80) != 0;
                    res = (data << 1) | (carry ? 1 : 0);
                    break;

                case 1: // RRC
                    carry = (data & 0x01) != 0;
                    res = (data >> 1) | (carry ? 0x80 : 0);
                    break;

                case 2: // RL
                    carry = (data & 0x80) != 0;
                    res = (data << 1) | (this->getC() ? 1 : 0);
                    break;

                case 3: // RR
                    carry = (data & 0x01) != 0;
                    res = (data >> 1) | (this->getC() ? 0x80 : 0);
                    break;

                case 4: // SLA
                    carry = (data & 0x80) != 0;
                    res = data << 1;
                    break;

                case 5: // SRA (keep bit 7)
                    carry = (data & 0x01) != 0;
                    res = (data >> 1) | (data & 0x80);
                    break;

                case 6: // SWAP
                    carry = false;
                    res = (data >> 4) | (data << 4);
                    break;

                case 7: // SRL
                    carry = (data & 0x01) != 0;
                    res = data >> 1;
                    break;

                default:
                    break;
            }
            this->setZ(res == 0);
            this->setN(false);
            this->setH(false);
            this->setC(carry);

            this->setReg(reg, res);
            break;
        }
        case 1: // BIT b,r
            this->setZ((data & (1 << bit)) == 0);
            this->setN(false);
            this->setH(true);
            break;

        case 2: { // RES b,r
            uint8_t newData = data & ~(1 << bit);
            this->setReg(reg, newData);
            break;
        }
        
        case 3: { // SET b,r
            uint8_t newData = data | (1 << bit);
            this->setReg(reg, newData);
            break;
        }

        default:
            break;
    }

    // Return clock cycles
    if (reg == 6) { // reg = (HL)
        if (group == 1) {
            return 12;
        } 
        else {
            return 16;
        }
    } 
    else {
        return 8; // reg = B,C,D,E,H,L,A
    }
}

uint8_t CPU::handleInterrupts() {
    uint8_t IE = this->bus.read(0xFFFF); // Interrupt Enable Register
    uint8_t IF = this->bus.read(0xFF0F); // Interrupt
    uint8_t pendingInterrupts = IE & IF & 0x1F; // Only consider the lower 5 bits

    if (!pendingInterrupts) {
        return 0; // No interrupts to handle
    }

    if (this->isHalted) {
        this->isHalted = false; // Exit halt state if an interrupt is pending
    }

    if (!this->IME) {
        return 0; // Interrupts are disabled
    }

    this->IME = false; // Disable further interrupts

    for (size_t i = 0; i < 5; i++)
    {
        if (pendingInterrupts & (1 << i)) {
            this->bus.write(0xFF0F, IF & ~(1 << i)); // Clear the interrupt flag
            this->PUSH(this->PC); // Save current PC
            this->PC = 0x0040 + (i * 8); // Jump to interrupt vector
            break; // Handle only one interrupt at a time
        }
    }
    
    return 20; // Return 20 cycles for the interrupt handling
}

uint8_t CPU::ADD(uint8_t reg) {
    this->setH((this->A & 0x0F) + (reg & 0x0F) > 0x0F);
    this->setC(static_cast<uint16_t>(this->A) + static_cast<uint16_t>(reg) > 0xFF);
    this->A += reg;
    this->setZ(this->A == 0);
    this->setN(false);
    return 4;
}

uint8_t CPU::ADC(uint8_t reg) {
    bool carry = this->getC();
    this->setH((this->A & 0x0F) + (reg & 0x0F) + carry > 0x0F);
    this->setC(static_cast<uint16_t>(this->A) + static_cast<uint16_t>(reg) + carry > 0xFF);
    this->A += reg + carry;
    this->setZ(this->A == 0);
    this->setN(false);
    return 4;
}

uint8_t CPU::SUB(uint8_t reg) {
    this->setH((this->A & 0x0F) < (reg & 0x0F));
    this->setC(this->A < reg);
    this->A -= reg;
    this->setZ(this->A == 0);
    this->setN(true);
    return 4;
}

uint8_t CPU::SBC(uint8_t reg) {
    int carry = this->getC() ? 1 : 0;
    int a = this->A;
    int b = reg;
    this->setH((a & 0x0F) - (b & 0x0F) - carry < 0);
    this->setC(a - b - carry < 0);
    this->A = static_cast<uint8_t>(a - b - carry);
    this->setZ(this->A == 0);
    this->setN(true);
    return 4;
}

uint8_t CPU::AND(uint8_t reg) {
    this->A &= reg;
    this->setH(true);
    this->setN(false);
    this->setZ(this->A == 0);
    this->setC(false);
    return 4;
}

uint8_t CPU::XOR(uint8_t reg) {
    this->A ^= reg;
    this->setH(false);
    this->setN(false);
    this->setZ(this->A == 0);
    this->setC(false);
    return 4;
}

uint8_t CPU::OR(uint8_t reg) {
    this->A |= reg;
    this->setH(false);
    this->setN(false);
    this->setZ(this->A == 0);
    this->setC(false);
    return 4;
}

uint8_t CPU::CP(uint8_t reg) {
    this->setH((this->A & 0x0F) < (reg & 0x0F));
    this->setC(this->A < reg);
    this->setZ(this->A == reg);
    this->setN(true);
    return 4;
}

void CPU::PUSH(uint16_t reg) {
    uint8_t lower = static_cast<uint8_t>(reg & 0x00FF);
    uint8_t upper = static_cast<uint8_t>((reg >> 8) & 0x00FF);
    this->writeByte(--this->SP, upper);
    this->writeByte(--this->SP, lower);
    return;
}

uint16_t CPU::POP() {
    uint8_t lower = this->bus.read(this->SP++);
    uint8_t upper = this->bus.read(this->SP++);
    return static_cast<uint16_t>(upper << 8 | lower);
}
