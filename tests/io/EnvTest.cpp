#include <kcore/io/Env.hh>

#include "TestSupport.hh"

namespace kc::test {
using namespace testing;

TEST(ScopedEnv, RestoresValuesAfterSettingUnsettingAndEmptyValues) {
    ScopedEnv outer{"KCORE_TEST_ENV", "original"};
    {
        ScopedEnv unset{"KCORE_TEST_ENV"};
        EXPECT_FALSE(getEnv("KCORE_TEST_ENV"));
    }
    EXPECT_EQ(getEnv("KCORE_TEST_ENV"), "original");
    {
        ScopedEnv empty{"KCORE_TEST_ENV", ""};
        EXPECT_EQ(getEnv("KCORE_TEST_ENV"), "");
    }
    EXPECT_EQ(getEnv("KCORE_TEST_ENV"), "original");
}

TEST(Env, MissingInvalidAndOutOfRangeValuesReturnEmpty) {
    ScopedEnv missing{"KCORE_TEST_ENV"};
    EXPECT_FALSE(getEnv("KCORE_TEST_ENV"));
    {
        ScopedEnv invalid{"KCORE_TEST_ENV", "not-a-number"};
        EXPECT_FALSE(getEnv<i64>("KCORE_TEST_ENV"));
        EXPECT_FALSE(getEnv<u64>("KCORE_TEST_ENV"));
        EXPECT_FALSE(getEnv<f32>("KCORE_TEST_ENV"));
        EXPECT_FALSE(getEnv<f64>("KCORE_TEST_ENV"));
    }
    ScopedEnv overflow{"KCORE_TEST_ENV", "99999999999999999999999999999999999"};
    EXPECT_FALSE(getEnv<i64>("KCORE_TEST_ENV"));
    EXPECT_FALSE(getEnv<u64>("KCORE_TEST_ENV"));
}

TEST(Env, ConvertsStringsIntegersAndFloatingPointValues) {
    ScopedEnv value{"KCORE_TEST_ENV", "42"};
    EXPECT_EQ(getEnv("KCORE_TEST_ENV"), "42");
    EXPECT_EQ(getEnv<u64>("KCORE_TEST_ENV"), 42);
    EXPECT_EQ(getEnv<i64>("KCORE_TEST_ENV"), 42);
    EXPECT_EQ(getEnv<f32>("KCORE_TEST_ENV"), 42.0f);
    EXPECT_EQ(getEnv<f64>("KCORE_TEST_ENV"), 42.0);
    ScopedEnv negative{"KCORE_TEST_ENV", "-42"};
    EXPECT_EQ(getEnv<i64>("KCORE_TEST_ENV"), -42);
}

}  // namespace kc::test
