#include "../include/bus.hpp"

#include "../include/cartridge.hpp"
#include "../include/serial.hpp"


Bus::Bus(Cartridge& cart) : cart(cart) {
    this->vram.fill(0x00);
    this->wram0.fill(0x00);
    this->wram1.fill(0x00);
    this->oam.fill(0x00);
    this->hram.fill(0x00);
    this->ieRegister = 0x00;
    this->dmaRegister = 0xFF;
    this->interruptFlag = 0x00;
}
        
bool Bus::isAddressValid(uint16_t addr) const {
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
           (addr >= 0xFF00));
};

uint8_t Bus::read(uint16_t addr) const {
    if (!(this->isAddressValid(addr)))
        return 0xFF;

    // ROM bank 0
    if (addr <= 0x7FFF || (addr >= 0xA000 && addr <= 0xBFFF)) {
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
        return this->ieRegister;
    }
    else {
        return 0xFF; // Unmapped address
    }
}

void Bus::write(uint16_t addr, uint8_t data) {
    if (!(this->isAddressValid(addr)))
        return;

    // ROM bank 0
    if (addr <= 0x7FFF || (addr >= 0xA000 && addr <= 0xBFFF)) {
        return this->cart.write(addr, data);
    }
    // ROM bank 1-n
    else if (addr >= 0x8000 && addr <= 0x9FFF) {
        this->vram[addr - 0x8000] = data;
        return;
    }
    // WRAM bank 0
    else if (addr >= 0xC000 && addr <= 0xCFFF) {
        this->wram0[addr - 0xC000] = data;
        return;
    }
    // WRAM bank 1
    else if (addr >= 0xD000 && addr <= 0xDFFF) {
        this->wram1[addr - 0xD000] = data;
        return;
    }
    // Echo RAM
    else if (addr >= 0xE000 && addr <= 0xFDFF) { // Deprecated address range
        this->write(addr - 0x2000, data); // Echo RAM
        return;
    }
    // OAM
    else if (addr >= 0xFE00 && addr <= 0xFE9F) {
        this->oam[addr - 0xFE00] = data;
        return;
    }
    // IO ports
    else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        this->writeIO(addr, data);
        return;
    }
    // HRAM
    else if (addr >= 0xFF80 && addr <= 0xFFFE) {
        this->hram[addr - 0xFF80] = data;
        return;
    }
    // Interrupt enable register
    else if (addr == 0xFFFF) {
        this->ieRegister = data;
        return;
    }
    else {
        // Unmapped address
        return;
    }
}

uint8_t Bus::readIO(uint16_t addr) const {
    if (addr == 0xFF00) {
        // Joypad (not yet implemented)
        return 0xFF;
    }
    else if (addr == 0xFF01 || addr == 0xFF02) {
        return this->serial.read(addr);
    }
    else if (addr >= 0xFF04 && addr <= 0xFF07) {
        if (addr == 0xFF04) {
            static uint8_t divCounter = 0;
            return divCounter++;
        }
        return 0x00;
    }
    else if (addr == 0xFF0F) {
        return this->interruptFlag;
    }
    else if (addr >= 0xFF10 && addr <= 0xFF3F) {
        // APU (not yet implemented)
        return 0x00;
    }
    else if (addr == 0xFF46) {
        return this->dmaRegister;
    }
    else if (addr >= 0xFF40 && addr <= 0xFF4B) {
        if (addr == 0xFF44) {
            return 0x90; // Fake VBlank (Scanline 144) so ROM wait loops don't hang!
        }
        return 0x00;
    }
    else {
        return 0xFF; // Unmapped address
    }
}

void Bus::writeIO(uint16_t addr, uint8_t data) {
    if (addr == 0xFF00) {
        // Joypad (not yet implemented)
        return;
    }
    else if (addr == 0xFF01 || addr == 0xFF02) {
        this->serial.write(addr, data);
        return;
    }
    else if (addr >= 0xFF04 && addr <= 0xFF07) {
        // Timer (not yet implemented)
        return;
    }
    else if (addr == 0xFF0F) {
        this->interruptFlag = data;
        return;
    }
    else if (addr >= 0xFF10 && addr <= 0xFF3F) {
        // APU (not yet implemented)
        return;
    }
    else if (addr == 0xFF46) {
        this->dmaRegister = data;
        this->dmaTransfer(this->dmaRegister);
        return;
    }
    else if (addr >= 0xFF40 && addr <= 0xFF4B) {
        // PPU (not yet implemented)
        return;
    }
    /* Ignore the rest of the IO ports 
    ⚬	$FF50 (Boot ROM is not emulated, CPU's PC is initilised at 0x0100)   
    ⚬	$FF4C-$FF4D (KEY0/KEY1 - Vitesse CPU CGB)
    ⚬	$FF4F (VRAM Bank CGB)
    ⚬	$FF51-$FF55 (HDMA CGB)
    ⚬	$FF56 (Infrarouge)
    ⚬	$FF68-$FF6B (Palettes couleur CGB)
    ⚬	$FF6C (Priorité des sprites CGB)
    ⚬	$FF70 (WRAM Bank CGB)
    because they are only for CGB mode
    */
}

void Bus::dmaTransfer(uint8_t source_prefix) {
    uint16_t baseAddr = static_cast<uint16_t>(source_prefix) << 8;
    for (uint16_t i = 0; i < 160; i++) {
        this->write(0xFE00 + i, this->read(baseAddr + i));
    }
}