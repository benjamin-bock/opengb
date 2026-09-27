#pragma once

#include <array>
#include <cstdint>

// Pulse with period sweep
class Channel1 {
    public:
        Channel1();
    
        uint8_t read(uint16_t addr) const;
        void write(uint16_t addr, uint8_t value);
        void trigger();
        void reset();
        bool isEnabled() const;
        
    private:
        uint8_t NR10; // $FF10 Sweep
        uint8_t NR11; // $FF11 Duty & Length
        uint8_t NR12; // $FF12 Volume & Envelope
        uint8_t NR13; // $FF13 Period Low [write-only]
        uint8_t NR14; // $FF14 Period High & Control

        bool enabled;
        bool DAC;

};

// Pulse without period sweep
class Channel2 {
    public:
        Channel2();
        
        uint8_t read(uint16_t addr) const;
        void write(uint16_t addr, uint8_t value);
        void trigger();
        void reset();
        bool isEnabled() const;
        
    private:
                      // no sweep
        uint8_t NR21; // $FF16 Duty & Length
        uint8_t NR22; // $FF17 Volume & Envelope
        uint8_t NR23; // $FF18 Period Low [write-only]
        uint8_t NR24; // $FF19 Period High & Control

        bool enabled;
        bool DAC;

};

// Wave output
class Channel3 {
    public:
        Channel3();
        
        uint8_t read(uint16_t addr) const;
        void write(uint16_t addr, uint8_t value);
        void trigger();
        void reset();
        bool isEnabled() const;
        
    private:
        uint8_t NR30; // $FF1A DAC Enable
        uint8_t NR31; // $FF1B Length Timer [write-only]
        uint8_t NR32; // $FF1C Output level
        uint8_t NR33; // $FF1D Period Low [write-only]
        uint8_t NR34; // $FF1E Period High & Control

        bool enabled;
        bool DAC;
};

// Noise
class Channel4 { 
    public:
        Channel4();
        
        uint8_t read(uint16_t addr) const;
        void write(uint16_t addr, uint8_t value);
        void trigger();
        void reset();
        bool isEnabled() const;
        
        private:
                      // no sweep
        uint8_t NR41; // $FF20 Length Timer [write-only]
        uint8_t NR42; // $FF21 Volume & Envelope
        uint8_t NR43; // $FF22 Frequency and randomness
        uint8_t NR44; // $FF23 Control

        bool enabled;
        bool DAC;
};

class APU {
    public:
        APU();
        ~APU() = default;
        uint8_t read(uint16_t addr) const;
        void write(uint16_t addr, uint8_t data);
        void reset();
        void step(uint8_t cycles);
    
    private:
        Channel1 ch1;
        Channel2 ch2;
        Channel3 ch3;
        Channel4 ch4;

        uint8_t NR50;
        uint8_t NR51;
        uint8_t NR52;
        std::array<uint8_t, 16> waveRam; // $FF30-FF3F
};
