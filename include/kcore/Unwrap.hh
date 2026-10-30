#pragma once

#include <kcore/Log.hh>

namespace kc {

template <typename T>
T unwrap(Result<T>&& result) {
    log::expect(result);
    return std::move(*result);
}

}  // namespace kc
