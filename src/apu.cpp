#include "../include/apu.hpp"
#include <cstdint>

Channel1::Channel1() {
    this->NR10 = 0x80;
    this->NR11 = 0xBF;
    this->NR12 = 0xF3;
    this->NR13 = 0xFF;
    this->NR14 = 0xBF;
    this->enabled = false;
    this->DAC = true;
    this->lengthTimer = 0;
    this->shadowPeriod = 0;
    this->sweepTimer = 0;
    this->sweepEnabled = false;
};

Channel2::Channel2() {
    this->NR21 = 0x3F;
    this->NR22 = 0x00;
    this->NR23 = 0xFF;
    this->NR24 = 0xBF;
    this->enabled = false;
    this->DAC = false;
    this->lengthTimer = 0;
};

Channel3::Channel3() {
    this->NR30 = 0x7F;
    this->NR31 = 0xFF;
    this->NR32 = 0x9F;
    this->NR33 = 0xFF;
    this->NR34 = 0xBF;
    this->enabled = false;
    this->DAC = false;
    this->lengthTimer = 0;
};

Channel4::Channel4() {
    this->NR41 = 0xFF;
    this->NR42 = 0x00;
    this->NR43 = 0x00;
    this->NR44 = 0xBF;
    this->enabled = false;
    this->DAC = false;
    this->lengthTimer = 0;
};

APU::APU() {
    this->NR50 = 0x77;
    this->NR51 = 0xF3;
    this->NR52 = 0xF1;

    this->frameSequencerCycles = 0;
    this->frameSequencerStep = 0;
}

uint8_t Channel1::read(uint16_t addr) const {
    switch (addr) {
        case 0xFF10: return this->NR10 | 0x80;
        case 0xFF11: return this->NR11 | 0x3F;
        case 0xFF12: return this->NR12;
        case 0xFF13: return 0xFF; // NR13 is write-only
        case 0xFF14: return this->NR14 | 0xBF;
        default: return 0xFF;
    }
}

void Channel1::write(uint16_t addr, uint8_t data) {
    switch (addr) {
        case 0xFF10: 
            this->NR10 = data; break;
        case 0xFF11:
            this->NR11 = data;
            this->lengthTimer = 64 - (data & 0x3F);
            break;
        case 0xFF12:
            this->NR12 = data;
            this->DAC = (data & 0xF8) != 0;
            if (!this->DAC) {
                this->enabled = false;
            } 
            break;
        case 0xFF13:
            this->NR13 = data; break;
        case 0xFF14:
            this->NR14 = data; 
            if (data & 0x80) {
                this->trigger();
            }
            break;
        default: break;
    }
}

void Channel1::trigger() {
    if (this->lengthTimer == 0) {
        this->lengthTimer = 64;
    }
    if (this->DAC) {
        this->enabled = true;
    }

    // Sweep reset
    this->shadowPeriod = ((this->NR14 & 0x07) << 8) | this->NR13;
    uint8_t pace = (this->NR10 & 0x70) >> 4;
    uint8_t step = this->NR10 & 0x07;
    this->sweepTimer = (pace > 0) ? pace : 8;
    this->sweepEnabled = (pace > 0) || (step > 0);

    // Immediate overflow test if step > 0
    if (step > 0) {
        this->calculateSweepPeriod();
    }

    // Enveloppe reset
    this->currentVolume = (this->NR12 & 0xF0) >> 4; // Bit 7-4
    this->envelopeTimer = (this->NR12 & 0x07);      // Bit 2-0

    // Reload frequency timer
    this->frequencyTimer = this->getPeriod();
}

void Channel1::reset() {
    this->NR10 = 0;
    this->NR11 = 0;
    this->NR12 = 0;
    this->NR13 = 0;
    this->NR14 = 0;

    this->enabled = false;
    this->DAC = false;

    this->dutyStep = 0;
    this->currentVolume = 0;
    this->frequencyTimer = 0;
    this->envelopeTimer = 0;
    this->sweepTimer = 0;
    this->shadowPeriod = 0;
    this->sweepEnabled = false;
}

void Channel1::step(uint8_t cycles) {
    this->frequencyTimer -= cycles;

    if (this->frequencyTimer <= 0) {
        // Reload timer for next pulse
        this->frequencyTimer += this->getPeriod();

        // Step in the square wave
        this->dutyStep = (this->dutyStep + 1) & 0x07;
    }
}

bool Channel1::isEnabled() const {
    return this->enabled;
}

uint16_t Channel1::calculateSweepPeriod() {
    bool direction = (this->NR10 & 0x08) != 0;
    uint8_t step = this->NR10 & 0x07;

    uint16_t newPeriod = this->shadowPeriod >> step;
    if (!direction) { // 0 : Addition
        newPeriod = this->shadowPeriod + newPeriod;
    }
    else { // 1 : Substraction
        newPeriod = this->shadowPeriod - newPeriod;
    }
    
    // 11-bit overflow
    if (newPeriod > 0x07FF) {
        this->enabled = false;
    }

    return newPeriod;
}

void Channel1::clockLength() {
    // Bit 6 of NR14 : Length Enable
    bool lengthEnabled = (this->NR14 & 0x40) != 0;

    if (lengthEnabled && this->lengthTimer > 0) {
        this->lengthTimer--;
        if (this->lengthTimer == 0) {
            this->enabled = false; // Tone is ended, turn off the channel
        }
    }
}

void Channel1::clockSweep() {
    if (this->sweepTimer > 0) {
        this->sweepTimer--;
    }
    if (this->sweepTimer == 0) {
        uint8_t pace = (this->NR10 & 0x70) >> 4;
        this->sweepTimer = (pace > 0) ? pace : 8;

        // Sweep only if pace > 0 and enabled
        if (this->sweepEnabled && pace > 0) {
            uint16_t newPeriod = this->calculateSweepPeriod();

            // If no overflow and there is an offset (step > 0)
            uint8_t step = this->NR10 & 0x07;
            if (newPeriod <= 0x07FF && step > 0) {
                this->shadowPeriod = newPeriod;

                // Update NR13 and NR14 registers
                this->NR13 = newPeriod & 0xFF;
                this->NR14 = (this->NR14 & 0xF8) | ((newPeriod >> 8) & 0x07);

                // DMG executes second overflow test immediately
                this->calculateSweepPeriod();
            }
        }
    }
    return;
}

void Channel1::clockEnvelope() {
    uint8_t pace = this->NR12 & 0x07;
    if (pace == 0) {
        return;
    }
    if (this->envelopeTimer > 0) {
        this->envelopeTimer--;
    }
    if (this->envelopeTimer == 0) {
        this->envelopeTimer = pace;

        if ((this->NR12 & 0x08) != 0) { // 1 : Addition
            if (this->currentVolume < 15) { // Volume value is between 0 and 15
                this->currentVolume++;
            }
        } 
        else { // 0 : Substraction
            if (this->currentVolume > 0) {
                this->currentVolume--; 
            }
        }
    }
}

uint16_t Channel1::getPeriod() const {
    uint16_t frequency = ((this->NR14 & 0x07) << 8) | this->NR13;
    return (2048 - frequency) * 4;
}

uint8_t Channel1::getSample() const {
    if (!this->enabled || !this->DAC) {
        return 0; // Silence if channel is off
    }

    static constexpr uint8_t DUTY_TABLE[4][8] = {
        {0, 0, 0, 0, 0, 0, 0, 1}, // 12.5%
        {1, 0, 0, 0, 0, 0, 0, 1}, // 25%
        {1, 0, 0, 0, 0, 1, 1, 1}, // 50%
        {0, 1, 1, 1, 1, 1, 1, 0}  // 75%
    };
    
    uint8_t dutyType = (this->NR11 & 0xC0) >> 6; // Bits 7-6

    if (DUTY_TABLE[dutyType][this->dutyStep]) {
        return this->currentVolume;
    }
    return 0;
}

uint8_t Channel2::read(uint16_t addr) const {
    switch (addr) {
        case 0xFF16: return this->NR21 | 0x3F;
        case 0xFF17: return this->NR22;
        case 0xFF18: return 0xFF; // NR23 is write-only
        case 0xFF19: return this->NR24 | 0xBF;
        default: return 0xFF;
    }
}

void Channel2::write(uint16_t addr, uint8_t data) {
    switch (addr) {
        case 0xFF16:
            this->NR21 = data; 
            this->lengthTimer = 64 - (data & 0x3F);
            break;
        case 0xFF17:
            this->NR22 = data; 
            this->DAC = (data & 0xF8) != 0;
            if (!this->DAC) {
                this->enabled = false;
            } 
            break;
        case 0xFF18:
            this->NR23 = data; break;
        case 0xFF19:
            this->NR24 = data; 
                if (data & 0x80) {
                    this->trigger();
                }
                break;
        default: break;
    }
}

void Channel2::trigger() {
    if (this->lengthTimer == 0) {
        this->lengthTimer = 64;
    }
    if (this->DAC) {
        this->enabled = true;
    }

    // Enveloppe reset
    this->currentVolume = (this->NR22 & 0xF0) >> 4; // Bit 7-4
    this->envelopeTimer = (this->NR22 & 0x07);      // Bit 2-0

    // Reload frequency timer
    this->frequencyTimer = this->getPeriod();
}

void Channel2::reset() {
    this->NR21 = 0;
    this->NR22 = 0;
    this->NR23 = 0;
    this->NR24 = 0;

    this->enabled = false;
    this->DAC = false;

    this->dutyStep = 0;
    this->currentVolume = 0;
    this->frequencyTimer = 0;
    this->envelopeTimer = 0;
}

void Channel2::step(uint8_t cycles) {
    this->frequencyTimer -= cycles;

    if (this->frequencyTimer <= 0) {
        // Reload timer for next pulse
        this->frequencyTimer += this->getPeriod();

        // Step in the square wave
        this->dutyStep = (this->dutyStep + 1) & 0x07;
    }
}

bool Channel2::isEnabled() const {
    return this->enabled;
}

void Channel2::clockLength() {
    // Bit 6 of NR24 : Length Enable
    bool lengthEnabled = (this->NR24 & 0x40) != 0;

    if (lengthEnabled && this->lengthTimer > 0) {
        this->lengthTimer--;
        if (this->lengthTimer == 0) {
            this->enabled = false; // Tone is ended, turn off the channel
        }
    }
}

void Channel2::clockEnvelope() {
    uint8_t pace = this->NR22 & 0x07;
    if (pace == 0) {
        return;
    }
    if (this->envelopeTimer > 0) {
        this->envelopeTimer--;
    }
    if (this->envelopeTimer == 0) {
        this->envelopeTimer = pace;

        if ((this->NR22 & 0x08) != 0) { // 1 : Addition
            if (this->currentVolume < 15) { // Volume value is between 0 and 15
                this->currentVolume++;
            }
        } 
        else { // 0 : Substraction
            if (this->currentVolume > 0) {
                this->currentVolume--; 
            }
        }
    }
}

uint16_t Channel2::getPeriod() const {
    uint16_t frequency = ((this->NR24 & 0x07) << 8) | this->NR23;
    return (2048 - frequency) * 4;
}

uint8_t Channel2::getSample() const {
    if (!this->enabled || !this->DAC) {
        return 0; // Silence if channel is off
    }

    static constexpr uint8_t DUTY_TABLE[4][8] = {
        {0, 0, 0, 0, 0, 0, 0, 1}, // 12.5%
        {1, 0, 0, 0, 0, 0, 0, 1}, // 25%
        {1, 0, 0, 0, 0, 1, 1, 1}, // 50%
        {0, 1, 1, 1, 1, 1, 1, 0}  // 75%
    };
    
    uint8_t dutyType = (this->NR21 & 0xC0) >> 6; // Bits 7-6

    if (DUTY_TABLE[dutyType][this->dutyStep]) {
        return this->currentVolume;
    }
    return 0;
}

uint8_t Channel3::read(uint16_t addr) const {
    switch (addr) {
        case 0xFF1A: return this->NR30 | 0x7F;
        case 0xFF1B: return 0xFF; // NR31 is write-only
        case 0xFF1C: return this->NR32 | 0x9F;
        case 0xFF1D: return 0xFF; // NR33 is write-only
        case 0xFF1E: return this->NR34 | 0xBF;
        default: return 0xFF;
    }
}

void Channel3::write(uint16_t addr, uint8_t data) {
    switch (addr) {
        case 0xFF1A: 
            this->NR30 = data; 
            this->DAC = (data & 0x80) != 0;
            if (!this->DAC) {
                this->enabled = false;
            } 
            break;
        case 0xFF1B:
            this->NR31 = data; 
            this->lengthTimer = 256 - data;
            break;
        case 0xFF1C:
            this->NR32 = data; break;
        case 0xFF1D:
            this->NR33 = data; break;
        case 0xFF1E:
            this->NR34 = data; 
            if (data & 0x80) {
                this->trigger();
            }
            break;
        default: break;
    }
}

void Channel3::trigger() {
    if (this->lengthTimer == 0) {
        this->lengthTimer = 256;
    }
    if (this->DAC) {
        this->enabled = true;
    }

    // Reload frequency timer
    this->frequencyTimer = this->getPeriod();
}

void Channel3::reset() {
    this->NR30 = 0;
    this->NR31 = 0;
    this->NR32 = 0;
    this->NR33 = 0;
    this->NR34 = 0;

    this->enabled = false;
    this->DAC = false;

    this->frequencyTimer = 0;
    this->sampleIndex = 0;
}

void Channel3::step(uint8_t cycles) {
    this->frequencyTimer -= cycles;

    if (this->frequencyTimer <= 0) {
        // Reload timer for next pulse
        this->frequencyTimer += this->getPeriod();
        this->sampleIndex = (this->sampleIndex + 1) & 0x1F; // Loop from 0 to 31
    }
}

bool Channel3::isEnabled() const {
    return this->enabled;
}

void Channel3::clockLength() {
    // Bit 6 of NR14 : Length Enable
    bool lengthEnabled = (this->NR34 & 0x40) != 0;

    if (lengthEnabled && this->lengthTimer > 0) {
        this->lengthTimer--;
        if (this->lengthTimer == 0) {
            this->enabled = false; // Tone is ended, turn off the channel
        }
    }
}

uint16_t Channel3::getPeriod() const {
    uint16_t frequency = ((this->NR34 & 0x07) << 8) | this->NR33;
    return (2048 - frequency) * 2;
}

uint8_t Channel3::getSample() {
    if (!this->enabled || !this->DAC) {
        return 0; // Silence if channel is off
    }
    uint8_t sample = 0;

    if (this->sampleIndex % 2 == 0) { // is Even
        sample = (this->waveRam[this->sampleIndex] & 0xF0) >> 4;
    }
    else { // is Odd
        sample = this->waveRam[this->sampleIndex] & 0x0F;
    }

    switch ((this->NR32 & 0x60) >> 5) {
        case 0b00: return 0;           //   0% volume (mute)
        case 0b01: return sample;      // 100% volume
        case 0b10: return sample >> 1; //  50% volume
        case 0b11: return sample >> 2; //  25% volume
        default: return 0;
    }
}

uint8_t Channel4::read(uint16_t addr) const {
    switch (addr) {
        case 0xFF20: return 0xFF; // NR41 is write-only
        case 0xFF21: return this->NR42;
        case 0xFF22: return this->NR43;
        case 0xFF23: return this->NR44 | 0xBF;
        default: return 0xFF;
    }
}

void Channel4::write(uint16_t addr, uint8_t data) {
    switch (addr) {
        case 0xFF20:
            this->NR41 = data;
            this->lengthTimer = 64 - (data & 0x3F);
            break;
        case 0xFF21:
            this->NR42 = data;
            this->DAC = (data & 0xF8) != 0;
            if (!this->DAC) {
                this->enabled = false;
            } 
            break;
        case 0xFF22:
            this->NR43 = data; break;
        case 0xFF23:
            this->NR44 = data; 
            if (data & 0x80) {
                this->trigger();
            }
            break;
        default: break;
    }
}

void Channel4::trigger() {
    if (this->lengthTimer == 0) {
        this->lengthTimer = 64;
    }
    if (this->DAC) {
        this->enabled = true;
    }
    
    // Enveloppe reset
    this->currentVolume = (this->NR42 & 0xF0) >> 4; // Bit 7-4
    this->envelopeTimer = (this->NR42 & 0x07);      // Bit 2-0

    // Reload frequency timer
    this->frequencyTimer = this->getPeriod();
}

void Channel4::reset() {
    this->NR41 = 0;
    this->NR42 = 0;
    this->NR43 = 0;
    this->NR44 = 0;

    this->enabled = false;
    this->DAC = false;
}

void Channel4::step(uint8_t cycles) {
    this->frequencyTimer -= cycles;

    if (this->frequencyTimer <= 0) {
        // Reload timer for next pulse
        this->frequencyTimer += this->getPeriod();
    }
}

bool Channel4::isEnabled() const {
    return this->enabled;
}

void Channel4::clockLength() {
    // Bit 6 of NR14 : Length Enable
    bool lengthEnabled = (this->NR44 & 0x40) != 0;

    if (lengthEnabled && this->lengthTimer > 0) {
        this->lengthTimer--;
        if (this->lengthTimer == 0) {
            this->enabled = false; // Tone is ended, turn off the channel
        }
    }
}

void Channel4::clockEnvelope() {
    uint8_t pace = this->NR42 & 0x07;
    if (pace == 0) {
        return;
    }
    if (this->envelopeTimer > 0) {
        this->envelopeTimer--;
    }
    if (this->envelopeTimer == 0) {
        this->envelopeTimer = pace;

        if ((this->NR42 & 0x08) != 0) { // 1 : Addition
            if (this->currentVolume < 15) { // Volume value is between 0 and 15
                this->currentVolume++;
            }
        } 
        else { // 0 : Substraction
            if (this->currentVolume > 0) {
                this->currentVolume--; 
            }
        }
    }
}

uint16_t Channel4::getPeriod() const {
    uint16_t frequency = ((this->NR44 & 0x07) << 8) | this->NR43;
    return (2048 - frequency) * 4;
}

uint8_t Channel4::getSample() {
    if (!this->enabled || !this->DAC) {
        return 0; // Silence if channel is off
    }
    bool bit0 = (this->LFSR & 0x0001);
    bool bit1 = (this->LFSR & 0x0002) >> 1;

    bool copy = bit0 == bit1; // LFSR0 NXOR LFSR1

    this->LFSR = (this->LFSR & 0x7FFF) | (copy << 15); // replace bit15 by copy

    if (this->NR43 & 0x08) { // short mode (7-bit LFSR)
        this->LFSR = (this->LFSR & 0xFF7F) | (copy << 7); // replace bit7 by copy
    }
    this->LFSR >>= 1; // Right shift

    return bit0 ? this->currentVolume : 0;
}

uint8_t APU::read(uint16_t addr) const {
    // Channels 1,2,3,4
    if (addr >= 0xFF10 && addr <= 0xFF14) return this->ch1.read(addr);
    if (addr >= 0xFF16 && addr <= 0xFF19) return this->ch2.read(addr);
    if (addr >= 0xFF1A && addr <= 0xFF1E) return this->ch3.read(addr);
    if (addr >= 0xFF20 && addr <= 0xFF23) return this->ch4.read(addr);

    // Global channels
    if (addr == 0xFF24) return this->NR50;
    if (addr == 0xFF25) return this->NR51;
    if (addr == 0xFF26) {
        uint8_t status = (this->NR52 & 0x80) | 0x70;
        if (this->ch1.isEnabled()) status |= 0x01;
        if (this->ch2.isEnabled()) status |= 0x02;
        if (this->ch3.isEnabled()) status |= 0x04;
        if (this->ch4.isEnabled()) status |= 0x08;
        return status;
    }
    // Wave RAM
    if (addr >= 0xFF30 && addr <= 0xFF3F) {
        return this->waveRam[addr - 0xFF30];
    }

    return 0xFF;
}

void APU::write(uint16_t addr, uint8_t data) {
    bool isOn = (this->NR52 & 0x80) != 0;

    // If APU is off, ignore all write except to NR52($FF26) and WaveRAM($FF30-$FF3F)
    if (!isOn && addr != 0xFF26) {
        if (addr >= 0xFF30 && addr <= 0xFF3F) {
            this->waveRam[addr - 0xFF30] = data;
        }
        return;
    }

    // Channels 1,2,3,4
    if (addr >= 0xFF10 && addr <= 0xFF14) {
        this->ch1.write(addr, data);
        return;
    }
    if (addr >= 0xFF16 && addr <= 0xFF19) {
        this->ch2.write(addr, data);
        return;
    }
    if (addr >= 0xFF1A && addr <= 0xFF1E) {
        this->ch3.write(addr, data);
        return;
    }
    if (addr >= 0xFF20 && addr <= 0xFF23) {
        this->ch4.write(addr, data);
        return;
    }

    // Global channels
    if (addr == 0xFF24) {
        this->NR50 = data;
        return;
    }
    if (addr == 0xFF25) {
        this->NR51 = data;
        return;
    }
    if (addr == 0xFF26) {
        // Only bit 7 can be written by the CPU on NR52
        bool turnOn = (data & 0x80) != 0;
        this->NR52 = (this->NR52 & 0x7F) | (data & 0x80);

        if (isOn && !turnOn) {
            this->reset();
        }
    }
    // Wave RAM
    if (addr >= 0xFF30 && addr <= 0xFF3F) {
        this->waveRam[addr - 0xFF30] = data;
        return;
    }

    return;
}

void APU::reset() {
    this->ch1.reset();
    this->ch2.reset();
    this->ch3.reset();
    this->ch4.reset();

    // Global channels
    this->NR50 = 0;
    this->NR51 = 0;
}

void APU::step(uint8_t cycles) {
    // If the APU is down, ignore
    if (!(this->NR52 & 0x80)) {
        return;
    }

    // Step on each channel
    this->ch1.step(cycles);
    this->ch2.step(cycles);
    this->ch3.step(cycles);
    this->ch4.step(cycles);

    // Increment the step frequencer (512 Hz -> every 8192 cycles)
    this->frameSequencerCycles += cycles;
    while (this->frameSequencerCycles >= 8192) {
        this->frameSequencerCycles -= 8192;

        switch (this->frameSequencerStep) {
            case 0:
                clockLength();
                break;
            case 1:
                break;
            case 2:
                clockLength();
                clockSweep();
                break;
            case 3:
                break;
            case 4:
                clockLength();
                break;
            case 5:
                break;
            case 6:
                clockLength();
                clockSweep();
                break;
            case 7:
                clockEnvelope();
                break;
        }

        this->frameSequencerStep = (this->frameSequencerStep + 1) & 0x07; // Loop from 0 to 7
    }
}

void APU::clockLength() {
    this->ch1.clockLength();
    this->ch2.clockLength();
    this->ch3.clockLength();
    this->ch4.clockLength();
}

void APU::clockSweep() {
    this->ch1.clockSweep(); // Only channel 1 has a sweep
}

void APU::clockEnvelope() {
    this->ch1.clockEnvelope();
    this->ch2.clockEnvelope();
    this->ch4.clockEnvelope(); // Channel 3 doesn't have envelope
}