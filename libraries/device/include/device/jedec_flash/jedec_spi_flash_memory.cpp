// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "device/jedec_flash/jedec_spi_flash_memory.hpp"

#include <algorithm>
#include <limits>

namespace sfw::device::jedec_flash {

JedecSpiFlashMemory::JedecSpiFlashMemory(
    JedecSpiFlash& flash_device, const hal_interface::MemoryMetadata& metadata)
  : flash_device_(flash_device), metadata_(metadata), has_user_metadata_(true) {
}

JedecSpiFlashMemory::~JedecSpiFlashMemory() {
  (void)this->Deinitialize();  // Using this pointer to solve "pure virtual
                               // method called" issue in base class destructor
}

hal_interface::ErrorCode JedecSpiFlashMemory::Initialize() {
  if (initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  const hal_interface::ErrorCode Result = flash_device_.Initialize();
  if (Result != hal_interface::ErrorCode::kOk) {
    return Result;
  }

  if (!has_user_metadata_) {
    if (!PopulateMetadataFromJedecInfo()) {
      return hal_interface::ErrorCode::kError;
    }
  }

  initialized_ = true;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode JedecSpiFlashMemory::Deinitialize() {
  if (!initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  const hal_interface::ErrorCode Result = flash_device_.Deinitialize();
  if (Result != hal_interface::ErrorCode::kOk) {
    return Result;
  }

  initialized_ = false;
  return hal_interface::ErrorCode::kOk;
}

bool JedecSpiFlashMemory::IsInitialized() {
  return initialized_;
}

hal_interface::ErrorCode JedecSpiFlashMemory::Read(uint64_t start_address,
                                                   std::span<uint8_t> buffer,
                                                   uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }
  if (buffer.empty()) {
    return hal_interface::ErrorCode::kOk;
  }

  if (!IsRangeValid(start_address, buffer.size())) {
    return hal_interface::ErrorCode::kError;
  }

  uint64_t offset = 0U;
  while (offset < buffer.size()) {
    uint64_t current_address = start_address + offset;
    uint64_t remaining = static_cast<uint64_t>(buffer.size()) - offset;
    uint32_t chunk_size = ResolveReadChunkSize(remaining);

    uint32_t device_address{};
    hal_interface::ErrorCode result =
        TranslateAddress(current_address, device_address);
    if (result != hal_interface::ErrorCode::kOk) {
      return result;
    }

    std::span<uint8_t> chunk = buffer.subspan(
        static_cast<std::span<uint8_t>::size_type>(offset), chunk_size);
    result = flash_device_.ReadData(device_address, kAddressSizeBytes, chunk,
                                    timeout_ms);
    if (result != hal_interface::ErrorCode::kOk) {
      return result;
    }

    offset += chunk_size;
  }

  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode JedecSpiFlashMemory::Write(
    uint64_t start_address, std::span<const uint8_t> buffer,
    uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }
  if (buffer.empty()) {
    return hal_interface::ErrorCode::kOk;
  }

  if (!IsRangeValid(start_address, buffer.size())) {
    return hal_interface::ErrorCode::kError;
  }

  if (metadata_.write_alignment > 0U
      && (start_address % metadata_.write_alignment) != 0U) {
    return hal_interface::ErrorCode::kError;
  }

  if (metadata_.write_unit > 0U
      && (buffer.size() % metadata_.write_unit) != 0U) {
    return hal_interface::ErrorCode::kError;
  }

  uint64_t offset = 0U;
  while (offset < buffer.size()) {
    const uint64_t CurrentAddress = start_address + offset;
    const uint64_t Remaining = static_cast<uint64_t>(buffer.size()) - offset;
    const uint32_t ChunkSize = ResolveWriteChunkSize(CurrentAddress, Remaining);

    uint32_t device_address{};
    const hal_interface::ErrorCode TranslateResult =
        TranslateAddress(CurrentAddress, device_address);
    if (TranslateResult != hal_interface::ErrorCode::kOk) {
      return TranslateResult;
    }

    const std::span<const uint8_t> Chunk = buffer.subspan(
        static_cast<std::span<const uint8_t>::size_type>(offset), ChunkSize);
    const hal_interface::ErrorCode WriteResult = flash_device_.ProgramPage(
        device_address, kAddressSizeBytes, Chunk, timeout_ms);
    if (WriteResult != hal_interface::ErrorCode::kOk) {
      return WriteResult;
    }

    offset += ChunkSize;
  }

  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode JedecSpiFlashMemory::EraseBlock(
    uint64_t address_within_sector, uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }
  if (metadata_.erase_block_size == 0U) {
    return hal_interface::ErrorCode::kError;
  }
  if (!IsRangeValid(address_within_sector, 1U)) {
    return hal_interface::ErrorCode::kError;
  }

  const uint64_t SectorStart =
      address_within_sector
      - (address_within_sector % metadata_.erase_block_size);
  if (SectorStart > std::numeric_limits<uint32_t>::max()) {
    return hal_interface::ErrorCode::kNotSupported;
  }

  return flash_device_.EraseSector(static_cast<uint32_t>(SectorStart),
                                   kAddressSizeBytes, timeout_ms);
}

hal_interface::ErrorCode JedecSpiFlashMemory::EraseAllMemory(
    uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  return flash_device_.EraseChip(timeout_ms);
}

hal_interface::MemoryMetadata JedecSpiFlashMemory::GetMetadata() const {
  return metadata_;
}

bool JedecSpiFlashMemory::IsRangeValid(uint64_t start_address,
                                       uint64_t size_bytes) const {
  if (size_bytes == 0U) {
    return true;
  }

  if (start_address >= metadata_.size_bytes) {
    return false;
  }

  if (size_bytes > metadata_.size_bytes - start_address) {
    return false;
  }

  return true;
}

hal_interface::ErrorCode JedecSpiFlashMemory::TranslateAddress(
    uint64_t relative_address, uint32_t& device_address) const {
  if (relative_address > std::numeric_limits<uint32_t>::max()) {
    return hal_interface::ErrorCode::kNotSupported;
  }

  device_address =
      static_cast<uint32_t>(relative_address + metadata_.base_address);
  return hal_interface::ErrorCode::kOk;
}

uint32_t JedecSpiFlashMemory::ResolveReadChunkSize(uint64_t remaining) const {
  const uint64_t Configured =
      metadata_.read_block_size == 0U ? remaining : metadata_.read_block_size;
  return static_cast<uint32_t>(std::min(remaining, Configured));
}

uint32_t JedecSpiFlashMemory::ResolveWriteChunkSize(uint64_t current_address,
                                                    uint64_t remaining) const {
  const uint32_t PageSize = metadata_.write_block_size;
  if (PageSize == 0U) {
    return static_cast<uint32_t>(remaining);
  }

  const auto OffsetInPage = static_cast<uint32_t>(current_address % PageSize);
  const uint32_t BytesToPageEnd = PageSize - OffsetInPage;

  const uint64_t MaxChunk = std::min<uint64_t>(PageSize, remaining);
  return static_cast<uint32_t>(std::min<uint64_t>(MaxChunk, BytesToPageEnd));
}

bool JedecSpiFlashMemory::PopulateMetadataFromJedecInfo() {
  jedec_flash::Info info{};
  const hal_interface::ErrorCode Result =
      flash_device_.ReadJedecInfo(info, kMetadataReadTimeoutMs);
  if (Result != hal_interface::ErrorCode::kOk) {
    return false;
  }

  metadata_.base_address = 0U;
  metadata_.requires_erase_before_program = true;
  metadata_.erase_cycle_limit = 0U;

  if (info.memory_size_bytes == 0U) {
    return false;
  }
  metadata_.size_bytes = info.memory_size_bytes;
  metadata_.read_block_size = info.memory_size_bytes;

  const uint32_t PageSizeBytes = DecodePageSizeBytes(info);
  if (PageSizeBytes == 0U) {
    return false;
  }
  metadata_.write_block_size = PageSizeBytes;
  metadata_.write_unit = kOneByteValue;
  metadata_.write_alignment = kOneByteValue;

  const EraseTypeSelection EraseType = DecodeSmallestEraseType(info);
  const uint32_t EraseBlockSizeBytes = EraseType.size_bytes;
  if (EraseBlockSizeBytes == 0U) {
    return false;
  }
  metadata_.erase_block_size = EraseBlockSizeBytes;
  metadata_.erase_block_alignment = EraseBlockSizeBytes;

  const uint32_t MaxWriteTimeUs =
      DecodeMaximumWriteTimeUs(info.program_timing_dword_raw);
  if (MaxWriteTimeUs == 0U) {
    return false;
  }
  metadata_.maximum_write_time_us = MaxWriteTimeUs;

  const uint32_t MaxSectorEraseTimeMs = DecodeMaximumSectorEraseTimeMs(
      info.erase_timing_dword_raw, EraseType.erase_type_index);
  if (MaxSectorEraseTimeMs == 0U) {
    return false;
  }
  metadata_.maximum_sector_erase_time_ms = MaxSectorEraseTimeMs;

  const uint32_t MaxChipEraseTimeMs =
      DecodeMaximumChipEraseTimeMs(info.program_timing_dword_raw);
  if (MaxChipEraseTimeMs == 0U) {
    return false;
  }
  metadata_.maximum_memory_erase_time_ms = MaxChipEraseTimeMs;

  (void)info.erase_to_suspend_resume_dword_raw;

  return true;
}

uint32_t JedecSpiFlashMemory::SaturateToU32(uint64_t value) {
  if (value > std::numeric_limits<uint32_t>::max()) {
    return std::numeric_limits<uint32_t>::max();
  }
  return static_cast<uint32_t>(value);
}

uint32_t JedecSpiFlashMemory::DecodeTypicalEraseTimeMs(
    uint32_t erase_timing_dword, uint8_t erase_type_index) {
  if (erase_type_index == 0U || erase_type_index > 4U) {
    return 0U;
  }

  const auto FieldShift =
      static_cast<uint8_t>(kEraseTypeTimeEntriesBaseShift
                           + static_cast<uint8_t>(erase_type_index - 1U)
                                 * kEraseTypeTimeFieldWidthBits);
  const auto Count = static_cast<uint8_t>(
      1U + ((erase_timing_dword >> FieldShift) & kEraseTypeCountMask));
  const auto Unit =
      static_cast<uint8_t>((erase_timing_dword >> static_cast<uint8_t>(
                                FieldShift + kEraseTypeTimeCountWidthBits))
                           & kEraseTypeUnitMask);

  uint64_t typ_ms = 0U;
  switch (Unit) {
    case 0U:
      typ_ms = Count;
      break;
    case 1U:
      typ_ms = static_cast<uint64_t>(Count) * kEraseTimeUnit16Ms;
      break;
    case 2U:
      typ_ms = static_cast<uint64_t>(Count) * kEraseTimeUnit128Ms;
      break;
    case 3U:
      typ_ms = static_cast<uint64_t>(Count) * kMsPerSecond;
      break;
    default:
      return 0U;
  }

  return JedecSpiFlashMemory::SaturateToU32(typ_ms);
}

uint32_t JedecSpiFlashMemory::DecodeTypicalToMaxFactor(uint32_t dword) {
  const uint64_t Factor =
      static_cast<uint64_t>(2U) * (1U + (dword & kMaxFactorMask));
  return JedecSpiFlashMemory::SaturateToU32(Factor);
}

uint32_t JedecSpiFlashMemory::DecodeMaximumSectorEraseTimeMs(
    uint32_t erase_timing, uint8_t erase_type_idx) {
  const uint32_t TypMs = JedecSpiFlashMemory::DecodeTypicalEraseTimeMs(
      erase_timing, erase_type_idx);
  if (TypMs == 0U) {
    return 0U;
  }

  const uint32_t Factor =
      JedecSpiFlashMemory::DecodeTypicalToMaxFactor(erase_timing);
  return JedecSpiFlashMemory::SaturateToU32(static_cast<uint64_t>(TypMs)
                                            * Factor);
}

uint32_t JedecSpiFlashMemory::DecodeMaximumWriteTimeUs(
    uint32_t program_timing_dword) {
  uint64_t typ_us = 1U
                    + ((program_timing_dword >> kPageProgramTimeCountShift)
                       & kPageProgramTimeCountMask);
  if ((program_timing_dword & (kOneByteValue << kPageProgramScaleBit)) != 0U) {
    typ_us *= kPageProgramScaleLargeUs;
  } else {
    typ_us *= kPageProgramScaleSmallUs;
  }

  const uint32_t Factor =
      JedecSpiFlashMemory::DecodeTypicalToMaxFactor(program_timing_dword);
  return JedecSpiFlashMemory::SaturateToU32(typ_us * Factor);
}

uint32_t JedecSpiFlashMemory::DecodeMaximumChipEraseTimeMs(
    uint32_t program_timing_dword) {
  uint64_t typ_ms = 1U
                    + ((program_timing_dword >> kChipEraseTimeCountShift)
                       & kChipEraseTimeCountMask);
  const auto Unit = static_cast<uint8_t>(
      (program_timing_dword >> kChipEraseTimeUnitShift) & kEraseTypeUnitMask);
  switch (Unit) {
    case 0U:
      typ_ms *= kEraseTimeUnit16Ms;
      break;
    case 1U:
      typ_ms *= kChipEraseTimeUnit256Ms;
      break;
    case 2U:
      typ_ms *= kChipEraseTimeUnit4SInMs;
      break;
    case 3U:
      typ_ms *= kChipEraseTimeUnit64SInMs;
      break;
    default:
      return 0U;
  }

  const uint32_t Factor =
      JedecSpiFlashMemory::DecodeTypicalToMaxFactor(program_timing_dword);
  return JedecSpiFlashMemory::SaturateToU32(typ_ms * Factor);
}

uint32_t JedecSpiFlashMemory::DecodeEraseTypeSizeBytes(uint32_t dword,
                                                       uint8_t opcode_shift,
                                                       uint8_t size_shift) {
  const uint32_t Opcode = (dword >> opcode_shift) & kMetadataByteMask;
  const uint32_t SizeExponent = (dword >> size_shift) & kMetadataByteMask;
  if (Opcode == 0U || SizeExponent == 0U
      || SizeExponent > kMaxSafeShiftBitsU32) {
    return 0U;
  }

  return static_cast<uint32_t>(kOneByteValue << SizeExponent);
}

JedecSpiFlashMemory::EraseTypeSelection
JedecSpiFlashMemory::DecodeSmallestEraseType(const jedec_flash::Info& info) {
  if (info.bfpt_dword_count_read <= kBfptDwordEraseType12Index) {
    return {};
  }

  JedecSpiFlashMemory::EraseTypeSelection selection{};
  const auto UpdateSmallest = [&selection](uint32_t size_bytes,
                                           uint8_t erase_type_index) {
    if (size_bytes == 0U || erase_type_index == 0U) {
      return;
    }
    if (selection.size_bytes == 0U || size_bytes < selection.size_bytes) {
      selection.size_bytes = size_bytes;
      selection.erase_type_index = erase_type_index;
    }
  };

  const uint32_t Dword8 = info.bfpt_dwords.at(kBfptDwordEraseType12Index);
  UpdateSmallest(JedecSpiFlashMemory::DecodeEraseTypeSizeBytes(
                     Dword8, kEraseType1OpcodeShift, kEraseType1SizeShift),
                 1U);
  UpdateSmallest(JedecSpiFlashMemory::DecodeEraseTypeSizeBytes(
                     Dword8, kEraseType2OpcodeShift, kEraseType2SizeShift),
                 2U);

  if (info.bfpt_dword_count_read > kBfptDwordEraseType34Index) {
    const uint32_t Dword9 = info.bfpt_dwords.at(kBfptDwordEraseType34Index);
    UpdateSmallest(JedecSpiFlashMemory::DecodeEraseTypeSizeBytes(
                       Dword9, kEraseType1OpcodeShift, kEraseType1SizeShift),
                   3U);
    UpdateSmallest(JedecSpiFlashMemory::DecodeEraseTypeSizeBytes(
                       Dword9, kEraseType2OpcodeShift, kEraseType2SizeShift),
                   4U);
  }

  return selection;
}

uint32_t JedecSpiFlashMemory::DecodePageSizeBytes(
    const jedec_flash::Info& info) {
  const uint32_t Exponent =
      (info.program_timing_dword_raw >> kPageSizeExponentShift)
      & kPageSizeExponentMask;
  if (Exponent == 0U || Exponent > kMaxSafeShiftBitsU32) {
    return 0U;
  }

  return static_cast<uint32_t>(kOneByteValue << Exponent);
}

JedecSpiFlashMemory::JedecSpiFlashMemory(JedecSpiFlash& flash_device)
  : flash_device_(flash_device), has_user_metadata_(false) {
}

}  // namespace sfw::device::jedec_flash
