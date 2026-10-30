#pragma once

#include <atomic>
#include <kcore/Concepts.hh>
#include <kcore/Core.hh>
#include <kcore/Error.hh>
#include <thread>
#include <vector>

namespace kc {

class Thread : public NonCopyable {
   public:
    explicit Thread(const Str& ident);
    virtual ~Thread();

    Thread(Thread&& other) noexcept;
    Thread& operator=(Thread&& other) noexcept;

    void start();
    void join();

    static void join(std::span<Thread*> threads);

    template <typename... Args>
        requires(std::derived_from<Args, Thread> && ...)
    static void join(Args&... threads) {
        (threads.join(), ...);
    }

   protected:
    void sleepFor(std::chrono::milliseconds duration);

   private:
    virtual void run() = 0;
    void go();

    Str m_ident;
    std::thread m_thread;
};

}  // namespace kc
