# HAL STM32 Module

STM32CubeMX-backed implementations of selected `hal_interface` contracts.

## Current scope
- `DigitalOutput` for STM32F401Cx projects generated with STM32CubeMX /
  STM32CubeIDE.

## Build model
`hal_stm32` assumes that STM32CubeMX owns:
- startup assembly
- linker script
- `SystemClock_Config()`
- `MX_GPIO_Init()` and other low-level peripheral initialization
- CMSIS and STM32 HAL source trees

SFW owns:
- CMake target composition
- C++ HAL adapters such as `hal_stm32::DigitalOutput`
- reusable device drivers and application logic

For STM32 builds, SFW automatically extracts from the CubeMX CMake project:
- MCU compile definitions (for example `STM32F401xC`, `USE_HAL_DRIVER`)
- CPU/FPU/float ABI flags from Cube toolchain settings
- source files and header include directories under the Cube project tree

This minimizes per-device edits in SFW CMake when changing MCU part numbers or
adding generated source/header folders.

## How To Link A CubeMX Project To SFW
1. Create or open an STM32CubeIDE project for the target MCU.
2. Configure the exact target part number and enable the peripherals or GPIO
   pins you need.
3. Configure the GPIO pin that SFW will drive as a generated output pin.
4. Generate code.
5. Copy or sync the generated project into
   `external/cubemx/generic-stm32f401cx`.
6. Ensure the generated tree contains:
   - `Core/Inc`
   - `Core/Src`
   - startup assembly either under `Core/Startup` or at the project root
   - `Drivers/STM32F4xx_HAL_Driver`
   - `Drivers/CMSIS`
   - one linker script matching `*_FLASH.ld`
7. Configure the STM32 preset:

```bash
cmake --preset stm32-debug
```

If your CubeMX project lives elsewhere, override the path:

```bash
cmake --preset stm32-debug \
  -DSFW_STM32_CUBEMX_DIR=/absolute/path/to/your/cubemx/project
```

The preset name no longer encodes the MCU part number because SFW derives that
configuration from the CubeMX-generated CMake files.

## Integration contract for DigitalOutput
`hal_stm32::DigitalOutput` does not configure pin muxing itself. The pin must
already be configured by CubeMX, and `MX_GPIO_Init()` must run before
`DigitalOutput::Initialize()` is called.

For the current STM32 app scaffold, LED blinking uses `LED_BLUE_GPIO_Port` and
`LED_BLUE_Pin` from CubeMX `main.h` (your PC13 configuration).

## Default firmware targets
When `SFW_PLATFORM=STM32`, the top-level `sfw` target emits:
- ELF image through the normal CMake target output
- HEX image beside the ELF
- BIN image beside the ELF
- MAP file in the build directory

If the host has `STM32_Programmer_CLI`, the build exposes
`sfw_flash_cubeprogrammer`.

If the host has `openocd`, the build exposes `sfw_flash_openocd` using
`interface/stlink.cfg` and `target/stm32f4x.cfg`.