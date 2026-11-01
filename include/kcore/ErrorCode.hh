#pragma once

#include <system_error>
#include <type_traits>

namespace kc {

enum class ErrorCode {
    // Values are serialized; removing codes must not renumber the others.
    noError = 0,
    queueClosed = 3,
    timeout = 4,
    invalidConfig = 5,
    fileSystemError = 6,
    invalidArgument = 7,
    jsonParsingError = 8,
    fileNotFound = 9,
    threadPoolClosed = 10,
};

const std::error_category& errorCategory() noexcept;
std::error_code make_error_code(ErrorCode code) noexcept;

}  // namespace kc

template <>
struct std::is_error_code_enum<kc::ErrorCode> : std::true_type {};
