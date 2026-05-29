// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "device/serial_eeprom/serial_eeprom.hpp"

#include <algorithm>
#include <array>

namespace sfw::device::serial_eeprom {

SerialEeprom::SerialEeprom(SerialEepromTransportInterface& transport,
                           const hal_interface::MemoryMetadata& metadata,
                           uint8_t erased_value)
  : transport_(transport), metadata_(metadata), erased_value_(erased_value) {
}

hal_interface::ErrorCode SerialEeprom::Initialize() {
  if (initialized_) {
    return hal_interface::ErrorCode::kOk;
  }
  if (metadata_.address_size_bytes == 0U
      || metadata_.address_size_bytes > kMaxAddressSizeBytes) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }
  if (metadata_.size_bytes == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  hal_interface::ErrorCode result = transport_.Initialize();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  initialized_ = true;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode SerialEeprom::Deinitialize() {
  if (!initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  hal_interface::ErrorCode result = transport_.Deinitialize();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  initialized_ = false;
  return hal_interface::ErrorCode::kOk;
}

bool SerialEeprom::IsInitialized() {
  return initialized_;
}

hal_interface::ErrorCode SerialEeprom::Read(uint64_t start_address,
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
    return hal_interface::ErrorCode::kOutOfRange;
  }

  uint64_t offset = 0U;
  while (offset < buffer.size()) {
    const uint64_t CurrentAddress = start_address + offset;
    const uint64_t Remaining = static_cast<uint64_t>(buffer.size()) - offset;
    const uint32_t ChunkSize = ResolveReadChunkSize(Remaining);

    uint32_t encoded_address = 0U;
    hal_interface::ErrorCode result =
        EncodeAddress(CurrentAddress, encoded_address);
    if (result != hal_interface::ErrorCode::kOk) {
      return result;
    }

    std::span<uint8_t> chunk = buffer.subspan(
        static_cast<std::span<uint8_t>::size_type>(offset), ChunkSize);
    result = transport_.ReadFromAddress(
        encoded_address, metadata_.address_size_bytes, chunk, timeout_ms);
    if (result != hal_interface::ErrorCode::kOk) {
      return result;
    }

    offset += ChunkSize;
  }

  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode SerialEeprom::Write(uint64_t start_address,
                                             std::span<const uint8_t> buffer,
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
    return hal_interface::ErrorCode::kOutOfRange;
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

    uint32_t encoded_address = 0U;
    hal_interface::ErrorCode result =
        EncodeAddress(CurrentAddress, encoded_address);
    if (result != hal_interface::ErrorCode::kOk) {
      return result;
    }

    const std::span<const uint8_t> DataChunk = buffer.subspan(
        static_cast<std::span<const uint8_t>::size_type>(offset), ChunkSize);

    result = transport_.WriteToAddress(
        encoded_address, metadata_.address_size_bytes, DataChunk, timeout_ms);
    if (result != hal_interface::ErrorCode::kOk) {
      return result;
    }

    result = transport_.WaitUntilReady(timeout_ms);
    if (result != hal_interface::ErrorCode::kOk) {
      return result;
    }

    offset += ChunkSize;
  }

  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode SerialEeprom::EraseBlock(
    uint64_t address_within_sector, uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }
  if (metadata_.erase_block_size == 0U) {
    return hal_interface::ErrorCode::kNotSupported;
  }

  const uint64_t BlockStart =
      address_within_sector
      - (address_within_sector % metadata_.erase_block_size);
  std::array<uint8_t, kEraseBufferSizeBytes> erased_block{};
  erased_block.fill(erased_value_);

  uint64_t offset = 0U;
  while (offset < metadata_.erase_block_size) {
    const uint64_t Remaining = metadata_.erase_block_size - offset;
    uint32_t chunk_size = static_cast<uint32_t>(
        std::min<uint64_t>(Remaining, erased_block.size()));
    if (metadata_.write_unit > 1U) {
      chunk_size -= (chunk_size % metadata_.write_unit);
    }
    if (chunk_size == 0U) {
      return hal_interface::ErrorCode::kInvalidArgument;
    }

    const std::span<const uint8_t> Chunk(erased_block.data(), chunk_size);
    const hal_interface::ErrorCode Result =
        Write(BlockStart + offset, Chunk, timeout_ms);
    if (Result != hal_interface::ErrorCode::kOk) {
      return Result;
    }

    offset += chunk_size;
  }

  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode SerialEeprom::EraseAllMemory(uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  if (metadata_.erase_block_size == 0U) {
    return hal_interface::ErrorCode::kNotSupported;
  }

  uint64_t address = 0U;
  while (address < metadata_.size_bytes) {
    const hal_interface::ErrorCode Result = EraseBlock(address, timeout_ms);
    if (Result != hal_interface::ErrorCode::kOk) {
      return Result;
    }

    address += metadata_.erase_block_size;
  }

  return hal_interface::ErrorCode::kOk;
}

hal_interface::MemoryMetadata SerialEeprom::GetMetadata() const {
  return metadata_;
}

bool SerialEeprom::IsRangeValid(uint64_t start_address,
                                uint64_t size_bytes) const {
  if (size_bytes == 0U) {
    return true;
  }
  if (start_address >= metadata_.size_bytes) {
    return false;
  }
  return size_bytes <= (metadata_.size_bytes - start_address);
}

hal_interface::ErrorCode SerialEeprom::EncodeAddress(uint64_t relative_address,
                                                     uint32_t& output) const {
  const uint64_t AbsoluteAddress = relative_address + metadata_.base_address;
  const auto AddressBits =
      static_cast<uint8_t>(metadata_.address_size_bytes * kBitsPerByte);
  if (AddressBits < kMaxAddressBits && (AbsoluteAddress >> AddressBits) != 0U) {
    return hal_interface::ErrorCode::kOutOfRange;
  }

  output = static_cast<uint32_t>(AbsoluteAddress);

  return hal_interface::ErrorCode::kOk;
}

uint32_t SerialEeprom::ResolveReadChunkSize(uint64_t remaining) const {
  const uint64_t Configured =
      metadata_.read_block_size == 0U ? remaining : metadata_.read_block_size;
  return static_cast<uint32_t>(std::min<uint64_t>(remaining, Configured));
}

uint32_t SerialEeprom::ResolveWriteChunkSize(uint64_t current_address,
                                             uint64_t remaining) const {
  const uint32_t PageSize = metadata_.write_block_size;
  if (PageSize == 0U) {
    return static_cast<uint32_t>(remaining);
  }

  const auto OffsetInPage = static_cast<uint32_t>(current_address % PageSize);
  const uint32_t BytesToPageEnd = PageSize - OffsetInPage;
  return static_cast<uint32_t>(
      std::min<uint64_t>(remaining, static_cast<uint64_t>(BytesToPageEnd)));
}

}  // namespace sfw::device::serial_eeprom
