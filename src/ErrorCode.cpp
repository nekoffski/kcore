#include <kcore/ErrorCode.hh>

namespace kc {
namespace {

class ErrorCategory final : public std::error_category {
   public:
    const char* name() const noexcept override { return "kcore"; }

    std::string message(int value) const override {
        switch (static_cast<ErrorCode>(value)) {
            case ErrorCode::noError:
                return "No error";
            case ErrorCode::queueClosed:
                return "Queue closed";
            case ErrorCode::timeout:
                return "Timeout";
            case ErrorCode::invalidConfig:
                return "Invalid configuration";
            case ErrorCode::fileSystemError:
                return "Filesystem error";
            case ErrorCode::invalidArgument:
                return "Invalid argument";
            case ErrorCode::jsonParsingError:
                return "JSON parsing error";
            case ErrorCode::fileNotFound:
                return "File not found";
        }
        return "Unknown kcore error";
    }
};

}  // namespace

const std::error_category& errorCategory() noexcept {
    static ErrorCategory category;
    return category;
}

std::error_code make_error_code(ErrorCode code) noexcept {
    return {static_cast<int>(code), errorCategory()};
}

}  // namespace kc
