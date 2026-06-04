#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}"
SELF_PATH="$(readlink -f "$0")"
TARGET_NAME="$(basename "$0")"

if [[ "${TARGET_NAME}" == *.elf ]]; then
  TARGET_NAME="${TARGET_NAME%.elf}"
fi

# Walk up from the script directory to locate the CMake build root.
SEARCH_DIR="${SCRIPT_DIR}"
while [[ ! -f "${SEARCH_DIR}/CMakeCache.txt" ]]; do
  if [[ "${SEARCH_DIR}" == "/" ]]; then
    echo "Error: could not find CMakeCache.txt from ${SCRIPT_DIR}."
    exit 1
  fi
  SEARCH_DIR="$(dirname "${SEARCH_DIR}")"
done
BUILD_DIR="${SEARCH_DIR}"

ELF_PATH=""
for CANDIDATE in \
  "${BUILD_DIR}/${TARGET_NAME}.firmware.elf" \
  "${BUILD_DIR}/${TARGET_NAME}.elf" \
  "${SCRIPT_DIR}/${TARGET_NAME}.firmware.elf" \
  "${SCRIPT_DIR}/${TARGET_NAME}.elf"; do
  if [[ -f "${CANDIDATE}" ]]; then
    ELF_PATH="${CANDIDATE}"
    break
  fi
done

if [[ -z "${ELF_PATH}" ]]; then
  echo "Error: firmware ELF for target ${TARGET_NAME} was not found."
  echo "Searched under ${BUILD_DIR} and ${SCRIPT_DIR}."
  exit 1
fi

if [[ "$(readlink -f "${ELF_PATH}")" == "${SELF_PATH}" ]]; then
  echo "Error: launcher script resolved itself as firmware image."
  echo "Expected a firmware ELF such as sfw.firmware.elf in ${BUILD_DIR}."
  exit 1
fi

if grep -q '^SFW_PLATFORM:STRING=STM32$' "${BUILD_DIR}/CMakeCache.txt"; then
  OPENOCD_TARGET_SCRIPT="${BUILD_DIR}/openocd-target.auto.cfg"
  pkill -f 'openocd.*interface/stlink.cfg' >/dev/null 2>&1 || true

  if command -v STM32_Programmer_CLI >/dev/null 2>&1; then
    echo "Flashing via STM32CubeProgrammer (ST-LINK): ${ELF_PATH}"
    if STM32_Programmer_CLI -c port=SWD -w "${ELF_PATH}" -v -rst; then
      exit 0
    fi

    echo "STM32CubeProgrammer failed; falling back to OpenOCD..."
  fi

  if command -v openocd >/dev/null 2>&1; then
    echo "Flashing via OpenOCD (ST-LINK): ${ELF_PATH}"
    if openocd \
      -f interface/stlink.cfg \
      -f "${OPENOCD_TARGET_SCRIPT}" \
      -c "program ${ELF_PATH} verify reset exit"; then
      exit 0
    fi
  fi

  echo "Error: failed to flash target with STM32_Programmer_CLI and OpenOCD."
  exit 1
fi

exec "${ELF_PATH}"