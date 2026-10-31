#include <barrier>
#include <kcore/Profiler.hh>
#include <thread>

#include "LogCapture.hh"
#include "TestSupport.hh"

namespace kc::test {
using namespace testing;

TEST(RegionTimer, EmitsPairedEventsIncludingNestedRegions) {
    ProfilerEvents events;
    {
        RegionTimer outer{"outer", events};
        ASSERT_EQ(events.size(), 1);
        EXPECT_EQ(events.front().type, ProfilerEvent::Type::begin);
        EXPECT_EQ(events.front().name, "outer");
        RegionTimer inner{"inner", events};
    }
    ASSERT_EQ(events.size(), 4);
    EXPECT_EQ(events[1].name, "inner");
    EXPECT_EQ(events[2].name, "inner");
    EXPECT_EQ(events[2].type, ProfilerEvent::Type::end);
    EXPECT_EQ(events[3].name, "outer");
    EXPECT_EQ(events[3].type, ProfilerEvent::Type::end);
    for (size_t i = 1; i < events.size(); ++i) {
        EXPECT_GE(events[i].timestamp, events[i - 1].timestamp);
    }
}

TEST(Profiler, ReportsTheConfiguredInstrumentationState) {
#ifdef KCORE_ENABLE_PROFILING
    EXPECT_TRUE(profilerEnabled());
#else
    EXPECT_FALSE(profilerEnabled());
#endif
}

using ProfilerTest = LogCapture;

TEST_F(
    ProfilerTest,
    SummaryPrintsNestedRegionsInInsertionOrderAndComputesStatistics
) {
    using Type = ProfilerEvent::Type;
    auto time = [](auto duration) {
        return std::chrono::steady_clock::time_point{duration};
    };
    ProfilerEvents events{
        {Type::end, "orphan", time(0ns)},
        {Type::begin, "outer", time(0ns)},
        {Type::begin, "z-first", time(0ns)},
        {Type::end, "z-first", time(1ms)},
        {Type::begin, "a-second", time(1ms)},
        {Type::end, "a-second", time(2ms)},
        {Type::begin, "z-first", time(2ms)},
        {Type::end, "z-first", time(4ms)},
        {Type::end, "outer", time(5ms)},
    };
    ProfilerSummary{{{std::this_thread::get_id(), events}}}.print();
    const auto text = output.str();
    EXPECT_THAT(text, HasSubstr("outer  5.000ms"));
    EXPECT_THAT(text, HasSubstr("z-first  x2  3.000ms  (avg 1.500ms)"));
    EXPECT_THAT(text, HasSubstr("a-second  1.000ms"));
    EXPECT_LT(text.find("z-first"), text.find("a-second"));
    EXPECT_THAT(text, Not(HasSubstr("orphan")));
}

TEST_F(
    ProfilerTest, GeneratingSummaryClearsEventsForRegisteredAndAutomaticThreads
) {
    auto& profiler = Profiler::get();
    static_cast<void>(profiler.generateSummary());
    profiler.registerThread();
    {
        auto region = profiler.profileRegion("first");
    }
    std::thread worker{[&] { auto region = profiler.profileRegion("worker"); }};
    worker.join();
    profiler.generateSummary().print();
    EXPECT_THAT(output.str(), HasSubstr("first"));
    EXPECT_THAT(output.str(), HasSubstr("worker"));
    output.str("");
    profiler.generateSummary().print();
    EXPECT_THAT(output.str(), Not(HasSubstr("first")));
    EXPECT_THAT(output.str(), Not(HasSubstr("worker")));
}

TEST_F(ProfilerTest, RetainsOpenNestedRegionsUntilTheyAreComplete) {
    Profiler profiler;
    {
        auto outer = profiler.profileRegion("retained outer");
        {
            auto inner = profiler.profileRegion("retained inner");
            profiler.generateSummary().print();
        }
        profiler.generateSummary().print();
        EXPECT_THAT(output.str(), Not(HasSubstr("retained outer")));
        EXPECT_THAT(output.str(), Not(HasSubstr("retained inner")));
    }
    profiler.generateSummary().print();
    EXPECT_THAT(output.str(), HasSubstr("retained outer"));
    EXPECT_THAT(output.str(), HasSubstr("retained inner"));
    output.str("");
    profiler.generateSummary().print();
    EXPECT_THAT(output.str(), Not(HasSubstr("retained")));
}

TEST_F(ProfilerTest, SnapshotsCanOverlapRecordingAndInstancesAreIndependent) {
    Profiler first, second;
    {
        auto region = first.profileRegion("first instance");
    }
    {
        auto region = second.profileRegion("second instance");
    }
    second.generateSummary().print();
    EXPECT_THAT(output.str(), HasSubstr("second instance"));
    EXPECT_THAT(output.str(), Not(HasSubstr("first instance")));
    first.generateSummary().print();
    EXPECT_THAT(output.str(), HasSubstr("first instance"));
    std::barrier start{2};
    std::thread worker{[&] {
        start.arrive_and_wait();
        for (int i = 0; i < 1000; ++i) {
            auto region = first.profileRegion("concurrent");
        }
    }};
    start.arrive_and_wait();
    for (int i = 0; i < 100; ++i) {
        (void)first.generateSummary();
    }
    worker.join();
    (void)first.generateSummary();
}

}  // namespace kc::test
