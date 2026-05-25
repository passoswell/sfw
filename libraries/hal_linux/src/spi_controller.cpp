// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "hal_linux/spi/spi_controller.hpp"

#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>

namespace sfw::hal_linux {

namespace {

void FillSpiTransfer(spi_ioc_transfer& transfer,
                     std::span<const uint8_t> tx_buffer,
                     std::span<uint8_t> rx_buffer, uint32_t clock_hz,
                     uint8_t bits_per_word) {
  std::memset(&transfer, 0, sizeof(transfer));
  transfer.tx_buf = std::bit_cast<decltype(transfer.tx_buf)>(tx_buffer.data());
  transfer.rx_buf = std::bit_cast<decltype(transfer.rx_buf)>(rx_buffer.data());
  if (rx_buffer.empty()) {
    transfer.len = static_cast<decltype(transfer.len)>(tx_buffer.size());
  } else {
    transfer.len = static_cast<decltype(transfer.len)>(rx_buffer.size());
  }
  transfer.speed_hz = clock_hz;
  transfer.delay_usecs = 0;
  transfer.bits_per_word = bits_per_word;
  transfer.cs_change = 0;
}

}  // namespace

SpiController::SpiController(std::string device, uint32_t mode,
                             uint32_t clock_hz, uint8_t bits_per_word)
  : device_(std::move(device))
  , mode_(mode)
  , bits_per_word_(bits_per_word)
  , clock_hz_(clock_hz) {
}

SpiController::~SpiController() {
  (void)CloseBus();
}

hal_interface::ErrorCode SpiController::Initialize() {
  if (initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  fd_ = ::open(device_.c_str(), O_RDWR);
  if (fd_ < 0) {
    return hal_interface::ErrorCode::kError;
  }

  if (ApplyConfiguration() != hal_interface::ErrorCode::kOk) {
    (void)Deinitialize();
    return hal_interface::ErrorCode::kError;
  }

  initialized_ = true;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode SpiController::Deinitialize() {
  if (!initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  return CloseBus();
}

bool SpiController::IsInitialized() {
  return initialized_;
}

hal_interface::ErrorCode SpiController::CloseBus() {
  if (fd_ < 0) {
    initialized_ = false;
    return hal_interface::ErrorCode::kOk;
  }

  if (::close(fd_) != 0) {
    return hal_interface::ErrorCode::kError;
  }

  fd_ = -1;
  initialized_ = false;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode SpiController::Read(
    hal_interface::DigitalOutput& cs_pin, bool active_state,
    std::span<uint8_t> buffer, uint32_t timeout_ms) {
  (void)timeout_ms;  // Placeholder until timeout handling is implemented
  if (!initialized_ || timeout_ms == 0U) {
    return hal_interface::ErrorCode::kError;
  }

  spi_ioc_transfer transfer{};
  FillSpiTransfer(transfer, std::span<const uint8_t>(), buffer, clock_hz_,
                  bits_per_word_);

  const hal_interface::ErrorCode CsAsserted =
      cs_pin.Write(active_state, timeout_ms);
  if (CsAsserted != hal_interface::ErrorCode::kOk) {
    return CsAsserted;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  const int Transferred = ::ioctl(fd_, SPI_IOC_MESSAGE(1), &transfer);

  const hal_interface::ErrorCode CsDeasserted =
      cs_pin.Write(!active_state, timeout_ms);
  if (CsDeasserted != hal_interface::ErrorCode::kOk) {
    return CsDeasserted;
  }

  if (Transferred != static_cast<int>(buffer.size())) {
    return hal_interface::ErrorCode::kError;
  }
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode SpiController::Write(
    hal_interface::DigitalOutput& cs_pin, bool active_state,
    std::span<const uint8_t> buffer, uint32_t timeout_ms) {
  (void)timeout_ms;  // Placeholder until timeout handling is implemented
  if (!initialized_ || timeout_ms == 0U) {
    return hal_interface::ErrorCode::kError;
  }

  spi_ioc_transfer transfer{};
  FillSpiTransfer(transfer, buffer, std::span<uint8_t>(), clock_hz_,
                  bits_per_word_);

  const hal_interface::ErrorCode CsAsserted =
      cs_pin.Write(active_state, timeout_ms);
  if (CsAsserted != hal_interface::ErrorCode::kOk) {
    return CsAsserted;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  const int Transferred = ::ioctl(fd_, SPI_IOC_MESSAGE(1), &transfer);

  const hal_interface::ErrorCode CsDeasserted =
      cs_pin.Write(!active_state, timeout_ms);
  if (CsDeasserted != hal_interface::ErrorCode::kOk) {
    return CsDeasserted;
  }

  if (Transferred != static_cast<int>(buffer.size())) {
    return hal_interface::ErrorCode::kError;
  }
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode SpiController::WriteThenRead(
    hal_interface::DigitalOutput& cs_pin, bool active_state,
    std::span<const uint8_t> write_buffer, std::span<uint8_t> read_buffer,
    uint32_t timeout_ms) {
  (void)timeout_ms;  // Placeholder until timeout handling is implemented
  if (!initialized_ || timeout_ms == 0U) {
    return hal_interface::ErrorCode::kError;
  }

  std::array<spi_ioc_transfer, 2U> transfers{};
  FillSpiTransfer(transfers.at(0U), write_buffer, std::span<uint8_t>(),
                  clock_hz_, bits_per_word_);
  FillSpiTransfer(transfers.at(1U), std::span<const uint8_t>(), read_buffer,
                  clock_hz_, bits_per_word_);

  const hal_interface::ErrorCode CsAsserted =
      cs_pin.Write(active_state, timeout_ms);
  if (CsAsserted != hal_interface::ErrorCode::kOk) {
    return CsAsserted;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  const int Transferred = ::ioctl(fd_, SPI_IOC_MESSAGE(2), transfers.data());

  const hal_interface::ErrorCode CsDeasserted =
      cs_pin.Write(!active_state, timeout_ms);
  if (CsDeasserted != hal_interface::ErrorCode::kOk) {
    return CsDeasserted;
  }

  if (Transferred < 1) {
    return hal_interface::ErrorCode::kError;
  }
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode SpiController::Transfer(
    hal_interface::DigitalOutput& cs_pin, bool active_state,
    std::span<const uint8_t> write_buffer, std::span<uint8_t> read_buffer,
    uint32_t timeout_ms) {
  (void)timeout_ms;  // Placeholder until timeout handling is implemented
  if (!initialized_ || timeout_ms == 0U) {
    return hal_interface::ErrorCode::kError;
  }

  if (write_buffer.size() != read_buffer.size()) {
    return hal_interface::ErrorCode::kError;
  }

  spi_ioc_transfer transfer{};
  FillSpiTransfer(transfer, write_buffer, read_buffer, clock_hz_,
                  bits_per_word_);

  const hal_interface::ErrorCode CsAsserted =
      cs_pin.Write(active_state, timeout_ms);
  if (CsAsserted != hal_interface::ErrorCode::kOk) {
    return CsAsserted;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  const int Transferred = ::ioctl(fd_, SPI_IOC_MESSAGE(1), &transfer);

  const hal_interface::ErrorCode CsDeasserted =
      cs_pin.Write(!active_state, timeout_ms);
  if (CsDeasserted != hal_interface::ErrorCode::kOk) {
    return CsDeasserted;
  }

  if (Transferred != static_cast<int>(write_buffer.size())) {
    return hal_interface::ErrorCode::kError;
  }
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode SpiController::ReadRegister(
    hal_interface::DigitalOutput& cs_pin, bool active_state,
    uint32_t register_address, uint8_t register_size_bytes,
    std::span<uint8_t> buffer, uint32_t timeout_ms) {
  (void)timeout_ms;  // Placeholder until timeout handling is implemented
  if (!IsRegSizeValid(register_size_bytes)) {
    return hal_interface::ErrorCode::kError;
  }

  const auto EncodedAddress =
      EncodeRegAddress(register_address, register_size_bytes);
  const std::span<const uint8_t> RegisterBytes(EncodedAddress.data(),
                                               register_size_bytes);

  return WriteThenRead(cs_pin, active_state, RegisterBytes, buffer, timeout_ms);
}

hal_interface::ErrorCode SpiController::WriteRegister(
    hal_interface::DigitalOutput& cs_pin, bool active_state,
    uint32_t register_address, uint8_t register_size_bytes,
    std::span<const uint8_t> buffer, uint32_t timeout_ms) {
  (void)timeout_ms;  // Placeholder until timeout handling is implemented
  if (!IsRegSizeValid(register_size_bytes)) {
    return hal_interface::ErrorCode::kError;
  }

  const auto EncodedAddress =
      EncodeRegAddress(register_address, register_size_bytes);
  const std::span<const uint8_t> RegisterBytes(EncodedAddress.data(),
                                               register_size_bytes);

  std::array<spi_ioc_transfer, 2U> transfers{};
  FillSpiTransfer(transfers.at(0U), RegisterBytes, std::span<uint8_t>(),
                  clock_hz_, bits_per_word_);
  FillSpiTransfer(transfers.at(1U), buffer, std::span<uint8_t>(), clock_hz_,
                  bits_per_word_);

  const hal_interface::ErrorCode CsAsserted =
      cs_pin.Write(active_state, timeout_ms);
  if (CsAsserted != hal_interface::ErrorCode::kOk) {
    return CsAsserted;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  const int Transferred = ::ioctl(fd_, SPI_IOC_MESSAGE(2), transfers.data());

  const hal_interface::ErrorCode CsDeasserted =
      cs_pin.Write(!active_state, timeout_ms);
  if (CsDeasserted != hal_interface::ErrorCode::kOk) {
    return CsDeasserted;
  }

  if (Transferred < 1) {
    return hal_interface::ErrorCode::kError;
  }
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode SpiController::ApplyConfiguration() const {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (::ioctl(fd_, SPI_IOC_WR_MODE32, &mode_) < 0) {
    const auto Mode8 = static_cast<uint8_t>(mode_);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    if (::ioctl(fd_, SPI_IOC_WR_MODE, &Mode8) < 0) {
      return hal_interface::ErrorCode::kError;
    }
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (::ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word_) < 0) {
    return hal_interface::ErrorCode::kError;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (::ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &clock_hz_) < 0) {
    return hal_interface::ErrorCode::kError;
  }

  return hal_interface::ErrorCode::kOk;
}

bool SpiController::IsRegSizeValid(uint8_t register_size_bytes) {
  return register_size_bytes >= hal_interface::SpiController::kMinRegAddressSize
         && register_size_bytes
                <= hal_interface::SpiController::kMaxRegAddressSize;
}

std::array<uint8_t, hal_interface::SpiController::kMaxRegAddressSize>
SpiController::EncodeRegAddress(uint32_t register_address,
                                uint8_t register_size_bytes) {
  std::array<uint8_t, hal_interface::SpiController::kMaxRegAddressSize>
      encoded{};
  for (uint8_t index = 0U; index < register_size_bytes; index++) {
    const auto Shift =
        static_cast<uint8_t>((register_size_bytes - 1U - index) * 8U);
    encoded.at(index) = static_cast<uint8_t>(register_address >> Shift);
  }
  return encoded;
}

}  // namespace sfw::hal_linux
