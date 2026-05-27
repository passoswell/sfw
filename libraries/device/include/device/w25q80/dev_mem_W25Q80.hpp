/**
 * @file dev_mem_W25Q80.hpp
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

#ifndef DEVICE_W25Q80_DEV_MEM_W25Q80_HPP
#define DEVICE_W25Q80_DEV_MEM_W25Q80_HPP

#include <array>
#include <cstdint>
#include <cstring>

#include "dev_mem_W25Q80_regs.hpp"
#include "hal_interface/digital_output.hpp"
#include "hal_interface/software_timer.hpp"
#include "hal_interface/spi_controller.hpp"

/**
 * @brief Size of the device driver buffers
 */
#define W25Q80_BUFFER_SIZE W25Q80_PAGE_SIZE

class MemW25Q80 final {
 public:
  MemW25Q80(sfw::hal_interface::SpiController &spi,
            sfw::hal_interface::DigitalOutput &cs_pin,
            sfw::hal_interface::SoftwareTimer &timer);

  ~MemW25Q80() = default;
  MemW25Q80(const MemW25Q80 &) = delete;
  MemW25Q80 &operator=(const MemW25Q80 &) = delete;
  MemW25Q80(MemW25Q80 &&) = delete;
  MemW25Q80 &operator=(MemW25Q80 &&) = delete;

  sfw::hal_interface::ErrorCode Read(uint32_t addr, uint8_t *rx_buffer,
                                     uint16_t size);

  sfw::hal_interface::ErrorCode Write(uint32_t addr, uint8_t *tx_buffer,
                                      uint16_t size);

  sfw::hal_interface::ErrorCode ReadPage(uint32_t addr, uint8_t *rx_buffer);

  sfw::hal_interface::ErrorCode WritePage(uint32_t addr, uint8_t *tx_buffer);

  sfw::hal_interface::ErrorCode EraseSector(uint32_t addr);

  sfw::hal_interface::ErrorCode EraseBlock64(uint32_t addr);

  sfw::hal_interface::ErrorCode EraseChip();

  sfw::hal_interface::ErrorCode GetId(uint16_t &mem_id);

  [[nodiscard]] static DevMemSizesT GetOrganization();

 private:
  void WriteEnable(bool status);

  bool IsBusy();

  sfw::hal_interface::ErrorCode IsBusy(bool &is_busy);

  bool WaitForTimeout(uint16_t time);

  sfw::hal_interface::SpiController &spi_;
  sfw::hal_interface::DigitalOutput &cs_;
  sfw::hal_interface::SoftwareTimer &timer_;
  std::array<uint8_t, W25Q80_BUFFER_SIZE + 4> rx_buffer_{};
  std::array<uint8_t, W25Q80_BUFFER_SIZE + 4> tx_buffer_{};
};

#endif  // DEVICE_W25Q80_DEV_MEM_W25Q80_HPP
