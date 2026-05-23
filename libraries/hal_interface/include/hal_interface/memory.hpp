// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_INTERFACE_MEMORY_HPP
#define HAL_INTERFACE_MEMORY_HPP

#include <cstdint>
#include <span>

#include "hal_interface/error_code.hpp"

namespace sfw::hal_interface {

/**
 * @brief Describes the geometry and timing characteristics of a memory device.
 */
struct MemoryMetadata {
  uint64_t base_address;  ///< First addressable byte of the memory region.
  uint64_t size_bytes;    ///< Total capacity of the memory region in bytes.
  uint32_t write_unit;  ///< Smallest number of bytes that can be written in one
                        ///< operation.
  uint32_t read_block_size;   ///< Maximum number of bytes that can be read in
                              ///< one instruction.
  uint32_t write_block_size;  ///< Maximum number of bytes that can be written
                              ///< in one instruction.
  uint32_t erase_block_size;  ///< Size of one erasable sector in bytes.
  uint32_t
      write_alignment;  ///< Required byte alignment for write start addresses.
  uint32_t erase_block_alignment;  ///< Required byte alignment for erase start
                                   ///< addresses.
  bool requires_erase_before_program;  ///< True if a sector must be erased
                                       ///< before it can be written again.
  uint32_t maximum_write_time_us;      ///< Worst-case write time for one write
                                       ///< unit, in microseconds.
  uint32_t maximum_sector_erase_time_ms;  ///< Worst-case time to erase one
                                          ///< sector, in milliseconds.
  uint32_t maximum_memory_erase_time_ms;  ///< Worst-case time for a full-chip
                                          ///< erase, in milliseconds.
  uint32_t erase_cycle_limit;  ///< Maximum number of erase cycles per sector (0
                               ///< = unlimited).
};

/**
 * @brief Abstract interface for non-volatile and volatile memory devices.
 *
 * Supports random-access read, page/word write, and block or full-chip erase
 * operations. Callers should consult GetMetadata() to understand alignment
 * requirements, erase granularity, and whether an erase is required before
 * programming.
 * All the methods on this interface are synchronous / blocking. This means
 * they only return if the operation is finished or if an error occurred.
 *
 * Typical usage:
 * 1. Call Initialize() once to configure the memory peripheral and underlying
 * buses.
 * 2. Call GetMetadata() to retrieve memory info and timing requirements.
 * 3. Call EraseBlock() or EraseAllMemory() if the memory requires erase before
 * program.
 * 4. Call Write() to program data into the erased regions.
 * 5. Call Read() to retrieve data from any valid memory address.
 * 6. Call Deinitialize() when access is no longer needed.
 */
class Memory {
 public:
  Memory() = default;
  Memory(const Memory&) = default;
  Memory& operator=(const Memory&) = default;
  Memory(Memory&&) = default;
  Memory& operator=(Memory&&) = default;

  /**
   * @brief Destructor
   */
  virtual ~Memory() = default;

  /**
   * @brief Initializes the memory peripheral.
   *
   * Prepares the underlying hardware (e.g. SPI/I2C bus, GPIO chip-select,
   * internal state machine) for use. Must be called before any other method.
   *
   * @retval ErrorCode::kOk    Initialization succeeded.
   * @retval ErrorCode::kError Initialization failed due to a hardware fault.
   */
  virtual ErrorCode Initialize() = 0;

  /**
   * @brief Deinitializes the memory peripheral.
   *
   * Releases hardware resources acquired during Initialize(). After this call,
   * all operations except Initialize() and IsInitialized() must fail.
   * Registers are deinitialized and peripherals can be put in low power or
   * off mode.
   *
   * @retval ErrorCode::kOk Peripheral deinitialized successfully.
   * @retval ErrorCode::kError Deinitialization failed due to hardware state.
   */
  virtual ErrorCode Deinitialize() = 0;

  /**
   * @brief Returns whether the peripheral has been successfully initialized.
   *
   * @retval true  Initialize() has been called and succeeded.
   * @retval false The peripheral is not yet initialized.
   */
  virtual bool IsInitialized() = 0;

  /**
   * @brief Reads bytes from the memory starting at @p start_address.
   *
   * Reads @p buffer.size() consecutive bytes beginning at @p start_address
   * into @p buffer. The read must not cross the boundary of the memory region.
   * If @p buffer.size() is greater than read_block_size in MemoryMetadata, the
   * implementation must split the read into multiple instructions in a
   * transparent way.
   *
   * @param[in]  start_address Byte address within the memory to begin reading
   * from.
   * @param[out] buffer        Destination span that receives the read bytes.
   * @param[in]  timeout_ms    Maximum time to wait for the read to complete, in
   * milliseconds.
   * @retval ErrorCode::kOk      All bytes were read successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   * @retval ErrorCode::kError   An address out-of-range or communication error
   * occurred.
   */
  virtual ErrorCode Read(uint64_t start_address, std::span<uint8_t> buffer,
                         uint32_t timeout_ms) = 0;

  /**
   * @brief Writes bytes from @p buffer to the memory starting at
   * @p start_address.
   *
   * Writes @p buffer.size() consecutive bytes to the memory. The caller must
   * ensure the target region has been erased first if
   * MemoryMetadata::requires_erase_before_program is true.
   * If @p buffer.size() is greater than MemoryMetadata::write_block_size, or if
   * @p start_address and @p buffer.size() are not aligned with
   * MemoryMetadata::write_alignment and MemoryMetadata::write_block_size,
   * respectively, the implementation must split the write into multiple
   * instructions in a transparent way.
   *
   * @param[in] start_address Byte address within the memory to begin writing
   * to.
   * @param[in] buffer        Source span containing the bytes to write.
   * @param[in] timeout_ms    Maximum time to wait for the write to complete, in
   * milliseconds.
   * @retval ErrorCode::kOk      All bytes were written successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   * @retval ErrorCode::kError   An alignment violation, out-of-range address,
   * or communication error occurred.
   */
  virtual ErrorCode Write(uint64_t start_address,
                          std::span<const uint8_t> buffer,
                          uint32_t timeout_ms) = 0;

  /**
   * @brief Erases the sector that contains @p address_within_sector.
   *
   * The address does not need to be sector-aligned; the implementation
   * determines which sector the address belongs to and erases that sector in
   * its entirety. The value a memory position will hold after an erase is
   * dependent on the device technology. If the device technology does not
   * limit which value will be held after an erase, the implementation must
   * default to 0xFF.
   *
   * @param[in] address_within_sector Any byte address that falls within the
   * target sector.
   * @param[in] timeout_ms            Maximum time to wait for the erase, in
   * milliseconds.
   * @retval ErrorCode::kOk      The sector was erased successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   * @retval ErrorCode::kError   A hardware or communication error occurred.
   */
  virtual ErrorCode EraseBlock(uint64_t address_within_sector,
                               uint32_t timeout_ms) = 0;

  /**
   * @brief Erases the entire memory device.
   *
   * After a successful erase, all bytes read as the erased value. The value a
   * memory position will hold after an erase is dependent on the device
   * technology. If the device technology does not limit which value will be
   * held after an erase, the implementation must default to 0xFF.
   * This operation may take significantly longer than erasing individual
   * sectors; consult MemoryMetadata::maximum_memory_erase_time_ms.
   *
   * @param[in] timeout_ms Maximum time to wait for the full-chip erase, in
   * milliseconds.
   * @retval ErrorCode::kOk      The memory was erased successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   * @retval ErrorCode::kError   A hardware or communication error occurred.
   */
  virtual ErrorCode EraseAllMemory(uint32_t timeout_ms) = 0;

  /**
   * @brief Returns the geometry and timing metadata for this memory device.
   *
   * The returned structure is valid for the lifetime of the object and does
   * not change between calls.
   *
   * @retval MemoryMetadata A structure describing the memory's capacity,
   *                        alignment requirements, and timing constraints.
   */
  [[nodiscard]] virtual MemoryMetadata GetMetadata() const = 0;
};

}  // namespace sfw::hal_interface

#endif  // HAL_INTERFACE_MEMORY_HPP
