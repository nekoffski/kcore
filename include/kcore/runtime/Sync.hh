#pragma once

#include <future>
#include <kcore/Time.hh>

namespace kc {

template <typename T>
bool futureReady(std::future<T>& future) {
    return future.wait_for(0s) == std::future_status::ready;
}

}  // namespace kc