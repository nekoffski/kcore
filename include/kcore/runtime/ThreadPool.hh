#pragma once

#include <functional>
#include <future>
#include <kcore/Concepts.hh>
#include <kcore/Core.hh>
#include <kcore/Functional.hh>
#include <kcore/runtime/Thread.hh>
#include <type_traits>

namespace kc {

class ThreadPool : public NonCopyable, public NonMovable {
    using Task = MoveOnlyFunction<void()>;

    class TaskQueue : public NonCopyable {
       public:
       private:
    };

    class Worker : public Thread {
       public:
        explicit Worker(u64 number);

       private:
        void run() override {}
    };

   public:
    explicit ThreadPool(u64 threadCount = std::thread::hardware_concurrency());
    ~ThreadPool();

    template <typename Callback>
    auto submit(Callback&& callback)
        -> std::future<std::invoke_result_t<Callback&&>> {
        std::packaged_task<std::invoke_result_t<Callback&&>()> task(
            std::forward<Callback>(callback)
        );
        auto future = task.get_future();
        enqueue([task = std::move(task)]() { task(); });
        return future;
    }

   private:
    void enqueue(Task task);
    std::vector<Worker> m_workers;
};

}  // namespace kc
