#include <future>
#include <kcore/runtime/ThreadSafeQueue.hh>
#include <thread>

#include "TestSupport.hh"

namespace kc::test {
using namespace testing;
using namespace std::chrono_literals;

using namespace testing;
using namespace std::chrono_literals;

TEST(ThreadSafeQueue, EmptyQueueAndFifoWithMoveOnlyValues) {
    ThreadSafeQueue<std::unique_ptr<int>> queue;
    EXPECT_FALSE(queue.closed());
    EXPECT_FALSE(queue.tryPop());
    EXPECT_TRUE(queue.push(std::make_unique<int>(1)));
    EXPECT_TRUE(queue.push(std::make_unique<int>(2)));
    auto first = queue.tryPop();
    ASSERT_TRUE(first);
    EXPECT_EQ(**first, 1);
    auto second = queue.pop();
    ASSERT_TRUE(second);
    EXPECT_EQ(**second, 2);
    EXPECT_FALSE(queue.tryPop());
}

TEST(ThreadSafeQueue, ClosingRejectsPushesButAllowsDraining) {
    ThreadSafeQueue<int> queue;
    EXPECT_TRUE(queue.push(1));
    queue.close();
    queue.close();
    EXPECT_TRUE(queue.closed());
    EXPECT_FALSE(queue.push(2));
    EXPECT_EQ(queue.pop(), 1);
    EXPECT_FALSE(queue.pop());
    EXPECT_FALSE(queue.tryPop());
}

TEST(ThreadSafeQueue, PushWakesConsumerAndCloseWakesAllRemainingConsumers) {
    ThreadSafeQueue<int> queue;
    auto first = std::async(std::launch::async, [&] { return queue.pop(); });
    EXPECT_TRUE(queue.push(42));
    EXPECT_EQ(first.wait_for(1s), std::future_status::ready);
    queue.close();
    EXPECT_EQ(first.get(), 42);

    ThreadSafeQueue<int> empty;
    auto second = std::async(std::launch::async, [&] { return empty.pop(); });
    auto third = std::async(std::launch::async, [&] { return empty.pop(); });
    empty.close();
    EXPECT_EQ(second.wait_for(1s), std::future_status::ready);
    EXPECT_EQ(third.wait_for(1s), std::future_status::ready);
    EXPECT_FALSE(second.get());
    EXPECT_FALSE(third.get());
}

TEST(ThreadSafeQueue, ConcurrentProducersDeliverEachValueOnce) {
    ThreadSafeQueue<int> queue;
    std::thread first{[&] {
        for (int i = 0; i < 100; ++i) {
            EXPECT_TRUE(queue.push(int{i}));
        }
    }};
    std::thread second{[&] {
        for (int i = 100; i < 200; ++i) {
            EXPECT_TRUE(queue.push(int{i}));
        }
    }};
    first.join();
    second.join();
    queue.close();
    std::vector<int> values;
    while (auto value = queue.pop()) {
        values.push_back(*value);
    }
    std::sort(values.begin(), values.end());
    ASSERT_EQ(values.size(), 200);
    for (int i = 0; i < 200; ++i) {
        EXPECT_EQ(values[i], i);
    }
}

TEST(ThreadSafeQueue, TimedPopDistinguishesTimeoutFromClosedAndDrainsValues) {
    ThreadSafeQueue<std::unique_ptr<int>> queue;
    auto timeout = queue.popFor(1ms);
    ASSERT_FALSE(timeout);
    EXPECT_EQ(timeout.error().code(), ErrorCode::timeout);

    EXPECT_TRUE(queue.push(std::make_unique<int>(42)));
    queue.close();
    auto value = queue.popFor(0ms);
    ASSERT_TRUE(value);
    EXPECT_EQ(**value, 42);
    auto closed = queue.popFor(0ms);
    ASSERT_FALSE(closed);
    EXPECT_EQ(closed.error().code(), ErrorCode::queueClosed);
}

TEST(ThreadSafeQueue, TimedPopWakesOnPushAndClose) {
    ThreadSafeQueue<int> queue;
    auto value =
        std::async(std::launch::async, [&] { return queue.popFor(1s); });
    EXPECT_TRUE(queue.push(42));
    EXPECT_EQ(value.wait_for(1s), std::future_status::ready);
    auto result = value.get();
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, 42);

    auto closed =
        std::async(std::launch::async, [&] { return queue.popFor(1s); });
    queue.close();
    EXPECT_EQ(closed.wait_for(1s), std::future_status::ready);
    auto error = closed.get();
    ASSERT_FALSE(error);
    EXPECT_EQ(error.error().code(), ErrorCode::queueClosed);
}

}  // namespace kc::test
