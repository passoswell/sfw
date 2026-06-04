# HAL STM32 Module

STM32CubeMX-backed implementations of selected `hal_interface` contracts.

## Table of contents
- [Overview](#overview)
- [Implemented modules](#implemented-modules)
- [Dependencies](#dependencies)
- [Build model](#build-model)
- [How To Link A CubeMX Project To SFW](#how-to-link-a-cubemx-project-to-sfw)
- [Required CubeMX main.c/main.h changes](#required-cubemx-maincmainh-changes)
- [Build](#build)
- [Integration contract for DigitalOutput](#integration-contract-for-digitaloutput)
- [Default firmware targets](#default-firmware-targets)
- [Build and Debug in VS Code](#build-and-debug-in-vs-code)
- [Notes](#notes)

## Overview

## Implemented modules
- `DigitalOutput` for STM32 projects generated with STM32CubeMX /
   STM32CubeIDE.

## Dependencies
Required host tools:
- `cmake` (3.23+)
- `ninja-build`
- `arm-none-eabi-gcc`
- `arm-none-eabi-g++`
- `arm-none-eabi-objcopy`
- `arm-none-eabi-size`

Required STM32 tooling:
- STM32CubeIDE or STM32CubeMX capable of generating CMake projects
- `STM32_Programmer_CLI` (STM32CubeProgrammer)

Recommended for debug in this repository:
- `ST-LINK_gdbserver`
- `arm-none-eabi-gdb`

Optional:
- `openocd`

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
- MCU compile definitions (for example `STM32H503xx`, `STM32F401xC`,
  `USE_HAL_DRIVER`)
- CPU/FPU/float ABI flags from Cube toolchain settings
- source files and header include directories under the Cube project tree

This minimizes per-device edits in SFW CMake when changing MCU part numbers or
adding generated source/header folders.

## How To Link A CubeMX Project To SFW
1. Create an STM32 CubeMX project for the target MCU.
2. Configure the exact target part number and enable the peripherals or GPIO
   pins you need.
3. Under "Project Manager", configure the project name and project location. SFW
   expects the name to be "generic" and the path to be inside "external/cubemx"
   folder by default.
4. Under "Project Manager", select "CMake" on Toolchain/IDE.
5. Under "Project Manager -> Code Generator", mark the option that says
   "Generate peripheral initialization as a pair of '.c/.h' files per peripheral".
6. Generate code.
7. Copy or sync the generated project into `external/cubemx/generic`, or keep
   it in another location and pass that path via `SFW_STM32_CUBEMX_DIR`.
8. Ensure the generated tree contains:
   - `Core/Inc`
   - `Core/Src`
   - startup assembly either under `Core/Startup` or at the project root
   - `Drivers/STM32*xx_HAL_Driver` for your MCU family
   - `Drivers/CMSIS`
   - one linker script matching `*_FLASH.ld`
7. Update the generated CubeMX startup entry points as described in
   [Required CubeMX main.c/main.h changes](#required-cubemx-maincmainh-changes).
8. Configure the STM32 preset:

```bash
cmake --preset stm32-debug
```

If your CubeMX project lives elsewhere, override the path:

```bash
cmake --preset stm32-debug \
  -DSFW_STM32_CUBEMX_DIR=/absolute/path/to/your/cubemx/project
```

The preset name does not encodes the MCU part number because SFW derives that
configuration from the CubeMX-generated CMake files.

## Required CubeMX main.c/main.h changes
SFW uses `apps/stm32/main.cpp` as the program entry point, so generated
`main.c` must provide initialization functions instead of a competing `main()`.

In `Core/Src/main.c`:
1. Keep CubeMX includes and peripheral init code.
2. Disable the generated `main(void)` body (for example with `#if 0` / `#endif`
   around the generated entry function).
3. Add a `void Stm32Initialize(void)` function that runs initialization steps
   previously in `main()` before the infinite loop, such as:
   - `HAL_Init()`
   - `SystemClock_Config()`
   - `MX_GPIO_Init()`
   - other `MX_*_Init()` calls generated for enabled peripherals
4. Add `void Stm32KernelInitialize(void)` if you need the initialization routines
   for any RTOS.

In `Core/Inc/main.h` inside `/* USER CODE BEGIN EFP */`:
1. Declare:
   - `void Stm32Initialize(void);`
   - `void Stm32KernelInitialize(void);`
2. Keep `Error_Handler(void)` declaration.

Regeneration tip:
- Keep custom declarations and wrappers inside `USER CODE` blocks where
  possible, then re-check these functions after each CubeMX code generation.

The result will be the following:

On main.c, betwiin `/* USER CODE BEGIN 0 */` and `/* USER CODE END 1 */`.

```c
/* Core/Src/main.c */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#if 0
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
#endif

void Stm32Initialize(void) {
  /* USER CODE END 1 */
```


On main.c, betwiin `/* USER CODE BEGIN 2 */` and `/* USER CODE END 2 */`.


```c
  /* USER CODE BEGIN 2 */
}

void Stm32KernelInitialize(void) {
  /* USER CODE END 2 */
```


On main.h file:

```c
/* Core/Inc/main.h */
/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void Stm32Initialize(void);
void Stm32KernelInitialize(void);
/* USER CODE END EFP */
```

## Build
From repository root:

1. Configure:

```bash
cmake --preset stm32-debug
```

2. Build:

```bash
cmake --build --preset build-stm32-debug
```

The STM32 app target currently built by this repository is `stm32_tests`.

## Integration contract for DigitalOutput
`hal_stm32::DigitalOutput` does not configure pin muxing itself. The pin must
already be configured by CubeMX, and `MX_GPIO_Init()` must run before
`DigitalOutput::Initialize()` is called.

For the current STM32 app scaffold, LED blinking in `apps/stm32/main.cpp`
targets `GPIOC` pin `13`.

## Default firmware targets
When `SFW_PLATFORM=STM32`, each STM32 executable target emits:
- firmware ELF copy: `${build_dir}/${target}.firmware.elf`
- HEX image beside target output
- BIN image beside target output
- MAP file for the target

If the host has `STM32_Programmer_CLI`, the build exposes
`${target}_flash_cubeprogrammer`.

If the host has `openocd`, the build exposes `${target}_flash_openocd` using
`interface/stlink.cfg` and an auto-generated target script:
`${build_dir}/openocd-target.auto.cfg`.

The build also exposes `${target}_flash`, which resolves to an available flash
backend.

## Build and Debug in VS Code
Required VS Code extensions:
- CMake Tools (`ms-vscode.cmake-tools`)
- C/C++ (`ms-vscode.cpptools`)
- Cortex-Debug (`marus25.cortex-debug`)

Recommended extension:
- C/C++ Extension Pack (`ms-vscode.cpptools-extension-pack`)

Step-by-step:
1. Open repository root in VS Code.
2. Install the required extensions listed above.
3. Open Command Palette and run `CMake: Select Configure Preset`, then choose
   `stm32-debug`.
4. Run `CMake: Configure`.
5. Run `CMake: Build`.
6. In Run and Debug, select `STM32 Debug (Cortex)` from
   `.vscode/launch.json`.
7. Start Debug (`F5`).
8. If needed, stop stale debug servers by running task `stm32-openocd-stop`,
   then start debug again.

Expected debug flow in this repository:
- Pre-launch task starts ST-LINK GDB server through
  `tools/start-stm32-gdbserver.sh`.
- AP selection is automatic by default.
- Cortex-Debug attaches to `localhost:50000`.

Troubleshooting:
- Verify `SFW_STM32_CUBEMX_DIR` points to the intended CubeMX project.
- Verify ST-LINK firmware is compatible with installed `ST-LINK_gdbserver`.
- Verify `arm-none-eabi-gdb`, `STM32_Programmer_CLI`, and
  `ST-LINK_gdbserver` are available in your environment.

## Notes
The debug with VScode functionality is currently not working properly.

The repository debug flow uses VS Code Cortex-Debug with:
- an external GDB target on `localhost:50000`
- pre-launch task `stm32-gdbserver-start`
- auto AP selection in `tools/start-stm32-gdbserver.sh`

If debug fails after changing boards, first check:
- selected CMake preset (`stm32-*`)
- `SFW_STM32_CUBEMX_DIR` points to the intended CubeMX tree
- ST-LINK firmware is up to date for your installed `ST-LINK_gdbserver`