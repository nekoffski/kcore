#include <csignal>
#include <kcore/Log.hh>
#include <kcore/runtime/SignalHandler.hh>

namespace kc {

SignalHandler::SignalHandler() : m_owner(pthread_self()) {
    sigemptyset(&m_sigset);
    sigaddset(&m_sigset, SIGINT);
    sigaddset(&m_sigset, SIGTERM);

    if (const auto error = pthread_sigmask(SIG_BLOCK, &m_sigset, &m_previous)) {
        throw std::system_error(
            error, std::generic_category(), "Block signals"
        );
    }
}

SignalHandler::~SignalHandler() {
    log::expect(
        pthread_equal(m_owner, pthread_self()),
        "SignalHandler destroyed on another thread"
    );
    const auto error = pthread_sigmask(SIG_SETMASK, &m_previous, nullptr);
    log::expect(error == 0, "Could not restore signal mask: {}", error);
}

void SignalHandler::waitImpl(MoveOnlyFunction<SignalAction(Signal)> callback) {
    log::expect(
        pthread_equal(m_owner, pthread_self()),
        "SignalHandler waited on another thread"
    );
    for (;;) {
        i32 posixSignal = 0;
        if (const auto error = sigwait(&m_sigset, &posixSignal)) {
            throw std::system_error(
                error, std::generic_category(), "Wait for signal"
            );
        }

        if (auto signal = fromPosix(posixSignal); signal) {
            if (callback(*signal) == SignalAction::stop) {
                break;
            }
        } else {
            log::panic(
                "Received posix signal that could not be converted to "
                "internal representation: {} - {}",
                posixSignal, signal.error()
            );
        }
    }
}

Result<Signal> fromPosix(i32 posixSignal) {
    switch (posixSignal) {
        case SIGINT:
            return Signal::sigint;
        case SIGTERM:
            return Signal::sigterm;
        default:
            return Error::unexpected(
                ErrorCode::invalidArgument, "Invalid signal"
            );
    }
}

Str toString(Signal signal) {
    switch (signal) {
        case Signal::sigint:
            return "SIGINT";
        case Signal::sigterm:
            return "SIGTERM";
    };
    std::unreachable();
}

}  // namespace kc
