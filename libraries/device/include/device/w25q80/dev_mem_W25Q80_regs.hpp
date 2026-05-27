/**
 * @file dev_mem_W25Q80_regs.hpp
 * @author user (contact@email.com)
 * @brief Insert a brief description here
 * @date 2023-12-23
 *
 * @copyright Copyright (c) 2023
 *
 * @warning This is an old and deprecated device driver. For new designs,
 * consider using the JEDEC SPI flash adapter (device/jedec_flash) instead,
 * which supports a wider range of devices including the W25Q80 and provides a
 * more flexible and feature-rich interface.
 *
 */

#ifndef DEVICE_W25Q80_DEV_MEM_W25Q80_REGS_HPP
#define DEVICE_W25Q80_DEV_MEM_W25Q80_REGS_HPP

#include <cstdint>

/******************************** Memory sizes ********************************/

// Number of pages in the memory
#define W25Q80_NUMBEROF_PAGES 4096  // NOLINT

// Size of a page in the memory
// Maximum number of bytes that can be written in one instruction
#define W25Q80_PAGE_SIZE 256  // NOLINT

// Number of sectors (groups of 16 pages, 4KB)
#define W25Q80_SECTOR_SIZE 16  // NOLINT

// Number of blocks (groups of 16 sectors or 256 pages, 64KB)
#define W25Q80_BLOCK_SIZE 16  // NOLINT

/******************************* Memory timings *******************************/

// Maximum time it takes to write to a page in ms
#define W25Q80_WRITE_PAGE_TIMEOUT 3  // NOLINT

// Maximum time it takes to erase a sector in ms
#define W25Q80_ERASE_SECTOR_TIMEOUT 300  // NOLINT

// Maximum time it takes to erase a block in ms
#define W25Q80_ERASE_BLOCK_TIMEOUT 1000  // NOLINT

// Maximum time it takes to erase the chip in ms
#define W25Q80_ERASE_CHIP_TIMEOUT 6000  // NOLINT

/******************************* Memory commands ******************************/

#define W25Q80_INSTRUCTION_PAGE_PROGRAM 0x02           // NOLINT
#define W25Q80_INSTRUCTION_NORMAL_READ 0x03            // NOLINT
#define W25Q80_INSTRUCTION_WRITE_DISABLE 0x04          // NOLINT
#define W25Q80_INSTRUCTION_WRITE_ENABLE 0x06           // NOLINT
#define W25Q80_INSTRUCTION_WRITE_STATUS_REGISTER 0x01  // NOLINT
#define W25Q80_INSTRUCTION_READ_STATUS_REGISTER 0x05   // NOLINT
// Erase a entire sector (16 pages)
#define W25Q80_INSTRUCTION_ERASE_SECTOR 0x20  // NOLINT
// Erase the entire chip. Instruction could be 0x60
#define W25Q80_INSTRUCTION_ERASE_CHIP 0xC7  // NOLINT
// Erase a entire block (128 or 256 pages)
#define W25Q80_INSTRUCTION_ERASE_BLOCK 0xD8  // NOLINT
// Read chip identifier
#define W25Q80_INSTRUCTION_READ_MANUFACTURER_ID 0x90  // NOLINT

/**
 * @brief
 *
 */
struct DevMemSizesT {
  uint16_t number_of_pages;
  uint16_t page_size;
  uint16_t sector_size;
  uint16_t block_size;
};

static constexpr DevMemSizesT kW25Q80_SIZES = {
    .number_of_pages = W25Q80_NUMBEROF_PAGES,
    .page_size = W25Q80_PAGE_SIZE,
    .sector_size = W25Q80_SECTOR_SIZE,
    .block_size = W25Q80_BLOCK_SIZE,
};

#endif  // DEVICE_W25Q80_DEV_MEM_W25Q80_REGS_HPP
