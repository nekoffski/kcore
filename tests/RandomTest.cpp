#include <array>
#include <barrier>
#include <kcore/Random.hh>
#include <set>
#include <thread>

#include "TestSupport.hh"

namespace kc::test {
using namespace testing;

TEST(Random, GeneratesValidDistinctUuidsAndStableObjectIdentity) {
    auto& engine = RandomEngine::get();
    auto first = engine.uuid();
    auto second = engine.uuid();
    EXPECT_TRUE(uuids::uuid::is_valid_uuid(first));
    EXPECT_TRUE(uuids::uuid::is_valid_uuid(second));
    EXPECT_NE(first, second);
    EXPECT_TRUE(uuids::uuid::is_valid_uuid(Tag<Str>::fromUuid().get()));
    WithUuid object;
    EXPECT_TRUE(uuids::uuid::is_valid_uuid(object.uuid()));
    EXPECT_EQ(&object.uuid(), &object.uuid());
    EXPECT_EQ(&engine, &RandomEngine::get());
}

TEST(Random, ValidatesUuidStrings) {
    EXPECT_TRUE(isValidUuid(RandomEngine::get().uuid()));
    EXPECT_TRUE(isValidUuid("12345678-1234-4234-8234-123456789abc"));
    for (const auto* value :
         {"", "not-a-uuid", "12345678-1234-4234-8234-123456789ab",
          "12345678-1234-4234-8234-123456789abz"}) {
        EXPECT_FALSE(isValidUuid(value)) << value;
    }
}

TEST(Random, ConcurrentUuidGenerationRemainsValidAndDistinct) {
    std::array<std::vector<Str>, 4> values;
    std::barrier start{4};
    std::vector<std::thread> threads;
    for (std::size_t i = 0; i < values.size(); ++i) {
        threads.emplace_back([&, i] {
            start.arrive_and_wait();
            for (int j = 0; j < 250; ++j) {
                values[i].push_back(RandomEngine::get().uuid());
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    std::set<Str> unique;
    for (const auto& strings : values) {
        for (const auto& value : strings) {
            EXPECT_TRUE(uuids::uuid::is_valid_uuid(value));
            unique.insert(value);
        }
    }
    EXPECT_EQ(unique.size(), 1000);
}

}  // namespace kc::test
