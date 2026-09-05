#pragma once

#include <vector>
#include <cstdint>
#include <string>

class Cartridge {
    public:
        Cartridge(const std::string& filepath);
        ~Cartridge() = default;  // the vector is destroyed by itself
    
        void load();
        bool nintendoLogo();
        bool headerChecksum();
        bool isColorMode();
        bool isColorOnly();
        std::string getTitle();
        std::string getManufacturerCode();
        bool isOldLicenseeCode();
        std::string getLicenseeCode();
        bool hasSGBSupport();
        std::string getType();
        uint32_t getROMSize();
        uint32_t getRAMSize();
        std::string getDestinationCode();
        std::string getMaskROMVersion();
        bool globalChecksum();
        uint8_t read(uint16_t addr) const;
        void write(uint16_t addr, uint8_t data);

    private:
        std::string filepath;
        std::vector<uint8_t> data;
};