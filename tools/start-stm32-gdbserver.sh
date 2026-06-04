#!/usr/bin/env bash

set -euo pipefail

WORKSPACE="${SFW_WORKSPACE:-$PWD}"
PRESET="${SFW_ACTIVE_CONFIGURE_PRESET:-}"

if [[ -z "${PRESET}" ]]; then
  PRESET="${CMAKE_CONFIGURE_PRESET:-stm32-debug}"
fi

BUILD_DIR="${WORKSPACE}/build/${PRESET}"
CMAKE_CACHE="${BUILD_DIR}/CMakeCache.txt"

if [[ ! -f "${CMAKE_CACHE}" ]]; then
  echo "Error: ${CMAKE_CACHE} not found. Configure preset ${PRESET} first."
  exit 1
fi

CUBEPROG_BIN="$(command -v STM32_Programmer_CLI || true)"
STLINK_GDBSERVER_BIN="$(command -v ST-LINK_gdbserver || true)"

if [[ -z "${CUBEPROG_BIN}" && -x "/home/user/st/stm32cubeclt/STM32CubeProgrammer/bin/STM32_Programmer_CLI" ]]; then
  CUBEPROG_BIN="/home/user/st/stm32cubeclt/STM32CubeProgrammer/bin/STM32_Programmer_CLI"
fi

if [[ -z "${STLINK_GDBSERVER_BIN}" && -x "/home/user/st/stm32cubeclt/STLink-gdb-server/bin/ST-LINK_gdbserver" ]]; then
  STLINK_GDBSERVER_BIN="/home/user/st/stm32cubeclt/STLink-gdb-server/bin/ST-LINK_gdbserver"
fi

if [[ -z "${CUBEPROG_BIN}" ]]; then
  echo "Error: STM32_Programmer_CLI not found."
  exit 1
fi

if [[ -z "${STLINK_GDBSERVER_BIN}" ]]; then
  echo "Error: ST-LINK_gdbserver not found."
  exit 1
fi

pkill -f 'openocd.*interface/stlink.cfg' >/dev/null 2>&1 || true
pkill -f 'ST-LINK_gdbserver' >/dev/null 2>&1 || true

APID="${SFW_STLINK_APID:-auto}"
if [[ "${APID}" == "auto" ]]; then
  CONNECT_LOG="$(${CUBEPROG_BIN} -c port=SWD -hardRst 2>&1 || true)"

  # Prefer explicit AP report from CubeProgrammer when available.
  if grep -Eq 'Connection established with AP[[:space:]]+[0-9]+' <<<"${CONNECT_LOG}"; then
    APID="$(grep -Eo 'Connection established with AP[[:space:]]+[0-9]+' <<<"${CONNECT_LOG}" | tail -n 1 | grep -Eo '[0-9]+$')"
  elif grep -Eq 'Connection to AP[[:space:]]+0 requested and failed' <<<"${CONNECT_LOG}"; then
    APID="1"
  else
    APID="0"
  fi
fi

TRY_APS=("${APID}")
if [[ "${SFW_STLINK_APID:-auto}" == "auto" ]]; then
  if [[ "${APID}" == "0" ]]; then
    TRY_APS=("0" "1")
  elif [[ "${APID}" == "1" ]]; then
    TRY_APS=("1" "0")
  fi
fi

for (( INDEX = 0; INDEX < ${#TRY_APS[@]}; ++INDEX )); do
  CANDIDATE_AP="${TRY_APS[${INDEX}]}"
  echo "Using ST-LINK GDB server APID: ${CANDIDATE_AP}"

  SERVER_ARGS=(
    -p 50000
    -cp "$(dirname "${CUBEPROG_BIN}")"
    -d
  )

  if [[ "${CANDIDATE_AP}" != "0" ]]; then
    SERVER_ARGS+=( -m "${CANDIDATE_AP}" )
  fi

  LOG_FILE="$(mktemp /tmp/sfw-stlink-gdbserver.XXXXXX.log)"
  set +e
  "${STLINK_GDBSERVER_BIN}" "${SERVER_ARGS[@]}" 2>&1 | tee "${LOG_FILE}"
  SERVER_RC=${PIPESTATUS[0]}
  set -e

  if [[ ${SERVER_RC} -eq 0 ]]; then
    exit 0
  fi

  if grep -qi 'firmware upgrade required' "${LOG_FILE}"; then
    echo "Error: ST-LINK firmware upgrade required by ST-LINK_gdbserver."
    echo "Update probe firmware with STM32CubeProgrammer, then retry debug."
    exit ${SERVER_RC}
  fi

  if [[ ${INDEX} -lt $((${#TRY_APS[@]} - 1)) ]]; then
    echo "AP ${CANDIDATE_AP} failed. Retrying with AP ${TRY_APS[$((INDEX + 1))]}..."
    continue
  fi

  exit ${SERVER_RC}
done

exit 1
