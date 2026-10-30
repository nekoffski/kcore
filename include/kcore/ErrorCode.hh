#pragma once

#include <system_error>
#include <type_traits>

namespace kc {

enum class ErrorCode {
    noError,
    internalError,
    queueError,
    queueClosed,
    timeout,
    invalidConfig,
    fileSystemError,
    invalidArgument,
    jsonParsingError,
    fileNotFound,
    eof,
    connectionFailure,
    transientNetworkError,
    bufferOverflow,
    serializationFailure,
    notImplemented,
};

const std::error_category& errorCategory() noexcept;
std::error_code make_error_code(ErrorCode code) noexcept;

}  // namespace kc

template <>
struct std::is_error_code_enum<kc::ErrorCode> : std::true_type {};
