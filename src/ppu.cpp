#include "../include/ppu.hpp"

PPU::PPU(Bus& bus) : bus(bus) {
    // LCDC register
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



