#include <iostream>
#include <string>
#include <SDL2/SDL.h>

#include "../include/cartridge.hpp"
#include "../include/bus.hpp"
#include "../include/cpu.hpp"
#include "../include/timer.hpp"
#include "../include/joypad.hpp"

bool translateKey(SDL_Keycode sym, JoypadKey& outKey) {
    switch (sym) {
        case SDLK_d:        outKey = JoypadKey::Right; return true;
        case SDLK_a:        outKey = JoypadKey::Left; return true;
        case SDLK_w:        outKey = JoypadKey::Up; return true;
        case SDLK_s:        outKey = JoypadKey::Down; return true;
        case SDLK_LSHIFT:   outKey = JoypadKey::Start; return true;
        case SDLK_RETURN:   outKey = JoypadKey::Select; return true;
        case SDLK_j:        outKey = JoypadKey::B; return true;
        case SDLK_k:        outKey = JoypadKey::A; return true;
        default:           return false;
    }
}

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

    // Initialise graphical window
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

    // Initialise sound card
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO); 

    SDL_AudioSpec want{}, have{};
    want.freq = 44100;
    want.format = AUDIO_F32SYS; // Float 32 bits (-1.0 to 1.0)
    want.channels = 2;          // Stereo
    want.samples = 1024;

    SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    SDL_PauseAudioDevice(audioDevice, 0); // Start sound card
    
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
            
            // Send accumulated sound to the sound card
            auto& buffer = bus.getAPU().getAudioBuffer();
            if (!buffer.empty()) {
                SDL_QueueAudio(audioDevice, buffer.data(), buffer.size() * sizeof(float));
                bus.getAPU().clearAudioBuffer();
            }

            // Sync the framerate to ~59.73 FPS
            while (SDL_GetQueuedAudioSize(audioDevice) > 44100 * 2 * sizeof(float) * 0.04) {
                SDL_Delay(1);
            }        

            // Event manager
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
                else if (event.type == SDL_KEYDOWN) {
                    JoypadKey key;
                    if (translateKey(event.key.keysym.sym, key)) {
                        bus.getJoypad().keyPressed(key);
                    }
                }
                else if (event.type == SDL_KEYUP) {
                    JoypadKey key;
                    if (translateKey(event.key.keysym.sym, key)) {
                        bus.getJoypad().keyReleased(key);
                    }
                }
            }
        }
    }
}

