#pragma once

#include <csignal>
#include <kcore/Concepts.hh>
#include <kcore/Core.hh>
#include <kcore/Error.hh>
#include <kcore/Functional.hh>

namespace kc {

enum class Signal { sigint, sigterm };

enum class SignalAction { waitForNext, stop };

class SignalHandler : public NonCopyable, public NonMovable {
   public:
    SignalHandler();

    template <typename Callback>
        requires Callable<Callback, SignalAction(Signal)>
    void wait(Callback&& callback) {
        waitImpl(std::forward<Callback>(callback));
    }

   private:
    void waitImpl(MoveOnlyFunction<SignalAction(Signal)>);

    sigset_t m_sigset;
};

Result<Signal> fromPosix(i32 posixSignal);
Str toString(Signal signal);

}  // namespace kc
