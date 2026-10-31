#pragma once

#include <cstdlib>
#include <kcore/Concepts.hh>
#include <kcore/Core.hh>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace kc {

template <typename T = std::string>
    requires OneOf<T, std::string, u64, i64, f32, f64>
inline Opt<T> getEnv(const std::string& env) {
    if (const auto value = std::getenv(env.c_str()); value) {
        try {
            if constexpr (std::is_same_v<T, std::string>) {
                return std::string{value};
            } else {
                const std::string_view input{value};
                constexpr auto whitespace = " \t\n\r\f\v";
                const auto first = input.find_first_not_of(whitespace);
                if (first == std::string_view::npos) {
                    return {};
                }
                const std::string text{input.substr(
                    first, input.find_last_not_of(whitespace) - first + 1
                )};
                std::size_t consumed = 0;
                T result;
                if constexpr (std::is_same_v<T, u64>) {
                    if (text.front() == '-') {
                        return {};
                    }
                    result = std::stoull(text, &consumed);
                } else if constexpr (std::is_same_v<T, i64>) {
                    result = std::stoll(text, &consumed);
                } else if constexpr (std::is_same_v<T, f32>) {
                    result = std::stof(text, &consumed);
                } else {
                    result = std::stod(text, &consumed);
                }
                if (consumed == text.size()) {
                    return result;
                }
            }
        } catch ([[maybe_unused]] const std::logic_error&) {
            return {};
        }
    }
    return {};
}

}  // namespace kc
