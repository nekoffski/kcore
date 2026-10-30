#pragma once

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/std.h>

#include <expected>
#include <kcore/ErrorCode.hh>
#include <optional>
#include <string>

namespace kc {

class Error {
   public:
    explicit Error(
        std::error_code code = ErrorCode::noError,
        const std::string& message = "No details provided"
    );

    template <typename... Args>
    explicit Error(std::error_code code, const std::string& fmt, Args&&... args)
        : m_code(code),
          m_message(
              fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...)
          ) {}

    std::error_code code() const;
    const std::string& message() const;

    bool valid() const { return bool(m_code); }

    template <typename... Args>
    static std::unexpected<Error> unexpected(
        std::error_code code, const std::string& fmt, Args&&... args
    ) {
        return std::unexpected{Error{code, fmt, std::forward<Args>(args)...}};
    }

    static std::unexpected<Error> unexpected(const Error& error);

    static std::optional<Error> empty();

    void reset();

   private:
    std::error_code m_code;
    std::string m_message;
};

template <typename T>
using Result = std::expected<T, Error>;

}  // namespace kc

template <>
class fmt::formatter<kc::Error> {
   public:
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename Context>
    constexpr auto format(const kc::Error& error, Context& ctx) const {
        return fmt::format_to(
            ctx.out(), "code: {}, details: '{}'", error.code(), error.message()
        );
    }
};
