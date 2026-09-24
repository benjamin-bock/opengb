#include "../include/ppu.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <sys/types.h>

#include "../include/bus.hpp"

static constexpr uint8_t MODE_2_LIMIT = 80;
static constexpr uint16_t MODE_3_LIMIT = 252; // 80 + 172 = 252
static constexpr uint16_t SCANLINE_LENGTH = 456;

static constexpr uint32_t COLOR_PALETTE[4] = {
    0xFFFFFFFF, // 0: White
    0xFFAAAAAA, // 1: Light gray
    0xFF555555, // 2: Dark gray
    0xFF000000  // 3: Black
};

static constexpr uint16_t OAM_MEMORY_ADDRESS = 0xFE00;

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
        this->windowLineCounter = 0;
        this->windowYTriggered = false;
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
        this->frameReady = true;
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
    uint16_t offsetTM = this->getBgTileMap() ? 0x0400 : 0x0000;
    bool isUnsignedMode = this->getAddrMode();

    uint8_t bgY = this->LY + this->SCY; // mod 256 is automatic on uint8_t
    uint16_t tileRow = bgY / 8;
    uint8_t lineOffset = (bgY % 8) << 1;

    // loop on the scanline
    for (uint8_t x = 0; x < 160; ++x) {
        // get tileID
        uint8_t bgX = (x + this->SCX); // mod 256 is automatic on uint8_t
        uint8_t tileCol = bgX / 8;

        uint16_t addrTM = (0x9800 + offsetTM) + (tileRow << 5) + tileCol;
        uint8_t tileID = this->bus.read(addrTM);

        uint16_t tileAddr;
        // find tile in VRAM
        if (isUnsignedMode) {
            // unsigned mode, base pointer at $8000
            tileAddr = 0x8000 + (tileID << 4) + lineOffset;
        }
        else {
            // signed mode, base pointer at $9000
            tileAddr = 0x9000 + (static_cast<int8_t>(tileID) * 16) + lineOffset;
        }
        // read the tile data
        uint8_t byte0 = this->bus.read(tileAddr);
        uint8_t byte1 = this->bus.read(tileAddr + 1);

        uint8_t pixelX = bgX % 8;
        uint8_t bitPos = 7 - pixelX;

        bool loBit = (byte0 >> bitPos) & 1;
        bool hiBit = (byte1 >> bitPos) & 1;
        uint8_t colorID = (hiBit << 1) | loBit;

        uint8_t shade = (this->BGP >> (colorID * 2)) & 0x03;

        // update the framebuffer and the scanline color buffer
        this->framebuffer[LY][x] = COLOR_PALETTE[shade];
        this->bgScanlineColor[x] = colorID;
    }
}

void PPU::renderWindow() {
    // check the Y condition
    if (this->LY == this->WY) {
        this->windowYTriggered = true;
    }
    // check if conditions are met
    if (!this->isWindowEnabled()) {
        return;
    }

    uint16_t offsetTM = this->getWinTileMap() ? 0x0400 : 0x0000;
    bool isUnsignedMode = this->getAddrMode();

    uint8_t winY = this->windowLineCounter; // mod 256 is automatic on uint8_t
    uint16_t tileRow = winY / 8;
    uint8_t lineOffset = (winY % 8) << 1;

    // local variable in <int> for speed in modern architecture
    int screenStartX = static_cast<int>(this->WX) - 7;
    if (screenStartX < 0) screenStartX = 0;

    // loop on the scanline
    for (int x = screenStartX; x < 160; ++x) {
        // internal window coordinate
        uint8_t winX = x - (this->WX - 7);
        uint8_t tileCol = winX / 8;

        // read tilemap
        uint16_t addrTM = (0x9800 + offsetTM) + (tileRow << 5) + tileCol;
        uint8_t tileID = this->bus.read(addrTM);

        // find tile in VRAM
        uint16_t tileAddr;
        if (isUnsignedMode) {
            // unsigned mode, base pointer at $8000
            tileAddr = 0x8000 + (tileID << 4) + lineOffset;
        }
        else {
            // signed mode, base pointer at $9000
            tileAddr = 0x9000 + (static_cast<int8_t>(tileID) * 16) + lineOffset;
        }
        // read the tile data
        uint8_t byte0 = this->bus.read(tileAddr);
        uint8_t byte1 = this->bus.read(tileAddr + 1);

        uint8_t pixelX = winX % 8;
        uint8_t bitPos = 7 - pixelX;

        bool loBit = (byte0 >> bitPos) & 1;
        bool hiBit = (byte1 >> bitPos) & 1;
        uint8_t colorID = (hiBit << 1) | loBit;

        uint8_t shade = (this->BGP >> (colorID * 2)) & 0x03;

        // update the framebuffer and the scanline color buffer
        this->framebuffer[LY][x] = COLOR_PALETTE[shade];
        this->bgScanlineColor[x] = colorID;
    }
    this->windowLineCounter++;
}

void PPU::renderSprites() {
    if (!this->isSpriteEnabled()) {
        return;
    }
    // iterate in decreasing order to get higher priority sprites (low index)
    for (int i = 39; i >= 0; --i) {
        uint16_t spriteAddr = OAM_MEMORY_ADDRESS + (i * 4);

        uint8_t spriteY = this->bus.read(spriteAddr);
        uint8_t spriteX = this->bus.read(spriteAddr + 1);
        uint8_t tileID  = this->bus.read(spriteAddr + 2);
        // Bit 7 : Priority (0 = over BG, 1 = behind BG unless color 0).
        // Bit 6 : Y flip.
        // Bit 5 : X flip.
        // Bit 4 : DMG Palette (0 = OBP0 at $FF48, 1 = OBP1 at $FF49).
        uint8_t spriteFlag = this->bus.read(spriteAddr + 3);

        
        int posY = static_cast<int>(spriteY) - 16;
        int posX = static_cast<int>(spriteX) - 8;
        uint8_t spriteHeight = this->getSpriteSize() ? 16 : 8;
        
        if (this->LY < posY || this->LY >= posY + spriteHeight) {
            continue; // Sprite is not on this scanline
        }
        
        // ignore bit0 of tileID for 8x16 sprite
        if (spriteHeight == 16) {
            tileID &= 0xFE;
        }

        uint8_t lineInSprite = this->LY - posY;

        // is Y-flip set ?
        if ((spriteFlag & 0x40) != 0) {
            lineInSprite = (spriteHeight - 1) - lineInSprite;
        }
        
        uint16_t tileAddr = 0x8000 + (tileID * 16) + (lineInSprite * 2);
    
        uint8_t byte0 = this->bus.read(tileAddr);
        uint8_t byte1 = this->bus.read(tileAddr + 1);

        for (int pixel = 0; pixel < 8; ++pixel) {
            int screenX = posX + pixel;
        
            // Verify if pixel is visible on screen (0 to 159)
            if (screenX < 0 || screenX >= 160) {
                continue;
            }
        
            // X-Flip management (bit 5)
            int bitPos = (spriteFlag & (1 << 5)) ? pixel : (7 - pixel);
        
            uint8_t loBit = (byte0 >> bitPos) & 1;
            uint8_t hiBit = (byte1 >> bitPos) & 1;
            uint8_t spriteColorID = (hiBit << 1) | loBit;
        
            // Couleur 0 = TRANSPARENTE pour les sprites !
            if (spriteColorID == 0) {
                continue;
            }
        
            // Choix de la palette (Bit 4 : 0 = OBP0, 1 = OBP1)
            uint8_t palette = (spriteFlag & (1 << 4)) ? this->OBP1 : this->OBP0;
            uint8_t spriteShade = (palette >> (spriteColorID * 2)) & 0x03;
        
            // Priorité par rapport au BG
            bool bgPriority = (spriteFlag & 0x80);
            if (!bgPriority || this->bgScanlineColor[screenX] == 0) {
                this->framebuffer[this->LY][screenX] = COLOR_PALETTE[spriteShade];
            }
        }
    }
}

bool PPU::isWindowEnabled() {
    return (this->LCDC & 0x20) && this->windowYTriggered && (this->WX <= 166) && (this->WY <= 143);
}

bool PPU::isSpriteEnabled() {
    return (this->LCDC & 0x02) != 0;
}

// if 0 : 8x8, if 1 : 8x16
bool PPU::getSpriteSize() {
    return (this->LCDC & 0x04) != 0;
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

bool PPU::isFrameReady() {
    return this->frameReady;
}

void PPU::clearFrameReady() {
    this->frameReady = false;
    return;
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

const uint32_t* PPU::getFramebuffer() const {
    return &this->framebuffer[0][0];
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



