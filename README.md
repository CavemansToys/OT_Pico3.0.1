# OT_Pico3.0.1 — OpenTrickler Controller

Firmware for the [OpenTrickler](https://github.com/eamars/OpenTrickler-RP2040-Controller) automated powder trickler, used in precision ammunition reloading. Runs on a **Raspberry Pi Pico 2 W (RP2350)** with FreeRTOS SMP, WiFi, and multiple hardware peripherals.

Developed by Gadgets — [www.cavemanstoys.com](https://www.cavemanstoys.com)

Based on [OpenTrickler-RP2040-Controller](https://github.com/eamars/OpenTrickler-RP2040-Controller).

## Supported Hardware

| Board | SoC 
|---|---|---|
| Raspberry Pi Pico 2 W | RP2350 - Tested
| Raspberry Pi Pico W | RP2040 

## Features

- Coarse tube reverse with configurable settle time --- I dont see this helping, but its there to try.
- AI-assisted PID auto-tuning
- Simulation mode for testing without teh Scale or powder.
- PWM servo powder gate
- WiFi access point + station mode with web portal

## Prerequisites

- ARM embedded toolchain (`arm-none-eabi-gcc`)
- CMake >= 3.25
- Python 3 (for build scripts)
- Git

## Building

```bash
# Clone with submodules
git clone --recursive https://github.com/<your-org>/OT_Pico3.0.1.git
cd OT_Pico3.0.1

# If you forgot --recursive:
git submodule update --init --recursive

# Build for Pico 2 W (RP2350, default) with Mini 12864 display
mkdir build && cd build
cmake -G Ninja ..
ninja

## Flashing

1. Hold **BOOTSEL** while plugging the Pico into USB
2. Copy `build/OpenTrickler_3.0.1.uf2` to the Pico mass storage device
3. The Pico reboots automatically

## Output Artifacts

| File | Description |
|---|---|
| `build/OpenTrickler_3.0.1.uf2` | Flashable firmware (drag-and-drop) |
| `build/OpenTrickler_3.0.1.elf` | For GDB debugging |
| `build/OpenTrickler_3.0.1.bin` | Binary image |

## Debugging

GDB scripts are provided in `scripts/`:
- `scripts/debug.gdb` — attach to running target
- `scripts/program.gdb` — flash and debug

Serial debug output is available via USB CDC at 115200 baud.

## Project Structure

```
src/                   Application source (C/C++)
  html/                Web portal pages (built into C headers at compile time)
  targets/             Board pin definitions
  generated/           Auto-generated files (do not edit, created at build time)
scripts/               Build helper scripts (html2header, version gen, GDB)
library/               Git submodule dependencies
  pico-sdk/            Raspberry Pi Pico SDK
  FreeRTOS-Kernel/     FreeRTOS with RP2350 SMP port
  u8g2/                Monochrome display library
  Trinamic-library/    TMC stepper driver library
  lvgl/                LVGL for TFT displays (TFT builds only)
```

## Configuration

All persistent settings are stored in CAT24C256 EEPROM (32KB) with CRC32 validation. Configuration can be changed via:
- The web portal (connect to the Pico's WiFi AP or join its STA network)
- REST API endpoints under `/rest/*`
- The on-device menu (Mini 12864 display + rotary encoder)

## License

GPLv3 — see [LICENSE](LICENSE).
