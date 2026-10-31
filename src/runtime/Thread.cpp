#include <functional>
#include <kcore/Log.hh>
#include <kcore/Profiler.hh>
#include <kcore/runtime/Thread.hh>

namespace kc {

Thread::Thread(const Str& ident) : m_ident(ident) {}

Thread::~Thread() = default;

void Thread::join() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void Thread::join(std::span<Thread*> threads) {
    for (auto& thread : threads) {
        thread->join();
    }
}

void Thread::sleepFor(std::chrono::milliseconds duration) {
    std::this_thread::sleep_for(duration);
}

void Thread::start() {
    log::expect(
        not m_thread.joinable(),
        "Join the previous thread before starting again"
    );
    m_thread = std::thread([&] { go(); });
}

void Thread::go() {
    KCORE_PROFILE_REGISTER_THREAD();
    log::info("{} - thread starting", m_ident);

    try {
        run();
    } catch (const std::exception& e) {
        log::panic("{} - standard exception in thread: {}", m_ident, e.what());
    } catch (...) {
        log::panic("{} - unknown exception in thread", m_ident);
    }
    log::info("{} - thread exiting", m_ident);
}

}  // namespace kc
