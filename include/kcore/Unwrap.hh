#pragma once

#include <kcore/Log.hh>

namespace kc {

template <typename T>
T unwrap(Result<T>&& result) {
    log::expect(result);
    if constexpr (not std::is_void_v<T>) {
        return std::move(*result);
    }
}

}  // namespace kc
