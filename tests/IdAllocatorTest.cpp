#include <array>
#include <barrier>
#include <kcore/IdAllocator.hh>
#include <set>
#include <thread>

#include "TestSupport.hh"

namespace kc::test {
using namespace testing;

TEST(IdAllocator, AllocatesSequentialIdsAndReusesReleasedIdsInOrder) {
    IdAllocator pool;
    EXPECT_EQ(pool.acquire(), 0);
    EXPECT_EQ(pool.acquire(), 1);
    EXPECT_EQ(pool.acquire(), 2);
    pool.release(1);
    pool.release(0);
    EXPECT_EQ(pool.acquire(), 1);
    EXPECT_EQ(pool.acquire(), 0);
    EXPECT_EQ(pool.acquire(), 3);
}

TEST(TypeIdAllocator, SharesPoolsAcrossInstancesButSeparatesTypes) {
    struct First {};
    struct Second {};
    TypeIdAllocator first, second;
    auto id = first.acquire<First>();
    EXPECT_EQ(second.acquire<First>(), id + 1);
    EXPECT_EQ(first.acquire<Second>(), 0);
    second.release<First>(id);
    EXPECT_EQ(first.acquire<First>(), id);
}

TEST(IdAllocatorDeathTest, RejectsForeignAndRepeatedReleases) {
    IdAllocator allocator;
    EXPECT_DEATH(allocator.release(0), "");
    const auto id = allocator.acquire();
    allocator.release(id);
    EXPECT_DEATH(allocator.release(id), "");
    EXPECT_EQ(allocator.acquire(), id);
}

TEST(TypeIdAllocator, ConcurrentFirstUseProducesUniqueIdsPerType) {
    struct Tag {};
    struct Other {};
    std::array<std::vector<u32>, 4> ids;
    std::barrier start{4};
    std::vector<std::thread> threads;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        threads.emplace_back([&, i] {
            TypeIdAllocator allocator;
            start.arrive_and_wait();
            for (int j = 0; j < 500; ++j) {
                ids[i].push_back(allocator.acquire<Tag>());
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    std::set<u32> unique;
    for (const auto& values : ids) {
        unique.insert(values.begin(), values.end());
    }
    EXPECT_EQ(unique.size(), 2000);
    TypeIdAllocator allocator;
    EXPECT_EQ(allocator.acquire<Other>(), 0);
    for (auto id : unique) {
        allocator.release<Tag>(id);
    }
    EXPECT_EQ(allocator.acquire<Tag>(), *unique.begin());
}

}  // namespace kc::test
