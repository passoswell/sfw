#!/usr/bin/env bash

set -euo pipefail

PRESET="${SFW_ACTIVE_CONFIGURE_PRESET:-}"
WORKSPACE="${SFW_WORKSPACE:-$PWD}"
OPENOCD_BIN="/home/user/.espressif/tools/openocd-esp32/v0.12.0-esp32-20240318/openocd-esp32/bin/openocd"
ARM_GDB="/opt/gcc-arm-none-eabi-10-2020-q4-major/bin/arm-none-eabi-gdb"
HOST_GDB="/usr/bin/gdb"

if [[ "${PRESET}" == stm32* ]]; then
  BUILD_DIR="${WORKSPACE}/build/${PRESET}"
  OPENOCD_TARGET_SCRIPT="${BUILD_DIR}/openocd-target.auto.cfg"
  pkill -f 'openocd.*interface/stlink.cfg' >/dev/null 2>&1 || true

  if [[ -x "${OPENOCD_BIN}" ]]; then
    "${OPENOCD_BIN}" -f interface/stlink.cfg -f "${OPENOCD_TARGET_SCRIPT}" \
      >/tmp/sfw-openocd.log 2>&1 &

    for _ in 1 2 3 4 5 6 7 8 9 10; do
      if grep -q 'Listening on port 3333 for gdb connections' \
        /tmp/sfw-openocd.log 2>/dev/null; then
        break
      fi
      sleep 0.2
    done
  fi

  exec "${ARM_GDB}" "$@"
fi

exec "${HOST_GDB}" "$@"
