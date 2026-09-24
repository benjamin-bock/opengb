#include <iostream>
#include <string>
#include <SDL2/SDL.h>

#include "../include/cartridge.hpp"
#include "../include/bus.hpp"
#include "../include/cpu.hpp"
#include "../include/timer.hpp"

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

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    constexpr int gbWidth = 160;
    constexpr int gbHeight = 144;
    constexpr int scale = 4;

    SDL_Window* window = SDL_CreateWindow(
        "opengb",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        gbWidth * scale,
        gbHeight * scale,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        gbWidth,
        gbHeight
    );
    if (!texture) {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_RenderSetLogicalSize(renderer, gbWidth, gbHeight);
    
    std::cout << "Loaded ROM: " << cart.getTitle() << std::endl;
    std::cout << "Starting the emulator..." << std::endl;

    bool running = true;
    while (running) {
        uint8_t cycles = cpu.step();

        // Update the bus components
        bus.step(cycles);

        if (bus.getPPU().isFrameReady()) {
            bus.getPPU().clearFrameReady();

            // SDL Render (60 times per second)
            SDL_UpdateTexture(texture, nullptr, bus.getPPU().getFramebuffer(), 160 * sizeof(uint32_t));
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
            
            // Event manager
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
            }
        }

    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
