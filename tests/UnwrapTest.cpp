#include <kcore/Unwrap.hh>

#include "TestSupport.hh"

namespace kc::test {
using namespace testing;

TEST(Unwrap, MovesSuccessfulValues) {
    Result<std::unique_ptr<int>> result{std::make_unique<int>(42)};
    auto value = unwrap(std::move(result));
    EXPECT_EQ(*value, 42);
}

TEST(UnwrapDeathTest, AbortsOnError) {
    EXPECT_DEATH(
        unwrap(
            Result<int>{Error::unexpected(ErrorCode::invalidArgument, "bad")}
        ),
        ""
    );
}

TEST(Unwrap, AcceptsSuccessfulVoidResults) { unwrap(Result<void>{}); }

}  // namespace kc::test
