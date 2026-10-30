#pragma once

#include <deque>
#include <kcore/Concepts.hh>
#include <kcore/Core.hh>
#include <mutex>
#include <typeindex>
#include <unordered_map>

namespace kc {

class IdPool : public NonCopyable, public NonMovable {
   public:
    u32 acquire();
    void release(u32 id);

   private:
    u32 m_counter{0u};
    std::deque<u32> m_freeIds;
    std::mutex m_mutex;
};

class IdLake : public NonCopyable, public NonMovable {
   public:
    template <typename T>
    u32 acquire() {
        return s_pools[typeid(T)].acquire();
    }

    template <typename T>
    void release(u32 id) {
        s_pools[typeid(T)].release(id);
    }

   private:
    inline static std::unordered_map<std::type_index, IdPool> s_pools;
};

}  // namespace kc
