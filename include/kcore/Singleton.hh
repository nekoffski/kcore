#pragma once

#include <kcore/Concepts.hh>
#include <kcore/Core.hh>
#include <kcore/Log.hh>
#include <mutex>

namespace kc {

template <typename T>
class Singleton : public virtual NonMovable, public virtual NonCopyable {
   public:
    [[nodiscard]] static T& get() {
        static T* instance = new T{};  // let it leak
        return *instance;
    }

    static void noop() {}
};

template <typename T>
class UniqueInstance : public virtual NonMovable, public virtual NonCopyable {
   public:
    explicit UniqueInstance() {
        std::lock_guard lock{s_mutex};
        log::expect(not s_instance, "Instance already exists");
        s_instance = static_cast<T*>(this);
    }

   protected:
    ~UniqueInstance() {
        std::lock_guard lock{s_mutex};
        s_instance = nullptr;
    }

    static T& instance() {
        std::lock_guard lock{s_mutex};
        log::expect(s_instance, "Instance does not exist");
        return *s_instance;
    }

   private:
    inline static std::mutex s_mutex;
    inline static T* s_instance = nullptr;
};

}  // namespace kc
