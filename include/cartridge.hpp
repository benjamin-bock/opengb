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
    
    private:
        std::string filepath;
        std::vector<uint8_t> data;
    };