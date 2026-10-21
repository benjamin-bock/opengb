#include "../include/cartridge.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>

static constexpr std::array<uint8_t, 48> NINTENDO_LOGO_REF = {
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

Cartridge::Cartridge(const std::string& filepath) {
    this->filepath = filepath;
    this->mbcType = MBCType::None;
    this->ramEnabled = false;
    this->bankingMode = false;
    this->romBank = 1;
    this->romBank2 = 0;
    this->ramBank = 0;
    this->rtcRegister = 0;
    this->rtc.fill(0x00);
}

Cartridge::~Cartridge() {
    this->saveRAM();
}

void Cartridge::detectMBC() {
    const uint8_t type = this->data[0x0147];

    switch (type) {
        case 0x00:
            this->mbcType = MBCType::None;
            break;
        case 0x01:
        case 0x02:
        case 0x03:
            this->mbcType = MBCType::MBC1;
            break;
        case 0x05:
        case 0x06:
            this->mbcType = MBCType::MBC2;
            break;
        case 0x0F:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            this->mbcType = MBCType::MBC3;
            break;
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
            this->mbcType = MBCType::MBC5;
            break;
        default:
            this->mbcType = MBCType::None;
            std::cerr << "Warning: unknown cartridge type 0x" << std::hex
                      << static_cast<int>(type) << ", falling back to no MBC" << std::endl;
            break;
    }
}

std::string Cartridge::saveFilePath() const {
    const size_t dot = this->filepath.find_last_of('.');
    if (dot != std::string::npos && dot != 0 && this->filepath.find_last_of("/\\") < dot) {
        return this->filepath.substr(0, dot) + ".sav";
    }
    return this->filepath + ".sav";
}

void Cartridge::load() {
    // load the cartridge from the file

    std::ifstream file(this->filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Unable to open the file: " + this->filepath);
    }

    std::streamsize size = file.tellg();

    
    file.seekg(0, std::ios::beg);
    
    this->data.resize(static_cast<size_t>(size));

    if (this->data.size() < 0x150) {
        throw std::runtime_error("File is too small to be a valid cartridge");
    }

    file.read(reinterpret_cast<char*>(this->data.data()), size);

    if (!file) {
        throw std::runtime_error("Unable to read the file: " + this->filepath);
    }

    this->detectMBC();

    // MBC2 always has 512 x 4 bits of on-cartridge RAM
    uint32_t ramSize = (this->mbcType == MBCType::MBC2) ? 512 : this->getRAMSize();
    if (this->mbcType == MBCType::MBC2 && this->getRAMSize() == 0) {
        ramSize = 512;
    }
    this->ram.resize(ramSize, 0x00);

    this->loadRAM();
}

void Cartridge::loadRAM() {
    std::ifstream file(this->saveFilePath(), std::ios::binary);
    if (!file) {
        return; // no save file yet
    }

    file.read(reinterpret_cast<char*>(this->ram.data()), this->ram.size());
    if (file.gcount() < static_cast<std::streamsize>(this->ram.size())) {
        std::cerr << "Warning: save file smaller than expected, missing data filled with 0x00" << std::endl;
    }
}

void Cartridge::saveRAM() {
    if (this->ram.empty()) {
        return;
    }

    std::ofstream file(this->saveFilePath(), std::ios::binary | std::ios::trunc);
    if (!file) {
        std::cerr << "Warning: unable to write the save file: " << this->saveFilePath() << std::endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(this->ram.data()), this->ram.size());
}

bool Cartridge::nintendoLogo() {
    // check if the nintendo logo is valid
    for (int i = 0; i < 48; i++) {
        if (this->data[0x0104 + i] != NINTENDO_LOGO_REF[i]) {
            return false;
        }
    }
    return true;
}

bool Cartridge::headerChecksum() {
    // check if the header checksum is valid
    uint8_t checksum = 0;

    // calculate the checksum
    for (uint16_t address = 0x0134; address <= 0x014C; address++) {
        checksum = checksum - this->data[address] - 1;
    }
    // compare the 8 LSB of checksum with the byte at address 0x014D
    return checksum == this->data[0x014D];
}

bool Cartridge::isColorMode() {
    // check if the cartridge supports color mode
    return this->data[0x0143] & 0x80;
}

bool Cartridge::isColorOnly() {
    // check if the cartridge supports color mode only
    return (this->data[0x0143] & 0xC0) == 0x80;
}

std::string Cartridge::getTitle() {
    // get the title of the cartridge

    const uint16_t startAddr = 0x0134;
    const uint16_t endAddr = ( this->isColorOnly() || this->isColorMode() ) ? 0x013F : 0x0144;

    auto it_start = this->data.begin() + startAddr;
    auto it_end = this->data.begin() + endAddr;

    auto it_null = std::find(it_start, it_end, 0x00);

    return std::string(it_start, it_null);
}

std::string Cartridge::getManufacturerCode() {
    // get the manufacturer code of the cartridge

    // manufacturer code is only available in newer cartridges (color mode or color only mode)
    if (!(this->isColorOnly() || this->isColorMode())) {
        return "N/A";
    }

    const uint16_t startAddr = 0x013F;
    const uint16_t endAddr = 0x0143; 

    auto it_start = this->data.begin() + startAddr;
    auto it_end = this->data.begin() + endAddr;

    auto it_null = std::find(it_start, it_end, 0x00);

    return std::string(it_start, it_null);
}  

bool Cartridge::isOldLicenseeCode() {
    // check if the cartridge uses the old licensee code
    return !(this->data[0x014B] == 0x33);
}

std::string Cartridge::getLicenseeCode() {
    // get the licensee code of the cartridge

    if (this->isOldLicenseeCode()) {
        return std::string(1, this->data[0x014B]);
    } else {
        return std::string(1, this->data[0x0144]) + std::to_string(this->data[0x0145]);
    }
}

bool Cartridge::hasSGBSupport() {
    // check if the cartridge supports SGB
    return this->data[0x0146] == 0x03;
}

std::string Cartridge::getType() {
    // get the type of the cartridge
    return std::string(1, this->data[0x0147]);
}

uint32_t Cartridge::getROMSize() {
    // get the ROM size of the cartridge in bytes
    switch (this->data[0x0148]) {
        case 0x52: return 1.1 * 1024 * 1024;
        case 0x53: return 1.2 * 1024 * 1024;
        case 0x54: return 1.5 * 1024 * 1024;
        default: return (32 * 1024 << (this->data[0x0148]));
    }
}

uint32_t Cartridge::getRAMSize() {
    // get the RAM size of the cartridge
    switch (this->data[0x0149]) {
        case 0x00: return 0 * 1024;
        case 0x01: return 2 * 1024;
        case 0x02: return 8 * 1024;
        case 0x03: return 32 * 1024;
        case 0x04: return 128 * 1024;
        case 0x05: return 64 * 1024;
        default: return 0;
    }
}

std::string Cartridge::getDestinationCode() {
    // get the destination code of the cartridge
    return std::string(1, this->data[0x014A]);
}

std::string Cartridge::getMaskROMVersion() {
    // get the mask ROM version of the cartridge
    return std::string(1, this->data[0x014C]);
}

bool Cartridge::globalChecksum() {
    // check if the global checksum is valid
    return ((this->data[0x014E] << 8) | this->data[0x014F]) == 0x00;
}

uint8_t Cartridge::readRAM(uint16_t addr) const {
    if (this->ram.empty()) {
        return 0xFF;
    }

    if (this->mbcType == MBCType::MBC2) {
        // MBC2 RAM is 512 bytes of 4-bit values
        return this->ram[(addr - 0xA000) & 0x1FF] & 0x0F;
    }

    uint8_t bank = this->ramBank;
    if (this->mbcType == MBCType::MBC1 && this->bankingMode) {
        bank = this->romBank2;
    }
    uint32_t ramAddr = static_cast<uint32_t>(bank) * 0x2000 + (addr - 0xA000);
    if (ramAddr >= this->ram.size()) {
        return 0xFF;
    }
    return this->ram[ramAddr];
}

void Cartridge::writeRAM(uint16_t addr, uint8_t data) {
    if (this->ram.empty()) {
        return;
    }

    if (this->mbcType == MBCType::MBC2) {
        this->ram[(addr - 0xA000) & 0x1FF] = data & 0x0F;
        return;
    }

    uint8_t bank = this->ramBank;
    if (this->mbcType == MBCType::MBC1 && this->bankingMode) {
        bank = this->romBank2;
    }
    uint32_t ramAddr = static_cast<uint32_t>(bank) * 0x2000 + (addr - 0xA000);
    if (ramAddr >= this->ram.size()) {
        return;
    }
    this->ram[ramAddr] = data;
}

uint8_t Cartridge::read(uint16_t addr) const {
    const uint32_t romBanks = this->data.size() / 0x4000;

    // read a byte from the cartridge
    if (addr < 0x4000) {
        // bank 0 area
        uint32_t bank = 0;
        if (this->mbcType == MBCType::MBC1 && this->bankingMode) {
            bank = (static_cast<uint32_t>(this->romBank2) << 5) % romBanks;
        }
        uint32_t offset = bank * 0x4000 + addr;
        if (offset < this->data.size()) {
            return this->data[offset];
        }
        return 0xFF;
    }
    else if (addr < 0x8000) {
        // switchable bank area
        uint32_t bank = this->romBank;
        if (this->mbcType == MBCType::MBC1) {
            bank = (this->romBank & 0x1F) | (static_cast<uint32_t>(this->romBank2) << 5);
            if ((bank & 0x1F) == 0) {
                bank |= 0x01;
            }
        }
        bank %= romBanks;
        uint32_t offset = bank * 0x4000 + (addr - 0x4000);
        if (offset < this->data.size()) {
            return this->data[offset];
        }
        return 0xFF;
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (!this->ramEnabled && this->mbcType != MBCType::None) {
            return 0xFF;
        }
        if (this->mbcType == MBCType::MBC3 && this->rtcRegister >= 0x08 && this->rtcRegister <= 0x0C) {
            return this->rtc[this->rtcRegister - 0x08];
        }
        return this->readRAM(addr);
    }
    else {
        return 0xFF; // Unmapped address
    }
}

void Cartridge::write(uint16_t addr, uint8_t data) {
    // write a byte to the cartridge
    if (addr < 0x2000) {
        // RAM enable / disable
        if (this->mbcType == MBCType::MBC2) {
            this->ramEnabled = (data & 0x0F) == 0x0A;
        } else if (this->mbcType != MBCType::None) {
            this->ramEnabled = (data & 0x0F) == 0x0A;
        }
        return;
    }
    else if (addr < 0x3000) {
        // ROM bank low bits (MBC5: bits 0-7; others: whole register)
        switch (this->mbcType) {
            case MBCType::MBC1: {
                uint8_t bank = data & 0x1F;
                if (bank == 0) {
                    bank = 1;
                }
                this->romBank = (this->romBank & 0x1FE0) | bank;
                break;
            }
            case MBCType::MBC2: {
                uint8_t bank = data & 0x0F;
                if (bank == 0) {
                    bank = 1;
                }
                this->romBank = (this->romBank & 0xFFF0) | bank;
                break;
            }
            case MBCType::MBC3: {
                uint8_t bank = data & 0x7F;
                if (bank == 0) {
                    bank = 1;
                }
                this->romBank = (this->romBank & 0xFF80) | bank;
                break;
            }
            case MBCType::MBC5: {
                this->romBank = (this->romBank & 0x100) | (data & 0xFF);
                break;
            }
            default:
                break;
        }
        return;
    }
    else if (addr < 0x4000) {
        // MBC1/MBC2/MBC3 decode the whole 0x2000-0x3FFF range as the bank register
        switch (this->mbcType) {
            case MBCType::MBC1: {
                uint8_t bank = data & 0x1F;
                if (bank == 0) {
                    bank = 1;
                }
                this->romBank = (this->romBank & 0x1FE0) | bank;
                break;
            }
            case MBCType::MBC2: {
                uint8_t bank = data & 0x0F;
                if (bank == 0) {
                    bank = 1;
                }
                this->romBank = (this->romBank & 0xFFF0) | bank;
                break;
            }
            case MBCType::MBC3: {
                uint8_t bank = data & 0x7F;
                if (bank == 0) {
                    bank = 1;
                }
                this->romBank = (this->romBank & 0xFF80) | bank;
                break;
            }
            case MBCType::MBC5: {
                // MBC5: bit 8 of the ROM bank number
                this->romBank = (this->romBank & 0xFF) | ((data & 0x01) << 8);
                break;
            }
            default:
                break;
        }
        return;
    }
    else if (addr < 0x6000) {
        // RAM bank / ROM bank high bits
        switch (this->mbcType) {
            case MBCType::MBC1:
                this->romBank2 = data & 0x03;
                break;
            case MBCType::MBC3:
                if ((data & 0x0F) >= 0x08) {
                    this->rtcRegister = data & 0x0F;
                } else {
                    this->ramBank = data & 0x03;
                    this->rtcRegister = 0;
                }
                break;
            case MBCType::MBC5:
                this->ramBank = data & 0x0F;
                break;
            default:
                break;
        }
        return;
    }
    else if (addr < 0x8000) {
        // MBC1 banking mode / MBC3 RTC latch clock (data 0x00 then 0x01)
        if (this->mbcType == MBCType::MBC1) {
            this->bankingMode = (data & 0x01) != 0;
        }
        return;
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (this->mbcType == MBCType::None) {
            // no MBC: direct RAM access, used by Blargg tests for their result output
            if (addr == 0xA000) {
                if (data == 0x00) {
                    std::cout << "\n\n=====> RESULT: PASSED! <=====\n" << std::endl;
                } else if (data != 0x80) {
                    std::cout << "\n\n=====> RESULT: FAILED #" << static_cast<int>(data) << " <=====\n" << std::endl;
                }
            }
        }
        if (!this->ramEnabled && this->mbcType != MBCType::None) {
            return;
        }
        if (this->mbcType == MBCType::MBC3 && this->rtcRegister >= 0x08 && this->rtcRegister <= 0x0C) {
            this->rtc[this->rtcRegister - 0x08] = data;
            return;
        }
        this->writeRAM(addr, data);
        return;
    }
    else {
        return; // Unmapped address
    }
}

MBCType Cartridge::getMBCType() const {
    return this->mbcType;
}
