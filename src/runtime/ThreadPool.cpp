#include <kcore/runtime/ThreadPool.hh>

namespace kc {

ThreadPool::ThreadPool(u64 threadCount) {
    for (u64 i = 0; i < threadCount; ++i) {
        m_workers.emplace_back(i);
    }
}

ThreadPool::~ThreadPool() {
    for (auto& worker : m_workers) {
        worker.join();
    }
}

void ThreadPool::enqueue(Task task) {}

ThreadPool::Worker::Worker(u64 number)
    : Thread{fmt::format("ThreadPoolWorker_{}", number)} {}

}  // namespace kc