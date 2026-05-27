/**
 * @file dev_mem_W25Q80.cpp
 * @author user (contact@email.com)
 * @brief Class device flash memory W25Q80
 * @version 0.1
 * @date 2023-04-17
 *
 * @copyright Copyright (c) 2023
 *
 * @warning This is an old and deprecated device driver. For new designs,
 * consider using the JEDEC SPI flash adapter (device/jedec_flash) instead,
 * which supports a wider range of devices including the W25Q80 and provides a
 * more flexible and feature-rich interface.
 *
 */

#include "device/w25q80/dev_mem_W25Q80.hpp"

using StatusT = sfw::hal_interface::ErrorCode;

static constexpr uint32_t kDfDelay = 100U;

/**
 * @brief Initializes the W25Q80 memory.
 *
 * @param port The SPI port number
 * @param cs_port The chip select's GPIO port number
 * @param cs_pin The chip select's pin number
 */
MemW25Q80::MemW25Q80(sfw::hal_interface::SpiController &spi,
                     sfw::hal_interface::DigitalOutput &cs_pin,
                     sfw::hal_interface::SoftwareTimer &timer)
  : spi_(spi), cs_(cs_pin), timer_(timer) {
}

/**
 * @brief Read one byte to 256 bytes from memory
 *
 * @param addr Address first position memory
 * @param rx_buffer Pointer to data buffer
 * @param size Number of bytes
 * @return true if successful, otherwise a different status is returned
 */
StatusT MemW25Q80::Read(uint32_t addr, uint8_t *rx_buffer, uint16_t size) {
  StatusT ret{};

  if (size >= W25Q80_BUFFER_SIZE) {
    return StatusT::kError;
  }

  if (!WaitForTimeout(W25Q80_ERASE_CHIP_TIMEOUT + 1)) {
    return StatusT::kError;
  }
  uint32_t command = addr + (W25Q80_INSTRUCTION_NORMAL_READ << 24);  // NOLINT

  std::span<uint8_t> rx_span(rx_buffer, size);
  ret = spi_.ReadRegister(cs_, false, command, 4, rx_span, kDfDelay);

  return ret;
}

/**
 * @brief Write one byte to 256 bytes from memory
 *
 * @param addr Address first position memory
 * @param tx_buffer Pointer to data buffer
 * @param size Number of bytes
 * @return true if successful, otherwise a different status is returned
 */
StatusT MemW25Q80::Write(uint32_t addr, uint8_t *tx_buffer, uint16_t size) {
  StatusT ret{};

  if (size >= W25Q80_BUFFER_SIZE) {
    return StatusT::kError;
  }

  if (!WaitForTimeout(W25Q80_ERASE_CHIP_TIMEOUT + 1)) {
    return StatusT::kError;
  }

  // Calculate the physical sector address
  uint32_t command = addr + (W25Q80_INSTRUCTION_PAGE_PROGRAM << 24);  // NOLINT

  WriteEnable(true);

  std::span<uint8_t> tx_span(tx_buffer, size);
  ret = spi_.WriteRegister(cs_, false, command, 4, tx_span, kDfDelay);

  if (!WaitForTimeout(W25Q80_ERASE_CHIP_TIMEOUT + 1)) {
    return StatusT::kError;
  }

  WriteEnable(false);

  return ret;
}

/**
 * @brief Read one page (256 bytes) from memory
 *
 * @param addr Address first position memory
 * @param rx_buffer Pointer to data buffer
 * @return true if successful, otherwise a different status is returned
 */
StatusT MemW25Q80::ReadPage(uint32_t addr, uint8_t *rx_buffer) {
  StatusT ret{};

  if (!WaitForTimeout(W25Q80_ERASE_CHIP_TIMEOUT + 1)) {
    return StatusT::kError;
  }

  tx_buffer_[0] = W25Q80_INSTRUCTION_NORMAL_READ;
  tx_buffer_[1] = (addr >> 16) & 0xFF;  // NOLINT
  tx_buffer_[2] = (addr >> 8) & 0xFF;   // NOLINT
  tx_buffer_[3] = addr & 0xFF;          // NOLINT

  //  m_spi.write(tx_buffer_, 4);
  //  ret = m_spi.read(rx_buffer, W25Q80_BUFFER_SIZE);

  std::span<uint8_t> tx_span(tx_buffer_.data(), W25Q80_BUFFER_SIZE + 4);
  std::span<uint8_t> rx_span(rx_buffer_.data(), W25Q80_BUFFER_SIZE + 4);
  ret = spi_.Transfer(cs_, false, tx_span, rx_span, kDfDelay);

  std::memcpy(rx_buffer, rx_buffer_.data() + 4, W25Q80_BUFFER_SIZE);

  return ret;
}

/**
 * @brief Write one page (256 bytes) to memory
 *
 * @param addr Address first position memory
 * @param tx_buffer Pointer to data buffer
 * @return true if successful, otherwise a different status is returned
 */
StatusT MemW25Q80::WritePage(uint32_t addr, uint8_t *tx_buffer) {
  StatusT ret{};

  if (!WaitForTimeout(W25Q80_ERASE_CHIP_TIMEOUT + 1)) {
    return StatusT::kError;
  }

  WriteEnable(true);

  // Calculate the physical sector address
  tx_buffer_[0] = W25Q80_INSTRUCTION_PAGE_PROGRAM;
  tx_buffer_[1] = (addr >> 16) & 0xFF;  // NOLINT
  tx_buffer_[2] = (addr >> 8) & 0xFF;   // NOLINT
  tx_buffer_[3] = addr & 0xFF;          // NOLINT
  std::memcpy(tx_buffer_.data() + 4, tx_buffer, W25Q80_BUFFER_SIZE);

  std::span<uint8_t> tx_span(tx_buffer_.data(), W25Q80_BUFFER_SIZE + 4);
  ret = spi_.Write(cs_, false, tx_span, kDfDelay);

  if (!WaitForTimeout(W25Q80_WRITE_PAGE_TIMEOUT + 1)) {
    return StatusT::kError;
  }

  WriteEnable(false);

  return ret;
}

/**
 * @brief Erase sector (4KB)
 *
 * @param addr Address first position memory
 * @return true if successful, otherwise a different status is returned
 */
StatusT MemW25Q80::EraseSector(uint32_t addr) {
  StatusT ret{};

  if (!WaitForTimeout(W25Q80_ERASE_CHIP_TIMEOUT + 1)) {
    return StatusT::kError;
  }

  WriteEnable(true);

  tx_buffer_[0] = W25Q80_INSTRUCTION_ERASE_SECTOR;
  tx_buffer_[1] = (addr >> 16) & 0xFF;  // NOLINT
  tx_buffer_[2] = (addr >> 8) & 0xFF;   // NOLINT
  tx_buffer_[3] = addr & 0xFF;          // NOLINT

  std::span<uint8_t> tx_span(tx_buffer_.data(), 4);
  ret = spi_.Write(cs_, false, tx_span, kDfDelay);

  WriteEnable(false);

  if (!WaitForTimeout(W25Q80_ERASE_CHIP_TIMEOUT + 1)) {
    return StatusT::kError;
  }

  return ret;
}

/**
 * @brief Erase block (64kb)
 *
 * @param addr Address first position memory
 * @return true if successful, otherwise a different status is returned
 */
StatusT MemW25Q80::EraseBlock64(uint32_t addr) {
  StatusT ret{};

  if (!WaitForTimeout(W25Q80_ERASE_CHIP_TIMEOUT + 1)) {
    return StatusT::kError;
  }

  WriteEnable(true);

  tx_buffer_[0] = W25Q80_INSTRUCTION_ERASE_BLOCK;
  tx_buffer_[1] = (addr >> 16) & 0xFF;  // NOLINT
  tx_buffer_[2] = (addr >> 8) & 0xFF;   // NOLINT
  tx_buffer_[3] = addr & 0xFF;          // NOLINT

  std::span<uint8_t> tx_span(tx_buffer_.data(), 4);
  ret = spi_.Write(cs_, false, tx_span, kDfDelay);

  if (!WaitForTimeout(W25Q80_ERASE_BLOCK_TIMEOUT + 1)) {
    return StatusT::kError;
  }

  WriteEnable(false);

  return ret;
}

/**
 * @brief Erase all memory
 *
 * @return true if successful, otherwise a different status is returned
 */
StatusT MemW25Q80::EraseChip() {
  StatusT ret{};

  if (!WaitForTimeout(W25Q80_ERASE_CHIP_TIMEOUT + 1)) {
    return StatusT::kError;
  }

  WriteEnable(true);

  tx_buffer_[0] = W25Q80_INSTRUCTION_ERASE_CHIP;

  std::span<uint8_t> tx_span(tx_buffer_.data(), 1);
  ret = spi_.Write(cs_, false, tx_span, kDfDelay);

  WriteEnable(false);

  if (!WaitForTimeout(W25Q80_ERASE_CHIP_TIMEOUT + 1)) {
    return StatusT::kError;
  }

  return ret;
}

/**
 * @brief Read memory ID
 *
 * @param rx_buffer Pointer to data buffer
 * @return true if successful, otherwise a different status is returned
 */
StatusT MemW25Q80::GetId(uint16_t &mem_id) {
  StatusT ret{};

  if (!WaitForTimeout(W25Q80_ERASE_CHIP_TIMEOUT + 1)) {
    return StatusT::kError;
  }
  // ret = isBusy(is_busy);
  // if(ret.success)
  // {
  //   if(is_busy)
  //   {
  //     return STATUS_DRV_ERR_BUSY;
  //   }
  // }else
  // {
  //   return ret;
  // }

  std::memset(tx_buffer_.data(), 0, 6);  // NOLINT
  tx_buffer_[0] = W25Q80_INSTRUCTION_READ_MANUFACTURER_ID;
  std::span<uint8_t> tx_span(tx_buffer_.data(), 6);  // NOLINT
  std::span<uint8_t> rx_span(rx_buffer_.data(), 6);  // NOLINT
  ret = spi_.Transfer(cs_, false, tx_span, rx_span, kDfDelay);
  mem_id = (rx_buffer_[4] << 8) | rx_buffer_[5];  // NOLINT

  return ret;
}

/**
 * @brief Get the memory organization data
 *
 * @return
 */
DevMemSizesT MemW25Q80::GetOrganization() {
  return kW25Q80_SIZES;
}

/**
 * @brief Allow write in memory
 *
 * @param status Enable or disable writing (true/false)
 */
void MemW25Q80::WriteEnable(bool status) {
  tx_buffer_[0] = W25Q80_INSTRUCTION_WRITE_ENABLE;
  tx_buffer_[1] = W25Q80_INSTRUCTION_WRITE_DISABLE;

  if (status) {
    std::span<uint8_t> tx_span((tx_buffer_).data(), 1);
    spi_.Write(cs_, false, tx_span, kDfDelay);
  } else {
    std::span<uint8_t> tx_span(&tx_buffer_[1], 1);
    spi_.Write(cs_, false, tx_span, kDfDelay);
  }
}

/**
 * @brief BUSY memory status
 *
 * @return true if memory is BUSY
 * @return false if memory is not BUSY
 */
bool MemW25Q80::IsBusy() {
  tx_buffer_[0] = W25Q80_INSTRUCTION_READ_STATUS_REGISTER;
  rx_buffer_[1] = 0xFF;  // NOLINT

  std::span<uint8_t> tx_span(tx_buffer_.data(), 2);
  std::span<uint8_t> rx_span(rx_buffer_.data(), 2);
  spi_.Transfer(cs_, false, tx_span, rx_span, kDfDelay);

  // return BUSY status (true for writing in memory, false if idle)
  return (rx_buffer_[1] & 1) == 1;
}

/**
 * @brief BUSY memory status
 *
 * @param is_busy
 * @return StatusT
 */
StatusT MemW25Q80::IsBusy(bool &is_busy) {
  StatusT ret{};
  tx_buffer_[0] = W25Q80_INSTRUCTION_READ_STATUS_REGISTER;
  rx_buffer_[1] = 0xFF;  // NOLINT

  std::span<uint8_t> tx_span(tx_buffer_.data(), 2);
  std::span<uint8_t> rx_span(rx_buffer_.data(), 2);
  ret = spi_.Transfer(cs_, false, tx_span, rx_span, kDfDelay);

  // return BUSY status (true for writing in memory, false if idle)
  is_busy = (rx_buffer_[1] & 1) == 1;
  return ret;
}

/**
 * @brief Wait for memory not busy or timeout to occur
 *
 * @param time Desired timeout in milliseconds
 * @return true if memory is not busy
 * @return false if timeout occurred
 */
bool MemW25Q80::WaitForTimeout(uint16_t time) {
  uint16_t counter{};
  while (IsBusy()) {
    // Timeout counter
    timer_.Delay(10, sfw::hal_interface::TimeUnit::kMilliseconds);  // NOLINT
    counter += 10;                                                  // NOLINT
    if (counter >= time) {
      return false;
    }
  }
  return true;
}
