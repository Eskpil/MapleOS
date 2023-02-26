#include <AK/NeverDestroyed.h>
#include <AK/WeakPtr.h>

#include <signal.h>
#include <sys/signalfd.h>

#include "Signals.h"

static NeverDestroyed<WeakPtr<Event::Signals>> s_the;

namespace Event {

ErrorOr<NonnullRefPtr<SignalHandler>> SignalHandler::create(Signal signal)
{
    auto handler = adopt_ref(*new SignalHandler());

    Signals::the()->handle(signal, handler.ptr());

    return handler;
}

ErrorOr<OwnPtr<Signals>> Signals::create()
{
    return adopt_own(*new Signals);
}

// NOTE: Signals is a very special case where the EventLoop class
// is in charge of adding us to the loop.
Signals::Signals()
    : m_signalfd(-1)
{
    VERIFY(!*s_the);

    MUST(m_handlers.try_ensure_capacity(32));

    *s_the = *this;
}

Signals::~Signals()
{
    MUST(EventLoop::the()->ctl(this, EventLoop::Action::Del));
    ::close(m_signalfd);
}

Signals* Signals::the()
{
    return *s_the;
}

void Signals::handle(Signal signal, RefPtr<SignalHandler> handler)
{
    m_handlers.set(signal, handler);
}

void Signals::handle_event(EventLoop::Event const& event)
{
    if (event.events == EventLoop::Event::Kind::EPOLLIN) {
        struct signalfd_siginfo siginfo = { 0 };

        size_t nread = read(m_signalfd, &siginfo, sizeof(siginfo));
        if (nread != sizeof(siginfo)) {
            outln("failed to read siginfo");
            VERIFY_NOT_REACHED();
        }

        auto handler = m_handlers.get((Signal)siginfo.ssi_signo).value_or(nullptr);
        if (!handler && (Signal)siginfo.ssi_signo == Signal::INT) {
            outln("gracefully exiting the eventloop");
            EventLoop::the()->exit(1);
        }

        if (!handler) {
            outln("no handler for signal: {} but we still got if for some reason", siginfo.ssi_signo);
            VERIFY_NOT_REACHED();
        }

        if (!handler->on_action && (Signal)siginfo.ssi_signo == Signal::INT) {
            outln("gracefully exiting the eventloop, but maybe you wanted to install your own on_action callback for this.");
            EventLoop::the()->exit(1);
        }

        if (!handler->on_action) {
            outln("handler for signal: {} is missing callback", siginfo.ssi_signo);
            VERIFY_NOT_REACHED();
        }

        handler->on_action();
    }
}

void Signals::close()
{
    (void)MUST(EventLoop::the()->ctl(this, EventLoop::Action::Del));
    ::close(m_signalfd);
}

EventLoop::Event Signals::event()
{
    EventLoop::Event event { 0 };

    if (m_signalfd == -1) {
        sigset_t mask;

        sigemptyset(&mask);

        sigaddset(&mask, (int)Signal::INT);

        for (auto const& pair : m_handlers) {
            if (pair.key != Signal::INT)
                sigaddset(&mask, (int)pair.key);
        }

        if (sigprocmask(SIG_BLOCK, &mask, nullptr) == -1) {
            VERIFY_NOT_REACHED();
        }

        m_signalfd = signalfd(-1, &mask, 0);
        if (m_signalfd == -1) {
            VERIFY_NOT_REACHED();
        }
    }

    event.events = EventLoop::Event::Kind::EPOLLIN | EventLoop::Event::Kind::EPOLLET;
    event.data.fd = m_signalfd;

    return event;
}

}
