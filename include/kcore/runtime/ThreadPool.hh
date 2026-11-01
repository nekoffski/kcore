#pragma once

#include <functional>
#include <future>
#include <kcore/Concepts.hh>
#include <kcore/Core.hh>
#include <kcore/Functional.hh>
#include <kcore/runtime/ThreadSafeQueue.hh>
#include <memory>
#include <type_traits>

namespace kc {

class ThreadPool : public NonCopyable, public NonMovable {
    using Task = MoveOnlyFunction<void()>;

   public:
    explicit ThreadPool(u64 threadCount = std::thread::hardware_concurrency());
    ~ThreadPool();

    void close();

    template <typename Callback>
    auto submit(Callback&& callback)
        -> Result<std::future<std::invoke_result_t<Callback&&>>> {
        std::packaged_task<std::invoke_result_t<Callback&&>()> task(
            std::forward<Callback>(callback)
        );
        auto future = task.get_future();
        auto wrapper = [task = std::move(task)]() mutable -> void { task(); };

        if (auto r = enqueue(std::move(wrapper)); not r) {
            return Error::unexpected(r.error());
        }

        return future;
    }

   private:
    Result<void> enqueue(Task task);

    void worker(u64 id);
    bool dispatch(u64 id);

    std::vector<std::thread> m_workers;
    std::vector<std::unique_ptr<ThreadSafeQueue<Task>>> m_queues;

    std::atomic<u64> m_nextId{0u};
    std::atomic_flag m_isClosed{false};
};

}  // namespace kc
