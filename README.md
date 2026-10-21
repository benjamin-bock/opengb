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
| Cartridge | ROM + MBC1/MBC2/MBC3/MBC5 banking (RTC non persisté) |
| APU (sound) | Not started |
| Frame limiter / VSync | Not started |
| Save files (`.sav`) | Working — battery RAM persisted next to the ROM |
| Game Boy Color | Not a goal yet |

ROMs up to 8 MiB with an MBC1, MBC2, MBC3 or MBC5 memory-bank controller now map correctly. Battery-backed cartridge RAM is loaded from and written back to a `.sav` file next to the ROM (written on exit).

## Requirements

- A C++23 compiler (`g++` on Linux/WSL, `clang++` on macOS)
- [SDL2](https://www.libsdl.org/) development files
- [SDL2_ttf](https://github.com/libsdl-org/SDL_ttf) development files (for the launcher UI)
- `pkg-config`
- `make`

### Install dependencies

**Debian / Ubuntu / WSL**

```bash
sudo apt install build-essential libsdl2-dev libsdl2-ttf-dev pkg-config
```

**Fedora**

```bash
sudo dnf install gcc-c++ SDL2-devel SDL2_ttf-devel pkgconf-pkg-config
```

**Arch**

```bash
sudo pacman -S base-devel sdl2 sdl2_ttf pkgconf
```

**macOS (Homebrew)**

```bash
brew install sdl2 sdl2_ttf pkg-config
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

Run without arguments to open the launcher:

```bash
./build/bin/opengb
```

The launcher lets you pick a ROM (type the path, **Parcourir...** to browse, or drag & drop a `.gb`/`.gbc` file), set the sound volume, and start the emulator with **Lancer**.

You can also skip the launcher:

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
include/     Public headers (CPU, PPU, bus, cartridge, timer, joypad, serial, launcher)
src/         Implementations, launcher UI, and SDL entry point
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

Packaging: pushing a tag `v*` (e.g. `v0.1.0`) triggers the [release workflow](.github/workflows/release.yml), which builds **macOS (Apple Silicon)**, **Windows (x64)**, and **Linux (x64)** binaries and publishes a draft GitHub release with the archives attached.

## Contributing

Issues and pull requests are welcome, especially around accuracy, MBCs, and the APU.

1. Keep the style close to the existing headers and `src/` files.
2. Prefer small, reviewable changes.
3. If you change CPU or PPU behavior, note which test ROM you ran.

## License

Released under the [MIT License](LICENSE).
