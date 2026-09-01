#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include <iostream>
#include <fstream>
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
        uint16_t getROMSize();
        uint16_t getRAMSize();
        std::string getDestinationCode();
        std::string getMaskROMVersion();
        bool globalChecksum();

    private:
        std::string filepath;
        std::vector<uint8_t> data;
    };