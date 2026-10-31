#include <kcore/IdAllocator.hh>
#include <kcore/Log.hh>
#include <limits>
#include <stdexcept>

namespace kc {

u32 IdAllocator::acquire() {
    std::lock_guard lock{m_mutex};
    if (not m_freeIds.empty()) {
        const auto id = m_freeIds.front();
        m_freeIds.pop_front();
        m_inUse[id] = true;
        return id;
    }
    if (m_inUse.size() > std::numeric_limits<u32>::max()) {
        throw std::overflow_error("ID allocator exhausted");
    }
    const auto id = static_cast<u32>(m_inUse.size());
    m_inUse.push_back(true);
    return id;
}

void IdAllocator::release(u32 id) {
    std::lock_guard lock{m_mutex};
    log::expect(
        id < m_inUse.size() and m_inUse[id],
        "Invalid or duplicate ID release: {}", id
    );
    m_freeIds.push_back(id);
    m_inUse[id] = false;
}

}  // namespace kc
