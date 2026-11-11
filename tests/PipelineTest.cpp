#include <kcore/Pipeline.hh>
#include <memory>
#include <stdexcept>

#include "TestSupport.hh"

namespace kc::test {
namespace {

template <typename F>
concept CanStartPipeline =
    requires(F&& f) { startPipeline(std::forward<F>(f)); };

template <typename P, typename F>
concept CanThen =
    requires(P&& p, F&& f) { std::forward<P>(p).then(std::forward<F>(f)); };

template <typename P, typename F>
concept CanTransform = requires(P&& p, F&& f) {
    std::forward<P>(p).transform(std::forward<F>(f));
};

template <typename P, typename F>
concept CanCheck =
    requires(P&& p, F&& f) { std::forward<P>(p).check(std::forward<F>(f)); };

template <typename P, typename F>
concept CanInspect =
    requires(P&& p, F&& f) { std::forward<P>(p).inspect(std::forward<F>(f)); };

template <typename P, typename F>
concept CanMapError =
    requires(P&& p, F&& f) { std::forward<P>(p).mapError(std::forward<F>(f)); };

template <typename P, typename F>
concept CanRecover =
    requires(P&& p, F&& f) { std::forward<P>(p).recover(std::forward<F>(f)); };

static_assert(not CanStartPipeline<decltype([] { return 1; })>);
static_assert(
    not CanStartPipeline<decltype([](int) -> Result<int> { return 1; })>
);
static_assert(not CanStartPipeline<decltype([]() -> std::expected<int, int> {
    return 1;
})>);
static_assert(not CanThen<Pipeline<int>, decltype([](int n) { return n; })>);
static_assert(not CanTransform<
              Pipeline<int>, decltype([](int n) -> Result<int> { return n; })>);
static_assert(
    not CanTransform<Pipeline<int>, decltype([](int& n) -> int& { return n; })>
);
static_assert(not CanCheck<Pipeline<int>, decltype([](int) { return true; })>);
static_assert(
    not CanCheck<Pipeline<int>, decltype([](int) -> Result<int> { return 1; })>
);
static_assert(not CanCheck<Pipeline<int>, decltype([](int&) -> Result<void> {
                               return {};
                           })>);
static_assert(not CanInspect<Pipeline<int>, decltype([](int) -> Result<void> {
                                 return {};
                             })>);
static_assert(
    not CanMapError<Pipeline<int>, decltype([](const Error&) { return 1; })>
);
static_assert(
    not CanRecover<Pipeline<int>, decltype([](const Error&) -> Result<void> {
                       return {};
                   })>
);
static_assert(not CanCheck<
              Pipeline<std::unique_ptr<int>>&,
              decltype([](const auto&) -> Result<void> { return {}; })>);
static_assert(
    not CanThen<
        Pipeline<std::unique_ptr<int>>&,
        decltype([](std::unique_ptr<int>) -> Result<void> { return {}; })>
);
static_assert(sizeof(Pipeline<int>) == sizeof(Result<int>));
static_assert(
    sizeof(Pipeline<std::unique_ptr<int>>) ==
    sizeof(Result<std::unique_ptr<int>>)
);

struct Category {
    int operator()(int&) const { return 1; }
    int operator()(const int&) const { return 2; }
    int operator()(int&&) const { return 3; }
    int operator()(const int&&) const { return 4; }
};

struct CallableCategory {
    int operator()(int) & { return 1; }
    int operator()(int) const& { return 2; }
    int operator()(int) && { return 3; }
    int operator()(int) const&& { return 4; }
};

struct ErrorCategory {
    int operator()(Error&) const { return 1; }
    int operator()(const Error&) const { return 2; }
    int operator()(Error&&) const { return 3; }
    int operator()(const Error&&) const { return 4; }
};

struct Counted {
    struct Counts {
        int copies{};
        int moves{};
    };
    Counts& counts;
    int value;

    Counted(Counts& counts, int value) : counts(counts), value(value) {}
    Counted(const Counted& other) : counts(other.counts), value(other.value) {
        ++counts.copies;
    }
    Counted(Counted&& other) noexcept
        : counts(other.counts), value(other.value) {
        ++counts.moves;
    }
};

}  // namespace

TEST(FluentPipeline, StartsImmediatelyAndRunsNamedStepsInOrder) {
    std::vector<int> calls;
    const auto first = [&]() -> Result<int> {
        calls.push_back(1);
        return 4;
    };
    const auto second = [&](int n) -> Result<Str> {
        calls.push_back(2);
        return std::to_string(n);
    };
    const auto third = [&](const Str& s) {
        calls.push_back(3);
        return s + "2";
    };
    auto started = startPipeline(first);
    EXPECT_THAT(calls, testing::ElementsAre(1));
    const auto r = started.then(second).transform(third).result();
    ASSERT_TRUE(r);
    EXPECT_EQ(*r, "42");
    EXPECT_THAT(calls, testing::ElementsAre(1, 2, 3));
    EXPECT_EQ(*started.result(), 4);
}

TEST(FluentPipeline, PreservesFirstFailureAndSkipsSuccessSteps) {
    for (const auto failFirst : {false, true}) {
        int calls = 0;
        const auto first = [&]() -> Result<int> {
            if (failFirst) {
                return Error::unexpected(ErrorCode::invalidArgument, "first");
            }
            return 7;
        };
        const auto second = [&](int) -> Result<int> {
            ++calls;
            return Error::unexpected(ErrorCode::fileSystemError, "second");
        };
        const auto r = startPipeline(first)
                           .then(second)
                           .check([&](int) -> Result<void> {
                               ++calls;
                               return {};
                           })
                           .inspect([&](int) { ++calls; })
                           .transform([&](int n) {
                               ++calls;
                               return n;
                           })
                           .result();
        ASSERT_FALSE(r);
        EXPECT_EQ(r.error().message(), failFirst ? "first" : "second");
        EXPECT_EQ(
            r.error().code(),
            failFirst ? ErrorCode::invalidArgument : ErrorCode::fileSystemError
        );
        EXPECT_EQ(calls, failFirst ? 0 : 1);
    }
}

TEST(FluentPipeline, CheckRetainsMoveOnlyValueAndPropagatesFailure) {
    for (const auto fail : {false, true}) {
        int observed = 0;
        const auto first = []() -> Result<std::unique_ptr<int>> {
            return std::make_unique<int>(7);
        };
        const auto check =
            [&](const std::unique_ptr<int>& value) -> Result<void> {
            EXPECT_EQ(*value, 7);
            if (fail) {
                return Error::unexpected(ErrorCode::invalidArgument, "check");
            }
            return {};
        };
        const auto r =
            startPipeline(first)
                .check(check)
                .inspect([&](const auto& value) { observed = *value; })
                .transform([](std::unique_ptr<int> value) { return *value; })
                .result();
        EXPECT_EQ(bool(r), not fail);
        EXPECT_EQ(observed, fail ? 0 : 7);
        if (fail) {
            EXPECT_EQ(r.error().message(), "check");
        }
    }
}

TEST(FluentPipeline, MapsErrorsRecoversAndContinues) {
    const auto first = []() -> Result<int> {
        return Error::unexpected(ErrorCode::fileNotFound, "input");
    };
    const auto context = [](Error error) {
        return Error{error.code(), "read: {}", error.message()};
    };
    const auto recover = [](const Error& error) -> Result<int> {
        EXPECT_EQ(error.message(), "read: input");
        if (error.code() == ErrorCode::fileNotFound) {
            return 9;
        }
        return Error::unexpected(error);
    };
    const auto r = startPipeline(first)
                       .mapError(context)
                       .recover(recover)
                       .transform([](int n) { return n + 1; })
                       .result();
    ASSERT_TRUE(r);
    EXPECT_EQ(*r, 10);

    const auto failed = startPipeline(first)
                            .recover([](const Error&) -> Result<int> {
                                return Error::unexpected(
                                    ErrorCode::invalidArgument, "recovery"
                                );
                            })
                            .result();
    ASSERT_FALSE(failed);
    EXPECT_EQ(failed.error().message(), "recovery");
}

TEST(FluentPipeline, SkipsErrorCallbacksOnSuccessIncludingMoveOnlyValues) {
    int calls = 0;
    auto source = []() -> Result<std::unique_ptr<int>> {
        return std::make_unique<int>(8);
    };
    const auto r =
        startPipeline(source)
            .mapError([&](Error error) {
                ++calls;
                return error;
            })
            .recover([&](const Error& error) -> Result<std::unique_ptr<int>> {
                ++calls;
                return Error::unexpected(error);
            })
            .result();
    ASSERT_TRUE(r);
    EXPECT_EQ(**r, 8);
    EXPECT_EQ(calls, 0);
}

TEST(FluentPipeline, HandlesVoidStepsAndIntentionalValueDiscarding) {
    std::vector<int> calls;
    const auto first = [&]() -> Result<void> {
        calls.push_back(1);
        return {};
    };
    auto r = startPipeline(first)
                 .check([&]() -> Result<void> {
                     calls.push_back(2);
                     return {};
                 })
                 .inspect([&] { calls.push_back(3); })
                 .then([&]() -> Result<int> {
                     calls.push_back(4);
                     return 7;
                 })
                 .then([&](int n) -> Result<void> {
                     EXPECT_EQ(n, 7);
                     calls.push_back(5);
                     return {};
                 })
                 .transform([&] { calls.push_back(6); })
                 .then([&]() -> Result<void> {
                     calls.push_back(7);
                     return {};
                 })
                 .result();
    static_assert(std::same_as<decltype(r), Result<void>>);
    EXPECT_TRUE(r);
    EXPECT_THAT(calls, testing::ElementsAre(1, 2, 3, 4, 5, 6, 7));

    const auto recovered =
        startPipeline([]() -> Result<void> {
            return Error::unexpected(ErrorCode::fileNotFound, "missing");
        })
            .mapError([](Error error) { return error; })
            .recover([](const Error&) -> Result<void> { return {}; })
            .transform([] { return 11; })
            .result();
    ASSERT_TRUE(recovered);
    EXPECT_EQ(*recovered, 11);

    int skipped = 0;
    const auto failed = startPipeline(first)
                            .then([]() -> Result<void> {
                                return Error::unexpected(
                                    ErrorCode::invalidArgument, "void step"
                                );
                            })
                            .then([&]() -> Result<void> {
                                ++skipped;
                                return {};
                            })
                            .result();
    ASSERT_FALSE(failed);
    EXPECT_EQ(failed.error().message(), "void step");
    EXPECT_EQ(skipped, 0);
}

TEST(
    FluentPipeline, ForwardsCallableAndValueCategoriesWithoutCopyingCallables
) {
    auto load = [p = std::make_unique<int>(7)]() mutable -> Result<int> {
        return ++*p;
    };
    auto convert = [p = std::make_unique<int>(2)](int n) { return n + *p; };
    EXPECT_EQ(*startPipeline(load).transform(convert).result(), 10);
    EXPECT_EQ(*startPipeline(load).result(), 9);

    CallableCategory f;
    const CallableCategory cf;
    EXPECT_EQ(*startPipeline(load).transform(f).result(), 1);
    EXPECT_EQ(*startPipeline(load).transform(cf).result(), 2);
    EXPECT_EQ(*startPipeline(load).transform(std::move(f)).result(), 3);
    EXPECT_EQ(*startPipeline(load).transform(std::move(cf)).result(), 4);

    auto p = startPipeline([]() -> Result<int> { return 3; });
    const auto c = p;
    const auto bind = [](auto&& value) -> Result<int> {
        return Category{}(std::forward<decltype(value)>(value));
    };
    EXPECT_EQ(*p.then(bind).result(), 1);
    EXPECT_EQ(*c.then(bind).result(), 2);
    EXPECT_EQ(*std::move(p).then(bind).result(), 3);
    EXPECT_EQ(*std::move(c).then(bind).result(), 4);
    auto q = startPipeline([]() -> Result<int> { return 3; });
    const auto d = q;
    EXPECT_EQ(*q.transform(Category{}).result(), 1);
    EXPECT_EQ(*d.transform(Category{}).result(), 2);
    EXPECT_EQ(*std::move(q).transform(Category{}).result(), 3);
    EXPECT_EQ(*std::move(d).transform(Category{}).result(), 4);
}

TEST(FluentPipeline, ForwardsErrorCategoriesForMappingAndRecovery) {
    const auto source = []() -> Result<int> {
        return Error::unexpected(ErrorCode::invalidArgument, "error");
    };
    const auto map = [](auto&& error) {
        return Error{
            ErrorCode::invalidArgument, "{}",
            ErrorCategory{}(std::forward<decltype(error)>(error))
        };
    };
    const auto recover = [](auto&& error) -> Result<int> {
        return ErrorCategory{}(std::forward<decltype(error)>(error));
    };
    auto p = startPipeline(source);
    const auto c = p;
    EXPECT_EQ(p.mapError(map).result().error().message(), "1");
    EXPECT_EQ(c.mapError(map).result().error().message(), "2");
    EXPECT_EQ(std::move(p).mapError(map).result().error().message(), "3");
    EXPECT_EQ(std::move(c).mapError(map).result().error().message(), "4");
    auto q = startPipeline(source);
    const auto d = q;
    EXPECT_EQ(*q.recover(recover).result(), 1);
    EXPECT_EQ(*d.recover(recover).result(), 2);
    EXPECT_EQ(*std::move(q).recover(recover).result(), 3);
    EXPECT_EQ(*std::move(d).recover(recover).result(), 4);
}

TEST(FluentPipeline, ConstructsValuesDirectlyAndOnlyMovesToRetainOrExtract) {
    Counted::Counts counts;
    const auto source = [&]() -> Result<Counted> {
        return Result<Counted>{std::in_place, counts, 7};
    };
    EXPECT_EQ(
        *startPipeline(source)
             .transform([](const Counted& n) { return n.value; })
             .result(),
        7
    );
    EXPECT_EQ(counts.copies, 0);
    EXPECT_EQ(counts.moves, 0);
    const auto r = startPipeline(source)
                       .check([](const Counted&) -> Result<void> { return {}; })
                       .result();
    EXPECT_EQ(r->value, 7);
    EXPECT_EQ(counts.copies, 0);
    EXPECT_EQ(counts.moves, 2);

    auto named = startPipeline(source);
    const auto copy = named.result();
    EXPECT_EQ(copy->value, 7);
    EXPECT_EQ(counts.copies, 1);
}

TEST(FluentPipeline, TransformsImmovableValuesWithoutIntermediateMoves) {
    struct Immovable {
        int value;
        explicit Immovable(int value) : value(value) {}
        Immovable(const Immovable&) = delete;
        Immovable(Immovable&&) = delete;
    };
    const auto source = []() -> Result<Immovable> {
        return Result<Immovable>{std::in_place, 7};
    };
    const auto read = [](const Immovable& value) { return value.value; };
    const auto make = [](int value) { return Immovable{value + 1}; };
    const auto r = startPipeline(source)
                       .transform(read)
                       .transform(make)
                       .transform(read)
                       .result();
    ASSERT_TRUE(r);
    EXPECT_EQ(*r, 8);
}

TEST(
    FluentPipeline, PropagatesCallbackExceptionsWithoutConvertingThemToErrors
) {
    EXPECT_THROW(
        (void)startPipeline([]() -> Result<int> {
            throw std::runtime_error("start");
        }),
        std::runtime_error
    );
    auto p = startPipeline([]() -> Result<int> { return 3; });
    EXPECT_THROW(
        (void)p.transform([](int) -> int {
            throw std::runtime_error("transform");
        }),
        std::runtime_error
    );
    EXPECT_THROW(
        (void)p.check([](int) -> Result<void> {
            throw std::runtime_error("check");
        }),
        std::runtime_error
    );
}

}  // namespace kc::test
