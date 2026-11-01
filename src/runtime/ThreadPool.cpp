#include <kcore/runtime/ThreadPool.hh>

namespace kc {

ThreadPool::ThreadPool(u64 threadCount) {
    log::expect(threadCount > 0, "Thread count must be positive");

    try {
        for (u64 i = 0; i < threadCount; ++i) {
            m_queues.emplace_back(std::make_unique<ThreadSafeQueue<Task>>());
        }
        for (u64 i = 0; i < threadCount; ++i) {
            m_workers.emplace_back([this, i]() { this->worker(i); });
        }
    } catch (...) {
        close();
        throw;
    }
}

ThreadPool::~ThreadPool() { close(); }

void ThreadPool::close() {
    if (m_isClosed.test_and_set()) {
        return;
    }

    for (auto& q : m_queues) {
        q->close();
    }
    for (auto& worker : m_workers) {
        worker.join();
    }
}

Result<void> ThreadPool::enqueue(Task task) {
    const auto index = m_nextId.fetch_add(1, std::memory_order_relaxed);
    auto& q = m_queues[index % m_queues.size()];

    if (not q->push(std::move(task))) {
        return Error::unexpected(
            ErrorCode::threadPoolClosed, "Thread pool is closed"
        );
    }

    return {};
}

void ThreadPool::worker(u64 id) {
    for (;;) {
        if (not dispatch(id)) {
            break;
        }
    }
}

bool ThreadPool::dispatch(u64 id) {
    // try to steal the task but start with own q
    for (u64 i = 0; i < m_queues.size(); ++i) {
        auto& q = m_queues[(id + i) % m_queues.size()];
        if (auto task = q->tryPop(); task) {
            std::invoke(std::move(task.value()));
            return true;
        }
    }

    // block on itself to avoid busy-wait
    if (auto task = m_queues[id]->pop(); task) {
        std::invoke(std::move(task.value()));
        return true;
    }

    return false;
}
}  // namespace kc