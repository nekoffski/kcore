#include <kcore/Functional.hh>
#include <variant>

#include "TestSupport.hh"

namespace kc::test {
using namespace testing;

TEST(MoveOnlyFunction, EmptyAndResetFunctionsHaveNoCallable) {
    MoveOnlyFunction<int()> empty;
    EXPECT_FALSE(empty);
    MoveOnlyFunction<void()> function;
    function = [] {};
    EXPECT_TRUE(function);
    function = nullptr;
    EXPECT_FALSE(function);
}

TEST(MoveOnlyFunctionDeathTest, InvokingEmptyFunctionAborts) {
    EXPECT_DEATH((MoveOnlyFunction<void()>{}()), "");
}

TEST(MoveOnlyFunction, AcceptsLvalueCallables) {
    int calls = 0;
    auto callback = [&] { return ++calls; };
    MoveOnlyFunction<int()> function{callback};
    EXPECT_EQ(function(), 1);
    function = callback;
    EXPECT_EQ(function(), 2);
}

TEST(MoveOnlyFunction, MovesUniqueCapturesAndForwardsArgumentsAndResults) {
    MoveOnlyFunction<int(int)> function{
        [value = std::make_unique<int>(3)](int n) { return *value + n; }
    };
    auto moved = std::move(function);
    EXPECT_FALSE(function);
    EXPECT_EQ(moved(4), 7);
    function = std::move(moved);
    EXPECT_FALSE(moved);
    EXPECT_EQ(function(2), 5);
    MoveOnlyFunction<void(int&)> update{[](int& n) { ++n; }};
    int n = 0;
    update(n);
    EXPECT_EQ(n, 1);
    MoveOnlyFunction<int(std::unique_ptr<int>)> consume{
        [](std::unique_ptr<int> value) { return *value; }
    };
    EXPECT_EQ(consume(std::make_unique<int>(8)), 8);
}

TEST(Functional, DeferRunsOnScopeExitAndCanBeDismissed) {
    int calls = 0;
    {
        KCORE_DEFER { ++calls; };
    }
    EXPECT_EQ(calls, 1);
    {
        KCORE_NAMED_DEFER(cleanup) { ++calls; };
        cleanup.dismiss();
    }
    EXPECT_EQ(calls, 1);
    DeferCall noop;
}

TEST(Functional, LazyEvaluationRunsOnlyWhenConverted) {
    int calls = 0;
    auto lazy = KCORE_LAZY_EVALUATE(++calls);
    EXPECT_EQ(calls, 0);
    int first = lazy;
    int second = lazy;
    EXPECT_EQ(first, 1);
    EXPECT_EQ(second, 2);
    auto text = lazyEvaluate([] { return "hello"; });
    Str result = text;
    EXPECT_EQ(result, "hello");
}

TEST(Functional, ConvertsFilteredRangesAndVisitsVariants) {
    std::vector<int> input{1, 2, 3, 4};
    auto output = input | std::views::filter([](int n) { return n % 2 == 0; }) |
                  toVector<int>();
    EXPECT_THAT(output, ElementsAre(2, 4));
    EXPECT_THAT(input | toVector<int>(), ElementsAre(1, 2, 3, 4));
    std::variant<int, Str> value{"test"};
    EXPECT_EQ(
        std::visit(
            Overloader{
                [](int n) { return n; },
                [](const Str& s) { return int(s.size()); }
            },
            value
        ),
        4
    );
}

struct ReturnValue {
    int operator()() const { return 42; }
};
struct ReturnReference {
    int value = 42;
    int& operator()() { return value; }
};
static_assert(
    not std::is_constructible_v<MoveOnlyFunction<const int&()>, ReturnValue>
);
static_assert(
    std::is_constructible_v<MoveOnlyFunction<const int&()>, ReturnReference>
);

TEST(MoveOnlyFunction, PreservesReferencesAndRecognizesTypedNullCallbacks) {
    int value = 42;
    MoveOnlyFunction<const int&()> reference{[&]() -> int& { return value; }};
    EXPECT_EQ(&reference(), &value);
    int (*pointer)() = nullptr;
    MoveOnlyFunction<int()> function{pointer};
    EXPECT_FALSE(function);
    function = [] { return 1; };
    function = pointer;
    EXPECT_FALSE(function);
    struct Object {
        int run() { return 1; }
    };
    int (Object::*member)() = nullptr;
    MoveOnlyFunction<int(Object&)> method{member};
    EXPECT_FALSE(method);
}

TEST(Functional, CopiesLvaluesAndSupportsMutableAndMoveOnlyCallbacks) {
    auto callback = [value = 0]() mutable { return ++value; };
    const auto lazy = lazyEvaluate(callback);
    EXPECT_EQ(int(lazy), 1);
    EXPECT_EQ(int(lazy), 2);
    EXPECT_EQ(callback(), 1);
    auto owned = lazyEvaluate([value = std::make_unique<int>(7)]() mutable {
        return ++*value;
    });
    EXPECT_EQ(int(owned), 8);
    EXPECT_EQ(int(owned), 9);
}

}  // namespace kc::test
