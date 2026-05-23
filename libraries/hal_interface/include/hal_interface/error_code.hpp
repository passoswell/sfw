// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_INTERFACE_ERROR_CODE_HPP
#define HAL_INTERFACE_ERROR_CODE_HPP

#include <cstdint>

namespace sfw::hal_interface {

/**
 * @brief Standardized error codes for HAL interface methods.
 *
 * These error codes are used by HAL interface methods to indicate the
 * result of an operation. They provide a consistent way to report
 * success, failure, and specific error conditions across different
 * HAL implementations.
 */
enum class ErrorCode : uint32_t {
  kOk = 0,           ///< Operation completed successfully.
  kInvalidArgument,  ///< Invalid argument provided to the method.
  kOutOfRange,       ///< Argument or value is out of valid range.
  kTimeout,          ///< Operation did not complete within timeout.
  kError,            ///< General error or operation failed.
  kNotSupported,     ///< Operation is not supported by this implementation.
};

}  // namespace sfw::hal_interface

#endif  // HAL_INTERFACE_ERROR_CODE_HPP
