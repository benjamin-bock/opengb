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
    std::ifstream file(this->filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Impossible to open the file: " + this->filepath);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    this->data.resize(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(this->data.data()), size);

    if (!file) {
        throw std::runtime_error("Incomplete file read: " + this->filepath);
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
