// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "device/jedec_flash/jedec_spi_flash.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <span>

namespace sfw::device::jedec_flash {

JedecSpiFlash::JedecSpiFlash(hal_interface::SpiController& spi,
                             hal_interface::DigitalOutput& cs_pin,
                             bool cs_active_state,
                             hal_interface::SoftwareTimer& timer)
  : JedecSpiFlash(spi, cs_pin, cs_active_state, timer,
                  jedec_flash::Commands()) {
}

JedecSpiFlash::JedecSpiFlash(hal_interface::SpiController& spi,
                             hal_interface::DigitalOutput& cs_pin,
                             bool cs_active_state,
                             hal_interface::SoftwareTimer& timer,
                             const jedec_flash::Commands& command_set)
  : spi_(spi)
  , cs_pin_(cs_pin)
  , cs_active_state_(cs_active_state)
  , timer_(timer)
  , command_set_(command_set) {
}

JedecSpiFlash::~JedecSpiFlash() {
  (void)Deinitialize();
}

hal_interface::ErrorCode JedecSpiFlash::Initialize() {
  if (initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  if (!spi_.IsInitialized()) {
    const hal_interface::ErrorCode SpiResult = spi_.Initialize();
    if (SpiResult != hal_interface::ErrorCode::kOk) {
      return SpiResult;
    }
  }

  if (!cs_pin_.IsInitialized()) {
    const hal_interface::ErrorCode CsResult = cs_pin_.Initialize();
    if (CsResult != hal_interface::ErrorCode::kOk) {
      return CsResult;
    }
  }

  if (!timer_.IsInitialized()) {
    const hal_interface::ErrorCode TimerResult = timer_.Initialize();
    if (TimerResult != hal_interface::ErrorCode::kOk) {
      return TimerResult;
    }
  }

  const hal_interface::ErrorCode CsIdleResult =
      cs_pin_.Write(!cs_active_state_, kControlOpTimeoutMs);
  if (CsIdleResult != hal_interface::ErrorCode::kOk) {
    return CsIdleResult;
  }

  initialized_ = true;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode JedecSpiFlash::Deinitialize() {
  if (!initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  hal_interface::ErrorCode first_error = hal_interface::ErrorCode::kOk;
  const auto UpdateFirstError = [&first_error](hal_interface::ErrorCode code) {
    if (first_error == hal_interface::ErrorCode::kOk
        && code != hal_interface::ErrorCode::kOk) {
      first_error = code;
    }
  };

  UpdateFirstError(cs_pin_.Write(!cs_active_state_, kControlOpTimeoutMs));
  UpdateFirstError(timer_.Deinitialize());
  UpdateFirstError(cs_pin_.Deinitialize());

  if (first_error == hal_interface::ErrorCode::kOk) {
    initialized_ = false;
  }
  return first_error;
}

bool JedecSpiFlash::IsInitialized() const {
  return initialized_;
}

hal_interface::ErrorCode JedecSpiFlash::ReadJedecId(
    std::span<uint8_t, 3> jedec_id, uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  const std::array<uint8_t, 1> TxBuffer{command_set_.read_jedec_id};
  return spi_.WriteThenRead(cs_pin_, cs_active_state_, TxBuffer, jedec_id,
                            timeout_ms);
}

hal_interface::ErrorCode JedecSpiFlash::ReadJedecInfo(jedec_flash::Info& info,
                                                      uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  info = jedec_flash::Info{};

  const hal_interface::ErrorCode JedecIdResult =
      ReadJedecId(std::span<uint8_t, 3>(info.jedec_id), timeout_ms);
  if (JedecIdResult != hal_interface::ErrorCode::kOk) {
    return JedecIdResult;
  }

  std::array<uint8_t, kSfdpHeaderSizeBytes> sfdp_header{};
  const hal_interface::ErrorCode SfdpHeaderResult =
      ReadSfdpBytes(0U, std::span<uint8_t>(sfdp_header), timeout_ms);
  if (SfdpHeaderResult != hal_interface::ErrorCode::kOk) {
    return SfdpHeaderResult;
  }

  const uint32_t Signature = DecodeLe32(
      std::span<const uint8_t, 4>(sfdp_header.data(), kBytesPerDword));
  if (Signature != kSfdpSignature) {
    return hal_interface::ErrorCode::kNotSupported;
  }

  info.sfdp_available = true;
  info.sfdp_minor_revision = sfdp_header[kSfdpHeaderMinorRevisionIndex];
  info.sfdp_major_revision = sfdp_header[kSfdpHeaderMajorRevisionIndex];
  info.sfdp_parameter_header_count =
      static_cast<uint8_t>(sfdp_header[kSfdpHeaderParameterCountIndex] + 1U);

  uint8_t bfpt_dword_count_available = 0U;
  uint32_t bfpt_pointer = 0U;
  const hal_interface::ErrorCode BfptHeaderResult =
      FindBfptTable(info.sfdp_parameter_header_count, timeout_ms,
                    bfpt_dword_count_available, bfpt_pointer);
  if (BfptHeaderResult != hal_interface::ErrorCode::kOk) {
    return BfptHeaderResult;
  }

  info.bfpt_dword_count_available = bfpt_dword_count_available;
  info.bfpt_pointer = bfpt_pointer;

  const hal_interface::ErrorCode BfptReadResult =
      ReadBfptTable(bfpt_pointer, bfpt_dword_count_available, timeout_ms, info);
  if (BfptReadResult != hal_interface::ErrorCode::kOk) {
    return BfptReadResult;
  }

  PopulateInfoFromBfpt(info);

  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode JedecSpiFlash::FindBfptTable(
    uint8_t parameter_header_count, uint32_t timeout_ms,
    uint8_t& bfpt_dword_count_available, uint32_t& bfpt_pointer) {
  for (uint8_t index = 0U; index < parameter_header_count; ++index) {
    std::array<uint8_t, kSfdpParameterHeaderSizeBytes> parameter_header{};
    const uint32_t HeaderAddress =
        kSfdpHeaderSizeBytes
        + static_cast<uint32_t>(index) * kSfdpParameterHeaderSizeBytes;
    const hal_interface::ErrorCode ReadResult = ReadSfdpBytes(
        HeaderAddress, std::span<uint8_t>(parameter_header), timeout_ms);
    if (ReadResult != hal_interface::ErrorCode::kOk) {
      return ReadResult;
    }

    const uint8_t ParameterIdLsb = parameter_header[kSfdpParameterIdLsbIndex];
    const uint8_t LengthInDwords = parameter_header[kSfdpParameterLengthIndex];
    const auto PointerBytes = std::span<const uint8_t, 3>(
        parameter_header.data() + kSfdpParameterPointerIndex, 3U);
    const uint32_t ParameterPointer = DecodeLe24(PointerBytes);
    const uint8_t ParameterIdMsb = parameter_header[kSfdpParameterIdMsbIndex];

    if (ParameterIdLsb == kBfptIdLsb && ParameterIdMsb == kBfptIdMsb) {
      bfpt_dword_count_available = LengthInDwords;
      bfpt_pointer = ParameterPointer;
      return hal_interface::ErrorCode::kOk;
    }
  }

  return hal_interface::ErrorCode::kNotSupported;
}

hal_interface::ErrorCode JedecSpiFlash::ReadBfptTable(
    uint32_t bfpt_pointer, uint8_t bfpt_dword_count_available,
    uint32_t timeout_ms, jedec_flash::Info& info) {
  const uint8_t BfptDwordsToRead =
      std::min<uint8_t>(kMaxBfptDwordsToRead, bfpt_dword_count_available);
  const uint32_t BfptBytesToRead =
      static_cast<uint32_t>(BfptDwordsToRead) * kBytesPerDword;

  std::array<uint8_t, kMaxBfptDwordsToRead * kBytesPerDword> bfpt_raw_bytes{};
  const hal_interface::ErrorCode ReadResult = ReadSfdpBytes(
      bfpt_pointer, std::span<uint8_t>(bfpt_raw_bytes.data(), BfptBytesToRead),
      timeout_ms);
  if (ReadResult != hal_interface::ErrorCode::kOk) {
    return ReadResult;
  }

  info.bfpt_dword_count_read = BfptDwordsToRead;
  for (uint8_t index = 0U; index < BfptDwordsToRead; ++index) {
    const auto Bytes = std::span<const uint8_t, 4>(
        bfpt_raw_bytes.data() + static_cast<size_t>(index) * kBytesPerDword,
        kBytesPerDword);
    info.bfpt_dwords.at(index) = DecodeLe32(Bytes);
  }

  return hal_interface::ErrorCode::kOk;
}

void JedecSpiFlash::PopulateInfoFromBfpt(jedec_flash::Info& info) {
  if (info.bfpt_dword_count_read > kBfptDwordAddressingIndex) {
    const uint32_t Dword0 = info.bfpt_dwords.at(kBfptDwordAddressingIndex);
    const auto AddressSupportValue = static_cast<uint8_t>(
        (Dword0 >> kAddressSupportBitOffset) & kAddressSupportBitMask);
    info.address_byte_support =
        static_cast<jedec_flash::Info::AddressByteSupport>(AddressSupportValue);
  }

  if (info.bfpt_dword_count_read > kBfptDwordDensityIndex) {
    const uint32_t DensityDescriptor =
        info.bfpt_dwords.at(kBfptDwordDensityIndex);
    if ((DensityDescriptor & kDensityLargeFlag) == 0U) {
      info.density_bits = static_cast<uint64_t>(DensityDescriptor) + 1U;
    } else {
      const uint32_t Exponent = DensityDescriptor & kDensityExponentMask;
      if (Exponent < kMaxSafeShiftBits) {
        info.density_bits = (1ULL << Exponent);
      } else {
        info.density_bits = 0U;
      }
    }

    if (info.density_bits > 0U) {
      const uint64_t FullBytes = info.density_bits / kBitsPerByte;
      const uint64_t HasRemainder =
          (info.density_bits % kBitsPerByte) == 0U ? 0U : 1U;
      info.memory_size_bytes = FullBytes + HasRemainder;
    }
  }

  if (info.bfpt_dword_count_read > kBfptDwordEraseTimingIndex) {
    info.erase_timing_dword_raw =
        info.bfpt_dwords.at(kBfptDwordEraseTimingIndex);
  }
  if (info.bfpt_dword_count_read > kBfptDwordEraseSuspendResumeIndex) {
    info.erase_to_suspend_resume_dword_raw =
        info.bfpt_dwords.at(kBfptDwordEraseSuspendResumeIndex);
  }
  if (info.bfpt_dword_count_read > kBfptDwordProgramTimingIndex) {
    info.program_timing_dword_raw =
        info.bfpt_dwords.at(kBfptDwordProgramTimingIndex);
  }
}

hal_interface::ErrorCode JedecSpiFlash::ReadData(uint32_t address,
                                                 uint8_t address_size_bytes,
                                                 std::span<uint8_t> buffer,
                                                 uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }
  if (!IsAddressSizeSupported(address_size_bytes)) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }
  if (buffer.empty()) {
    return hal_interface::ErrorCode::kOk;
  }

  const auto RegisterSizeBytes = static_cast<uint8_t>(address_size_bytes + 1U);

  const uint32_t RegisterAddress =
      BuildCommandAddress(command_set_.read_data, address, address_size_bytes);

  return spi_.ReadRegister(cs_pin_, cs_active_state_, RegisterAddress,
                           RegisterSizeBytes, buffer, timeout_ms);
}

hal_interface::ErrorCode JedecSpiFlash::ProgramPage(
    uint32_t address, uint8_t address_size_bytes, std::span<const uint8_t> data,
    uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }
  if (!IsAddressSizeSupported(address_size_bytes)) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }
  if (data.empty()) {
    return hal_interface::ErrorCode::kOk;
  }

  hal_interface::ErrorCode code{};
  const auto RegisterSizeBytes = static_cast<uint8_t>(address_size_bytes + 1U);
  const uint32_t RegisterAddress = BuildCommandAddress(
      command_set_.page_program, address, address_size_bytes);

  code = WaitUntilReady(timeout_ms);
  if (code != hal_interface::ErrorCode::kOk) {
    return code;
  }

  code = SetWriteEnable(true, timeout_ms);
  if (code != hal_interface::ErrorCode::kOk) {
    return code;
  }

  code = spi_.WriteRegister(cs_pin_, cs_active_state_, RegisterAddress,
                            RegisterSizeBytes, data, timeout_ms);
  if (code != hal_interface::ErrorCode::kOk) {
    return code;
  }

  code = WaitUntilReady(timeout_ms);
  if (code != hal_interface::ErrorCode::kOk) {
    return code;
  }

  return SetWriteEnable(false, timeout_ms);
}

hal_interface::ErrorCode JedecSpiFlash::EraseSector(uint32_t address,
                                                    uint8_t address_size_bytes,
                                                    uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }
  if (!IsAddressSizeSupported(address_size_bytes)) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  hal_interface::ErrorCode code{};
  const auto RegisterSizeBytes = static_cast<uint8_t>(address_size_bytes + 1U);

  code = WaitUntilReady(timeout_ms);
  if (code != hal_interface::ErrorCode::kOk) {
    return code;
  }

  const uint32_t RegisterAddress = BuildCommandAddress(
      command_set_.sector_erase, address, address_size_bytes);

  std::array<uint8_t, hal_interface::SpiController::kMaxRegAddressSize>
      command_buffer{};
  for (uint8_t index = 0U; index < RegisterSizeBytes; ++index) {
    const auto Shift =
        static_cast<uint8_t>((RegisterSizeBytes - 1U - index) * 8U);
    command_buffer.at(index) = static_cast<uint8_t>(RegisterAddress >> Shift);
  }
  const std::span<const uint8_t> CommandBuffer(command_buffer.data(),
                                               RegisterSizeBytes);

  code = SetWriteEnable(true, timeout_ms);
  if (code != hal_interface::ErrorCode::kOk) {
    return code;
  }

  code = spi_.Write(cs_pin_, cs_active_state_, CommandBuffer, timeout_ms);
  if (code != hal_interface::ErrorCode::kOk) {
    return code;
  }

  code = WaitUntilReady(timeout_ms);
  if (code != hal_interface::ErrorCode::kOk) {
    return code;
  }

  return SetWriteEnable(false, timeout_ms);
}

hal_interface::ErrorCode JedecSpiFlash::EraseChip(uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  hal_interface::ErrorCode code{};

  code = WaitUntilReady(timeout_ms);
  if (code != hal_interface::ErrorCode::kOk) {
    return code;
  }

  code = SetWriteEnable(true, timeout_ms);
  if (code != hal_interface::ErrorCode::kOk) {
    return code;
  }

  const std::array<uint8_t, 1> TxBuffer{command_set_.chip_erase};
  code = spi_.Write(cs_pin_, cs_active_state_, TxBuffer, timeout_ms);
  if (code != hal_interface::ErrorCode::kOk) {
    return code;
  }

  code = WaitUntilReady(timeout_ms);
  if (code != hal_interface::ErrorCode::kOk) {
    return code;
  }

  return SetWriteEnable(false, timeout_ms);
  ;
}

hal_interface::ErrorCode JedecSpiFlash::WaitUntilReady(uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  hal_interface::ErrorCode result = timer_.EnableAutoReload(false);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = timer_.Start(timeout_ms, hal_interface::TimeUnit::kMilliseconds);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  while (true) {
    uint8_t status{};
    const hal_interface::ErrorCode StatusResult =
        ReadStatusRegister1(status, timeout_ms);
    if (StatusResult != hal_interface::ErrorCode::kOk) {
      return StatusResult;
    }

    if ((status & kStatusBusyMask) == 0U) {
      return hal_interface::ErrorCode::kOk;
    }

    if (timer_.HasExpired()) {
      return hal_interface::ErrorCode::kTimeout;
    }

    result =
        timer_.Delay(kBusyPollDelayMs, hal_interface::TimeUnit::kMilliseconds);
    if (result != hal_interface::ErrorCode::kOk) {
      return result;
    }
  }
}

hal_interface::ErrorCode JedecSpiFlash::SetWriteEnable(bool enable,
                                                       uint32_t timeout_ms) {
  const std::array<uint8_t, 1> TxBuffer{enable ? command_set_.write_enable
                                               : command_set_.write_disable};
  return spi_.Write(cs_pin_, cs_active_state_, TxBuffer, timeout_ms);
}

hal_interface::ErrorCode JedecSpiFlash::ReadStatusRegister1(
    uint8_t& status, uint32_t timeout_ms) {
  const std::array<uint8_t, 1> TxBuffer{command_set_.read_status_register_1};
  std::span<uint8_t> rx_buffer(&status, 1);
  return spi_.WriteThenRead(cs_pin_, cs_active_state_, TxBuffer, rx_buffer,
                            timeout_ms);
}

hal_interface::ErrorCode JedecSpiFlash::ReadSfdpBytes(uint32_t sfdp_address,
                                                      std::span<uint8_t> buffer,
                                                      uint32_t timeout_ms) {
  if (buffer.empty()) {
    return hal_interface::ErrorCode::kOk;
  }

  // SFDP always uses command + 24-bit address + 1 dummy byte.
  std::array<uint8_t, kSfdpReadPrefixSizeBytes> tx_buffer{};
  tx_buffer[0] = kSfdpReadOpcode;
  tx_buffer[kSfdpTxAddressMsbIndex] =
      static_cast<uint8_t>((sfdp_address >> kShift16Bits) & kByteMask);
  tx_buffer[kSfdpTxAddressMidIndex] =
      static_cast<uint8_t>((sfdp_address >> kShift8Bits) & kByteMask);
  tx_buffer[kSfdpTxAddressLsbIndex] =
      static_cast<uint8_t>(sfdp_address & kByteMask);
  tx_buffer[kSfdpTxDummyByteIndex] = 0U;

  return spi_.WriteThenRead(cs_pin_, cs_active_state_, tx_buffer, buffer,
                            timeout_ms);
}

bool JedecSpiFlash::IsAddressSizeSupported(uint8_t address_size_bytes) {
  return address_size_bytes >= kMinAddressSizeBytes
         && address_size_bytes <= kMaxAddressSizeBytes;
}

uint32_t JedecSpiFlash::BuildCommandAddress(uint8_t opcode, uint32_t address,
                                            uint8_t address_size_bytes) {
  const auto ShiftBits = static_cast<uint8_t>(address_size_bytes * 8U);
  const uint32_t AddressMask =
      ShiftBits >= kShift32Bits
          ? std::numeric_limits<uint32_t>::max()
          : static_cast<uint32_t>((1ULL << ShiftBits) - 1ULL);
  return (static_cast<uint32_t>(opcode) << ShiftBits) | (address & AddressMask);
}

uint32_t JedecSpiFlash::DecodeLe32(std::span<const uint8_t, 4> bytes) {
  return static_cast<uint32_t>(bytes[0])
         | (static_cast<uint32_t>(bytes[1]) << kShift8Bits)
         | (static_cast<uint32_t>(bytes[2]) << kShift16Bits)
         | (static_cast<uint32_t>(bytes[3]) << kShift24Bits);
}

uint32_t JedecSpiFlash::DecodeLe24(std::span<const uint8_t, 3> bytes) {
  return static_cast<uint32_t>(bytes[0])
         | (static_cast<uint32_t>(bytes[1]) << kShift8Bits)
         | (static_cast<uint32_t>(bytes[2]) << kShift16Bits);
}

}  // namespace sfw::device::jedec_flash
