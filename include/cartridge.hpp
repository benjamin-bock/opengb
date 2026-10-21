#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <string>

enum class MBCType {
    None,
    MBC1,
    MBC2,
    MBC3,
    MBC5
};

class Cartridge {
    public:
        Cartridge(const std::string& filepath);
        ~Cartridge();

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

        void saveRAM();
        MBCType getMBCType() const;

    private:
        std::string filepath;
        std::vector<uint8_t> data;
        std::vector<uint8_t> ram;

        MBCType mbcType;

        bool ramEnabled;
        bool bankingMode; // MBC1: 0 = ROM only, 1 = RAM banking

        uint16_t romBank;     // current ROM bank for 0x4000-0x7FFF (9 bits for MBC5)
        uint8_t romBank2;     // MBC1: upper 2 bits used for RAM banking / large ROMs
        uint8_t ramBank;      // current RAM bank
        uint8_t rtcRegister;  // MBC3: selected RTC register (0x08-0x0C)

        std::array<uint8_t, 5> rtc; // MBC3 RTC: seconds, minutes, hours, days low, days high/flags

        void detectMBC();
        void loadRAM();
        std::string saveFilePath() const;
        uint8_t readRAM(uint16_t addr) const;
        void writeRAM(uint16_t addr, uint8_t data);
};
