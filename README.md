# opengb

A Game Boy emulator written from scratch in modern C++.

OpenGB is a learning project: a cycle-stepped Sharp SM83 CPU, PPU, timer, joypad, and memory bus, rendered through SDL2. It already passes the Blargg CPU instruction and instruction-timing test suites and can run simple DMG ROMs.

> **Legal:** This repository does not include commercial Game Boy ROMs. Only use dumps of cartridges you own, or freely licensed homebrew / test ROMs.

## Status

| Component | State |
| --- | --- |
| CPU (SM83) | Working — Blargg `cpu_instrs` and `instr_timing` pass |
| PPU | Working — background, window, and sprites |
| Timer | Working |
| Joypad | Working |
| Serial | Stub (enough for Blargg output) |
| Cartridge | ROM + 8 KiB RAM, **no MBC banking** |
| APU (sound) | Not started |
| Frame limiter / VSync | Not started |
| Save files (`.sav`) | Not started |
| Game Boy Color | Not a goal yet |

ROMs that fit in 32 KiB with no memory-bank controller (for example many test ROMs and early titles) are the ones most likely to run well. Banked cartridges will not map correctly until MBCs land.

## Requirements

- A C++23 compiler (`g++` on Linux/WSL, `clang++` on macOS)
- [SDL2](https://www.libsdl.org/) development files
- `pkg-config`
- `make`

### Install dependencies

**Debian / Ubuntu / WSL**

```bash
sudo apt install build-essential libsdl2-dev pkg-config
```

**Fedora**

```bash
sudo dnf install gcc-c++ SDL2-devel pkgconf-pkg-config
```

**Arch**

```bash
sudo pacman -S base-devel sdl2 pkgconf
```

**macOS (Homebrew)**

```bash
brew install sdl2 pkg-config
```

## Build

```bash
make
```

The binary is written to `build/bin/opengb`.

```bash
make clean   # remove build/
```

## Usage

```bash
./build/bin/opengb path/to/rom.gb
```

The window is 160×144 pixels scaled 4×. Close it to quit.

### Controls

| Game Boy | Keyboard |
| --- | --- |
| D-pad | `W` `A` `S` `D` |
| A | `K` |
| B | `J` |
| Start | `Left Shift` |
| Select | `Enter` |

## Project layout

```
include/     Public headers (CPU, PPU, bus, cartridge, timer, joypad, serial)
src/         Implementations and SDL entry point
roms/        Local ROMs (gitignored — not shipped)
build/       Object files and the emulator binary
```

## Compatibility tests

Blargg suites currently used as a smoke check:

- `cpu_instrs` — all 11 ROMs pass
- `instr_timing` — pass

Details live in [`BLARGG_TEST_CHECKLIST.md`](BLARGG_TEST_CHECKLIST.md).

## Roadmap

1. Memory Bank Controllers (MBC1/MBC2/MBC3/MBC5) and battery saves (`.sav`)
2. Frame limiter / VSync so games run at ~59.7 Hz
3. APU (sound)

Packaging (`.deb`, `.dmg`, `.exe`) is extra, after the core is playable.

## Contributing

Issues and pull requests are welcome, especially around accuracy, MBCs, and the APU.

1. Keep the style close to the existing headers and `src/` files.
2. Prefer small, reviewable changes.
3. If you change CPU or PPU behavior, note which test ROM you ran.

## License

No license file is included yet. Until one is added, the default is that others should not assume they may copy or redistribute the code. If you want this to be proper open source, add a [`LICENSE`](https://choosealicense.com/) (MIT is a common choice for emulators).
