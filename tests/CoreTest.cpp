#include <kcore/Concepts.hh>
#include <kcore/Core.hh>
#include <stdexcept>

#include "TestSupport.hh"

struct ThrowingHashKey {
    bool operator==(const ThrowingHashKey&) const = default;
};
template <>
struct std::hash<ThrowingHashKey> {
    std::size_t operator()(const ThrowingHashKey&) const {
        throw std::runtime_error("hash failure");
    }
};

namespace kc::test {
using namespace testing;

static_assert(OneOf<int, int, Str>);
static_assert(not OneOf<double, int, Str>);
static_assert(not std::is_copy_constructible_v<NonCopyable>);
static_assert(not std::is_move_constructible_v<NonMovable>);
static_assert(not std::is_constructible_v<StaticClass>);
static_assert(Iterable<std::vector<int>>);
static_assert(Countable<std::vector<int>>);
static_assert(Callable<decltype([](int n) { return n; }), int(int)>);
static_assert(not Callable<decltype([](int n) { return n; }), void(int)>);

TEST(Core, NumericLimitsAndValueWrappers) {
    EXPECT_EQ(maxOf<u16>(), 65535);
    EXPECT_EQ(minOf<i16>(), -32768);
    EXPECT_EQ(max<u32>(), std::numeric_limits<u32>::max());
    EXPECT_EQ(min<i32>(), std::numeric_limits<i32>::min());
    Name name{"daily"};
    EXPECT_EQ(name.get(), "daily");
    EXPECT_EQ(name, Name{"daily"});
    EXPECT_NE(name, Name{"weekly"});
    EXPECT_EQ(std::hash<Name>{}(name), std::hash<Str>{}("daily"));
    Tag<Str> tag{"tag"};
    EXPECT_EQ(std::hash<Tag<Str>>{}(tag), std::hash<Str>{}("tag"));
}

TEST(Core, PreservesUnderlyingExceptions) {
    static_assert(not std::is_nothrow_invocable_v<
                  std::hash<Wrapper<ThrowingHashKey>>,
                  const Wrapper<ThrowingHashKey>&>);
    EXPECT_THROW(
        std::hash<Wrapper<ThrowingHashKey>>{}(
            Wrapper<ThrowingHashKey>{ThrowingHashKey{}}
        ),
        std::runtime_error
    );
    EXPECT_THROW(
        std::hash<Tag<ThrowingHashKey>>{}(
            Tag<ThrowingHashKey>{ThrowingHashKey{}}
        ),
        std::runtime_error
    );
}

}  // namespace kc::test
