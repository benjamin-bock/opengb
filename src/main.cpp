#include <iostream>

#include "../include/cartridge.hpp"
#include "../include/bus.hpp"
#include "../include/cpu.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << "<path_to_rom.gb>" << std::endl;
        return 1;
    }
    
    std::string romPath = argv[1];
    
    // Instantiate the cartridge and load the ROM
    Cartridge cart(romPath);
    cart.load();
    
    // Instantiate the bus with a reference to the cartridge
    Bus bus(cart);
    
    // Instantiate the CPU with a reference to the bus
    CPU cpu(bus);
    
    std::cout << "Loaded ROM: " << cart.getTitle() << std::endl;
    std::cout << "Starting the emulator..." << std::endl;

    // Loop execution
    for (;;) {
        uint8_t cycles = cpu.step();

        // Update the timer
        // timer.step(cycles);

        // For now, we can just print the cycles for debugging
        std::cout << "Executed instruction in " << static_cast<int>(cycles) << " cycles." << std::endl;
    }

    return 0;
}
