#include <kcore/Enum.hh>

#include "TestSupport.hh"

namespace kc::test {
using namespace testing;

enum class Flags { none = 0, first = 1, second = 2 };
KCORE_BIT_ENUM(Flags)
TEST(Enum, CombinesAndChecksEnabledFlags) {
    auto flags = Flags::first | Flags::second;
    EXPECT_TRUE(checkFlag(flags, Flags::first));
    EXPECT_TRUE(checkFlag(flags, Flags::second));
    EXPECT_FALSE(checkFlag(Flags::first, Flags::second));
    EXPECT_EQ(flags & Flags::first, Flags::first);
    flags &= Flags::first;
    EXPECT_EQ(flags, Flags::first);
    flags |= Flags::second;
    EXPECT_TRUE(checkFlag(flags, Flags::second));
}

enum class OtherFlags { first = 1 };
KCORE_BIT_ENUM(OtherFlags)
template <typename A, typename B>
concept ComparableFlags = requires(A a, B b) { checkFlag(a, b); };
static_assert(not ComparableFlags<Flags, OtherFlags>);

TEST(Enum, CompoundAssignmentsReturnTheOriginalLvalue) {
    auto flags = Flags::first;
    EXPECT_EQ(&(flags |= Flags::second), &flags);
    EXPECT_EQ(&(flags &= Flags::first), &flags);
    (flags |= Flags::second) &= Flags::second;
    EXPECT_EQ(flags, Flags::second);
}

}  // namespace kc::test
