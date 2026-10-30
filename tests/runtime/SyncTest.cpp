#include <kcore/runtime/Sync.hh>

#include "TestSupport.hh"

namespace kc::test {
using namespace testing;

TEST(Sync, FutureReadinessFollowsPromiseCompletion) {
    std::promise<int> promise;
    auto future = promise.get_future();
    EXPECT_FALSE(futureReady(future));
    promise.set_value(42);
    EXPECT_TRUE(futureReady(future));
    EXPECT_EQ(future.get(), 42);
}

}  // namespace kc::test
