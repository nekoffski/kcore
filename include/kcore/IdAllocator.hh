#pragma once

#include <deque>
#include <kcore/Concepts.hh>
#include <kcore/Core.hh>
#include <mutex>
#include <vector>

namespace kc {

class IdAllocator : public NonCopyable, public NonMovable {
   public:
    u32 acquire();
    void release(u32 id);

   private:
    std::deque<u32> m_freeIds;
    std::vector<bool> m_inUse;
    std::mutex m_mutex;
};

class TypeIdAllocator : public NonCopyable, public NonMovable {
   public:
    template <typename T>
    u32 acquire() {
        return allocator<T>().acquire();
    }

    template <typename T>
    void release(u32 id) {
        allocator<T>().release(id);
    }

   private:
    template <typename T>
    static IdAllocator& allocator() {
        static IdAllocator value;
        return value;
    }
};

}  // namespace kc
