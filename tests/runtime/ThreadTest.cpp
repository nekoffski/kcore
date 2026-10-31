#include <atomic>
#include <kcore/runtime/Thread.hh>
#include <latch>

#include "TestSupport.hh"

namespace kc::test {
using namespace testing;
using namespace std::chrono_literals;
class CountingThread : public Thread {
   public:
    explicit CountingThread(std::atomic_int& count)
        : Thread("test"), count(count) {}

   private:
    void run() override { ++count; }
    std::atomic_int& count;
};

TEST(Thread, StartsAndJoinsThroughAllOverloads) {
    std::atomic_int count{0};
    CountingThread first{count}, second{count};
    first.join();
    first.start();
    second.start();
    Thread::join(first, second);
    EXPECT_EQ(count.load(), 2);
    first.join();
    first.start();
    second.start();
    std::vector<Thread*> threads{&first, &second};
    Thread::join(std::span<Thread*>{threads});
    EXPECT_EQ(count.load(), 4);
}

class BlockingThread : public Thread {
   public:
    BlockingThread(std::latch& entered, std::latch& finish)
        : Thread("lifecycle test"), entered(entered), finish(finish) {}

   private:
    void run() override {
        entered.count_down();
        finish.wait();
    }
    std::latch& entered;
    std::latch& finish;
};
static_assert(not std::is_move_constructible_v<BlockingThread>);

TEST(ThreadDeathTest, JoinableDestructionTerminatesInsteadOfImplicitlyJoining) {
    EXPECT_EXIT(
        {
            std::set_terminate([] { std::_Exit(77); });
            std::latch entered{1};
            std::latch finish{1};
            {
                BlockingThread worker(entered, finish);
                worker.start();
                entered.wait();
            }
            std::_Exit(1);
        },
        ExitedWithCode(77), ""
    );
}

TEST(ThreadDeathTest, RejectsASecondStartBeforeLaunchingAnotherWorker) {
    EXPECT_DEATH(
        {
            std::latch entered{1};
            std::latch finish{1};
            BlockingThread worker(entered, finish);
            worker.start();
            entered.wait();
            worker.start();
        },
        ""
    );
}

TEST(Thread, DerivedOwnerJoinsBeforeDestroyingItsMembers) {
    struct Owner : BlockingThread {
        Owner(std::latch& entered, std::latch& finish)
            : BlockingThread(entered, finish), finish(finish) {}
        ~Owner() override {
            finish.count_down();
            join();
        }
        std::latch& finish;
    };
    std::latch entered{1};
    std::latch finish{1};
    {
        Owner owner{entered, finish};
        owner.start();
        entered.wait();
    }
}

}  // namespace kc::test
