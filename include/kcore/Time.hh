#pragma once

#include <fmt/format.h>

#include <chrono>
#include <format>
#include <kcore/Core.hh>

namespace kc {

using namespace std::chrono_literals;

inline Str formatTime(
    std::chrono::system_clock::time_point tp, std::string_view format
) {
    auto fmt = Str{"{:"} + Str{format} + "}";
    return std::vformat(fmt, std::make_format_args(tp));
}
}  // namespace kc
