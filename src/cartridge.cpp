#include "cartridge.hpp"

static constexpr std::array<uint8_t, 48> NINTENDO_LOGO_REF = {
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

Cartridge::Cartridge(const std::string& filepath) {
    this->filepath = filepath;
}

Cartridge::~Cartridge() {
    // close the file
    fclose(this->file);
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
        return std::string(this->data[0x014B]);
    } else {
        return std::string(this->data[0x0144], this->data[0x0145]);
    }
}

bool Cartridge::hasSGBSupport() {
    // check if the cartridge supports SGB
    return this->data[0x0146] == 0x03;
}

std::string Cartridge::getType() {
    // get the type of the cartridge
    return this->data[0x0147];
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
    return this->data[0x014A];
}

std::string Cartridge::getMaskROMVersion() {
    // get the mask ROM version of the cartridge
    return this->data[0x014C];
}

bool Cartridge::globalChecksum() {
    // check if the global checksum is valid
    return ((this->data[0x014E] << 8) | this->data[0x014F]) == 0x00;
}

uint8_t Cartridge::read(uint16_t addr) const {
    // read a byte from the cartridge
    if (addr >= this->data.size()) {
        return 0xFF;
    }
    if (addr < 0x8000) {
        return this->data[addr];
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        return 0xFF; // RAM is not implemented yet
    }
    else {
        return 0xFF; // Unmapped address
    }
}

void Cartridge::write(uint16_t addr, uint8_t data) {
    // write a byte to the cartridge
    if (addr >= this->data.size()) {
        return;
    }
    if (addr < 0x8000) {
        return; // ROM is read only
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        return; // RAM is not implemented yet
    }
    else {
        return; // Unmapped address
    }
}