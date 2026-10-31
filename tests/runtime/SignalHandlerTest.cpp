#include <kcore/runtime/SignalHandler.hh>

#include "TestSupport.hh"

namespace kc::test {
using namespace testing;
using namespace std::chrono_literals;
TEST(Signal, ConvertsSupportedPosixSignalsAndRejectsOthers) {
    EXPECT_EQ(*fromPosix(SIGINT), Signal::sigint);
    EXPECT_EQ(*fromPosix(SIGTERM), Signal::sigterm);
    EXPECT_EQ(toString(Signal::sigint), "SIGINT");
    EXPECT_EQ(toString(Signal::sigterm), "SIGTERM");
    auto unsupported = fromPosix(SIGUSR1);
    ASSERT_FALSE(unsupported);
    EXPECT_EQ(unsupported.error().code(), ErrorCode::invalidArgument);
}

TEST(SignalHandlerDeathTest, WaitsForSupportedSignalsUntilCallbackStops) {
    EXPECT_EXIT(
        {
            SignalHandler handler;
            int received = 0;
            if (std::raise(SIGINT) != 0) {
                std::_Exit(1);
            }
            handler.wait([&](Signal signal) {
                ++received;
                if (received == 1 && signal == Signal::sigint) {
                    if (std::raise(SIGTERM) != 0) {
                        std::_Exit(1);
                    }
                    return SignalAction::waitForNext;
                }
                if (received != 2 || signal != Signal::sigterm) {
                    std::_Exit(1);
                }
                return SignalAction::stop;
            });
            std::_Exit(received == 2 ? 0 : 1);
        },
        ExitedWithCode(0), ""
    );
}

TEST(SignalHandler, RestoresTheConstructingThreadsMask) {
    sigset_t before{}, during{}, after{};
    ASSERT_EQ(pthread_sigmask(SIG_SETMASK, nullptr, &before), 0);
    {
        SignalHandler handler;
        ASSERT_EQ(pthread_sigmask(SIG_SETMASK, nullptr, &during), 0);
        EXPECT_EQ(sigismember(&during, SIGINT), 1);
        EXPECT_EQ(sigismember(&during, SIGTERM), 1);
    }
    ASSERT_EQ(pthread_sigmask(SIG_SETMASK, nullptr, &after), 0);
    EXPECT_EQ(sigismember(&before, SIGINT), sigismember(&after, SIGINT));
    EXPECT_EQ(sigismember(&before, SIGTERM), sigismember(&after, SIGTERM));
}

}  // namespace kc::test
