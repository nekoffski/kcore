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

TEST(Env, RejectsPartialNumbersAndUnsignedNegatives) {
    for (auto* value : {"42junk", "1.5junk", "", "  "}) {
        ScopedEnv env{"KCORE_AUDIT_ENV", value};
        EXPECT_FALSE(getEnv<u64>("KCORE_AUDIT_ENV"));
        EXPECT_FALSE(getEnv<i64>("KCORE_AUDIT_ENV"));
        EXPECT_FALSE(getEnv<f32>("KCORE_AUDIT_ENV"));
        EXPECT_FALSE(getEnv<f64>("KCORE_AUDIT_ENV"));
    }
    ScopedEnv negative{"KCORE_AUDIT_ENV", " -1 "};
    EXPECT_FALSE(getEnv<u64>("KCORE_AUDIT_ENV"));
    EXPECT_EQ(getEnv<i64>("KCORE_AUDIT_ENV"), -1);
    ScopedEnv spaced{"KCORE_AUDIT_ENV", " \t+42\n"};
    EXPECT_EQ(getEnv<u64>("KCORE_AUDIT_ENV"), 42);
    EXPECT_EQ(getEnv<f64>("KCORE_AUDIT_ENV"), 42.0);
}

}  // namespace kc::test
