#pragma once

#include <cstdint>
#include <array>

#include "../include/cartridge.hpp"
#include "../include/serial.hpp"
#include "../include/timer.hpp"
#include "../include/ppu.hpp"

constexpr size_t ADDRESS_BUS_SIZE = 64 * 1024; // 64 KiB memory table
constexpr size_t VRAM_SIZE = 8 * 1024; // 8 KiB VRAM
constexpr size_t WRAM_SIZE = 4 * 1024; // 4 KiB WRAM each
constexpr size_t OAM_SIZE = 160; // 160 bytes of OAM
constexpr size_t HRAM_SIZE = 127; // 127 bytes of HRAM

class Bus {
    public:
        Bus(Cartridge& cart);
        ~Bus() = default;

        uint8_t read(uint16_t addr) const;
        void write(uint16_t addr, uint8_t data);

        void step(uint8_t cycles);
    private:
        Cartridge& cart;
        Serial serial;
        Timer timer;
        PPU ppu;

        bool isAddressValid(uint16_t addr) const;

        std::array<uint8_t, VRAM_SIZE> vram;
        std::array<uint8_t, WRAM_SIZE> wram0;
        std::array<uint8_t, WRAM_SIZE> wram1;
        std::array<uint8_t, OAM_SIZE> oam;
        std::array<uint8_t, HRAM_SIZE> hram;
        uint8_t interruptFlag;
        uint8_t dmaRegister;

        uint8_t ieRegister; // Interrupt enable register

        uint8_t readIO(uint16_t addr) const;
        void writeIO(uint16_t addr, uint8_t data);

        void dmaTransfer(uint8_t source_prefix);
};