#include "bus.hpp"


Bus::Bus(Cartridge& cart) : cart(cart) {
    this->vram.fill(0x00);
    this->wram0.fill(0x00);
    this->wram1.fill(0x00);
    this->oam.fill(0x00);
    this->hram.fill(0x00);
    this->ie_register = 0x00;
}
        
bool Bus::isAddressValid(uint16_t addr) {
    // Validate against Game Boy address ranges:
    // - $0000-$3FFF (ROM bank 0)
    // - $4000-$7FFF (ROM bank 1-n)
    // - $8000-$9FFF (VRAM)
    // - $A000-$BFFF (External RAM)
    // - $C000-$CFFF (WRAM bank 0)
    // - $D000-$DFFF (WRAM bank 1)
    // - $E000-$FDFF (Echo RAM)
    // - $FE00-$FE9F (OAM)
    // - $FEA0-$FEFF (Unusable)
    // - $FF00-$FF7F (IO ports)
    // - $FF80-$FFFE (HRAM)
    // - $FFFF (Interrupt enable register)

    return (addr <= 0xDFFF || 
           (addr >= 0xFE00 && addr <= 0xFE9F) ||
           (addr >= 0xFF00 && addr <= 0xFFFF))
};

uint8_t Bus::read(uint16_t addr) const {
    if (!(this->isAddressValid(addr)))
        return 0xFF;

    // ROM bank 0
    if ((addr >= 0x0000 && addr <= 0x7FFF) || (addr >= 0xA000 && addr <= 0xBFFF)) {
        return this->cart.read(addr);
    }
    // ROM bank 1-n
    else if (addr >= 0x8000 && addr <= 0x9FFF) {
        return this->vram[addr - 0x8000];
    }
    // WRAM bank 0
    else if (addr >= 0xC000 && addr <= 0xCFFF) {
        return this->wram0[addr - 0xC000];
    }
    // WRAM bank 1
    else if (addr >= 0xD000 && addr <= 0xDFFF) {
        return this->wram1[addr - 0xD000];
    }
    // Echo RAM
    else if (addr >= 0xE000 && addr <= 0xFDFF) { // Deprecated address range
        return this->read(addr - 0x2000); // Echo RAM
    }
    // OAM
    else if (addr >= 0xFE00 && addr <= 0xFE9F) {
        return this->oam[addr - 0xFE00];
    }
    // IO ports
    else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        return this->readIO(addr);
    }
    // HRAM
    else if (addr >= 0xFF80 && addr <= 0xFFFE) {
        return this->hram[addr - 0xFF80];
    }
    // Interrupt enable register
    else if (addr == 0xFFFF) {
        return this->ie_register;
    }
    else {
        return 0xFF; // Unmapped address
    }
}

void Bus::write(uint16_t addr, uint8_t data) {
    if (!(this->isAddressValid(addr)))
        return;

    // ROM bank 0
    if ((addr >= 0x0000 && addr <= 0x7FFF) || (addr >= 0xA000 && addr <= 0xBFFF)) {
        return this->cart.write(addr, data);
    }
    // ROM bank 1-n
    else if (addr >= 0x8000 && addr <= 0x9FFF) {
        return this->vram[addr - 0x8000] = data;
    }
    // WRAM bank 0
    else if (addr >= 0xC000 && addr <= 0xCFFF) {
        return this->wram0[addr - 0xC000] = data;
    }
    // WRAM bank 1
    else if (addr >= 0xD000 && addr <= 0xDFFF) {
        return this->wram1[addr - 0xD000] = data;
    }
    // Echo RAM
    else if (addr >= 0xE000 && addr <= 0xFDFF) { // Deprecated address range
        return this->write(addr - 0x2000, data); // Echo RAM
    }
    // OAM
    else if (addr >= 0xFE00 && addr <= 0xFE9F) {
        return this->oam[addr - 0xFE00] = data;
    }
    // IO ports
    else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        return this->writeIO(addr, data);
    }
    // HRAM
    else if (addr >= 0xFF80 && addr <= 0xFFFE) {
        return this->hram[addr - 0xFF80] = data;
    }
    // Interrupt enable register
    else if (addr == 0xFFFF) {
        return this->ie_register = data;
    }
    else {
        return; // Unmapped address
    }
}

uint8_t Bus::readIO(uint16_t addr) const {
    // TO-DO:
}

void Bus::writeIO(uint16_t addr, uint8_t data) {
    // TO-DO:
}