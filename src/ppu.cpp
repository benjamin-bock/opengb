#include "../include/ppu.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sys/types.h>

#include "../include/bus.hpp"

static constexpr uint8_t MODE_2_LIMIT = 80;
static constexpr uint16_t MODE_3_LIMIT = 252; // 80 + 172 = 252
static constexpr uint16_t SCANLINE_LENGTH = 456;

PPU::PPU(Bus& bus) : bus(bus) {
    cycleCounter = 0x0000;
    
    // LCDC register
    LCDC = 0x00;
/*    lcdEnable = false;
    winMap = false;
    winEnable = false;
    winAddrMode = false;
    bgMap = false;
    objSize = false;
    objEnable = false;
    enablePrio = false; */

    // Window coordinates
    WY = 0x00;
    WX = 0x00;

    // LCD status register
    LY = 0x00;
    LYC = 0x00;
    STAT = 0x00;;

    // Background Tilemap viewport positions
    SCY = 0x00;
    SCX = 0x00;

    // BG palette data. Determine gray shades: 0 White, 1 Light gray, 2	Dark gray, 3 Black
    BGP = 0x00;

    // OBJ palette 0,1 data
    OBP0 = 0x00;
    OBP1 = 0x00;
}

void PPU::step(uint8_t cycles) {
    this->cycleCounter += cycles;

    // check if LCD is enabled
    if (!this->isLCDEnabled()) {
        this->LY = 0;
        this->cycleCounter = 0;
        this->setMode(0);
        return;
    }
    // check end of scanline
    if (this->cycleCounter >= SCANLINE_LENGTH) {
        // reset the counter
        this->cycleCounter -= SCANLINE_LENGTH;
        this->LY++;

        // Trigger VBlank at rising edge of 144
        if (this->LY == 144) {
            this->setVBlank(true);
        }

        // LY overflow => reset
        if (this->LY > 153) {
            this->LY = 0;
        }

        // Compare LY == LYC
        this->cmpLY();
    }

    uint8_t penalty = 0;

    // Mode 2, 3 and 0
    if (this->LY <= 143) {
        // Mode 2
        if (this->cycleCounter < MODE_2_LIMIT) {
            // OAM Scan
            this->setMode(2);
        }
        // Mode 3
        else if (this->cycleCounter < MODE_3_LIMIT + penalty) {
            // Drawing pixels
            this->setMode(3);
        }
        // Mode 0
        else {
            // Horizontal blank
            this->renderScanline();
            this->setMode(0);
        }
    }
    // Mode 1
    else if (this->LY >= 144 && this->LY <= 153) {
        // Vertical blank
        this->setMode(1);
    }
    return;
}

void PPU::setMode(uint8_t sel) {
    if (sel > 3) {
        perror("function PPU::setMode(uint8_t sel) takes a 2-bit argument and cannot exceed 0b11");
        return;
    }
    // Replace 2 LSB of STAT by sel
    this->STAT = (this->STAT & 0xFC) | sel;
    return;
}

void PPU::cmpLY() {
    this->STAT = (this->STAT & 0xFB) | ((this->LY == this->LYC) << 2); // Replace bit2 by comparison result
}

void PPU::setVBlank(bool data) {
    uint8_t IF = this->bus.read(0xFF0F);

    // set bit0 of IF
    this->bus.write(0xFF0F, (IF & 0xFE) | data);
}

bool PPU::isLCDEnabled() {
    return (this->LCDC & 0x80) != 0;
}

void PPU::renderScanline() {
    // first render the background
    this->renderBackground();

    // second render the window, if enabled
    if (this->isWindowEnabled() && this->LY >= this->WY) {
        this->renderWindow();
    }

    // third render the sprites, if enabled
    if (this->isSpriteEnabled()) {
        this->renderSprites();
    }
    return;
}

void PPU::renderBackground() {
    uint8_t tilemap;
    uint16_t tileID;
    uint16_t lineOffset;
    uint16_t tileAddr;
    uint16_t tile;
    uint8_t byte0;
    uint8_t byte1;
    uint8_t pixelX;
    uint8_t bitPos;
    bool loBit;
    bool hiBit;
    uint8_t colorID;
    uint8_t shade;
    uint16_t offsetTM = this->getBgTileMap() ? 0x0400 : 0x0000;
    uint16_t offsetTD = this->getAddrMode() ? 0x1000 : 0x0000;

    uint8_t virtY = (this->LY + this->SCY); // mod 256 is automatic on uint8_t
    uint8_t virtX;
    uint16_t tileCol, tileRow;
    uint16_t addrTM;

    // loop on the scanline
    for (uint8_t x = 0; x < 160; ++x) {
        // get tileID
        virtX = (x + this->SCX); // mod 256 is automatic on uint8_t
        tileCol = virtX / 8;
        tileRow = virtY / 8;
        addrTM = (0x9800 + offsetTM) + (tileRow << 5) + tileCol;
        tileID = (this->bus.read(addrTM) << 8) | (this->bus.read(addrTM));

        // find tile in VRAM
        lineOffset = (virtX % 8) << 1;
        if (!this->getAddrMode()) {
            // unsigned mode, base pointer at $8000
            tileAddr = 0x8000 + (tileID << 4) + lineOffset;
        }
        else {
            // signed mode, base pointer at $9000
            tileAddr = 0x9000 + (static_cast<int16_t>(tileID) * 16) + lineOffset;
        }
        byte0 = this->bus.read(tileAddr);
        byte1 = this->bus.read(tileAddr + 1);

        pixelX = virtX % 8;
        bitPos = 7 - pixelX;

        loBit = (byte0 >> bitPos) & 1;
        hiBit = (byte1 >> bitPos) & 1;

        colorID = (hiBit << 1) | loBit;

        shade = (this->BGP >> (colorID * 2)) & 0x03;

    }
}

void PPU::renderWindow() {
    
}

void PPU::renderSprites() {
    
}

bool PPU::isWindowEnabled() {
    return (this->LCDC & 0x20) != 0;
}

bool PPU::isSpriteEnabled() {
    return (this->LCDC & 0x02) != 0;
}

// if false, "$8800 method" uses $9000 base pointer
// if true,  "$8000 method" uses $8000 base pointer
bool PPU::getAddrMode() {
    return (this->LCDC & 0x10) != 0;
}

// When it’s clear (0), the $9800 tilemap is used, otherwise it’s the $9C00 one.
bool PPU::getBgTileMap() {
    return (this->LCDC & 0x08) != 0;
}

// When it’s clear (0), the $9800 tilemap is used, otherwise it’s the $9C00 one.
bool PPU::getWinTileMap() {
    return (this->LCDC & 0x40) != 0;
}

// Getter functions
uint8_t PPU::getLCDC() const {
    return this->LCDC;
}

uint8_t PPU::getSTAT() const {
    return this->STAT;
}

uint8_t PPU::getLY() const {
    return this->LY;
}

uint8_t PPU::getLYC() const {
    return this->LYC;
}

uint8_t PPU::getWY() const {
    return this->WY;
}

uint8_t PPU::getWX() const {
    return this->WX;
}

uint8_t PPU::getSCY() const {
    return this->SCY;
}

uint8_t PPU::getSCX() const {
    return this->SCX;
}

uint8_t PPU::getBGP() const {
    return this->BGP;
}

uint8_t PPU::getOBP0() const {
    return this->OBP0;
}

uint8_t PPU::getOBP1() const {
    return this->OBP1;
}

// Setter functions
void PPU::setLCDC(uint8_t data) {
    this->LCDC = data;
}

void PPU::setSTAT(uint8_t data) {
    this->STAT = data;
}

void PPU::setSCY(uint8_t data) {
    this->SCY = data;
}

void PPU::setSCX(uint8_t data) {
    this->SCX = data;
}

void PPU::setBGP(uint8_t data) {
    this->BGP = data;
}

void PPU::setOBP0(uint8_t data) {
    this->OBP0 = data;
}

void PPU::setOBP1(uint8_t data) {
    this->OBP1 = data;
}

void PPU::setWY(uint8_t data) {
    this->WY = data;
}

void PPU::setWX(uint8_t data) {
    this->WX = data;
}

void PPU::setLY(uint8_t data) {
    this->LY = data;
}

void PPU::setLYC(uint8_t data) {
    this->LYC = data;
}



