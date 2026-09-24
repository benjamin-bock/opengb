#pragma once

#include <cstdint>

class Bus;

class PPU {
    public:
        PPU(Bus& bus);
        ~PPU() = default;

        void step(uint8_t cycles);
        void setVBlank(bool data);
        bool isLCDEnabled();
        bool isWindowEnabled();
        bool isSpriteEnabled();
        bool getAddrMode();
        bool getBgTileMap();
        bool getWinTileMap();

        // getter functions
        uint8_t getLCDC() const;
        uint8_t getSTAT() const;
        uint8_t getSCY() const;
        uint8_t getSCX() const;
        uint8_t getLY() const;
        uint8_t getLYC() const;
        uint8_t getWY() const;
        uint8_t getWX() const;
        uint8_t getBGP() const;
        uint8_t getOBP0() const;
        uint8_t getOBP1() const;

        // setter functions
        void setLCDC(uint8_t data);
        void setSTAT(uint8_t data);
        void setSCY(uint8_t data);
        void setSCX(uint8_t data);
        void setLY(uint8_t data);
        void setLYC(uint8_t data);
        void setWY(uint8_t data);
        void setWX(uint8_t data);
        void setBGP(uint8_t data);
        void setOBP0(uint8_t data);
        void setOBP1(uint8_t data);

    private:
        Bus& bus;

        // Count cycles from 0 to 456 in the current scanline
        uint16_t cycleCounter;

        // LCDC register
        uint8_t LCDC;
/*      bool lcdEnable;
        bool winMap;
        bool winEnable;
        bool winAddrMode;
        bool bgMap;
        bool objSize;
        bool objEnable;
        bool enablePrio; */

        // Window coordinates
        uint8_t WY, WX;

        // LCD status register
        uint8_t LY;
        uint8_t LYC;
        uint8_t STAT;

        // Background Tilemap viewport positions
        uint8_t SCY, SCX;

        // BG palette data. Determine gray shades: 0 White, 1 Light gray, 2	Dark gray, 3 Black
        uint8_t BGP;

        // OBJ palette 0,1 data
        uint8_t OBP0, OBP1;

        // Private functions
        void setMode(uint8_t sel);
        void cmpLY();

        // Drawing
        void renderScanline();
        void renderBackground();
        void renderWindow();
        void renderSprites();

};