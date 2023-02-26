#include <AK/Error.h>
#include <AK/FixedArray.h>
#include <AK/NeverDestroyed.h>
#include <AK/OwnPtr.h>
#include <AK/WeakPtr.h>

#include <errno.h>
#include <sys/epoll.h>

#include "EventLoop.h"
#include "Signals.h"

static NeverDestroyed<WeakPtr<Event::EventLoop>> s_the;

// NOTE: We could have tied this directly to the
// event loop object, but then we risk having
// headers import loops. And we want to avoid it.
static NeverDestroyed<OwnPtr<Event::Signals>> s_signals;

namespace Event {

ErrorOr<OwnPtr<EventLoop>> EventLoop::create()
{
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        return Error::from_errno(errno);
    }

    return adopt_own(*new EventLoop(epollfd));
}

EventLoop::EventLoop(int const& epollfd)
    : m_epollfd(epollfd)
{
    VERIFY(!*s_the);

    *s_signals = MUST(Signals::create());

    MUST(m_handlers.try_ensure_capacity(MAX_EVENTS));

    *s_the = *this;
}

EventLoop::~EventLoop()
{
    close(m_epollfd);
}

EventLoop* EventLoop::the()
{
    return *s_the;
}

int EventLoop::exec()
{
    if (Signals::the()) {
        MUST(ctl(Signals::the(), EventLoop::Action::Add));
    }

    for (;;) {
        auto size = m_handlers.size() == 0 ? 1 : m_handlers.size();
        auto events = MUST(FixedArray<Event>::try_create(size));
        int nfds = epoll_wait(m_epollfd, (epoll_event*)events.data(), (int)size, 50);

        if (nfds == -1) {
            outln("epoll_wait failed with error: {}", strerror(errno));
            continue;
        }

        for (size_t i = 0; nfds > i; ++i) {
            auto event = events.at(i);
            auto handler = m_handlers.get(event.data.fd).value_or(nullptr);

            if (!handler) {
                outln("Event loop has stray handler.");
                VERIFY_NOT_REACHED();
            }

            handler->handle_event(event);
        }
    }
}

void EventLoop::exit(int code)
{
    for (auto handler : m_handlers) {
        handler.value->close();
    }

    close(m_epollfd);

    ::exit(code);
}

ErrorOr<bool> EventLoop::ctl(EventHandler* handler, EventLoop::Action action)
{
    auto event = handler->event();

    if (epoll_ctl(m_epollfd, action, event.data.fd, (epoll_event*)&event) == -1) {
        return Error::from_errno(errno);
    }

    if (action == Action::Add)
        m_handlers.set(event.data.fd, handler);

    if (action == Action::Del)
        m_handlers.remove(event.data.fd);

    return true;
}

// NOTE: This is to discover if the EventHandler has implemented
// its virtuals. If they have not this will crash the program,
// and it is easy to see why.
void EventHandler::handle_event(EventLoop::Event const&)
{
    VERIFY_NOT_REACHED();
}

void EventHandler::close()
{
    VERIFY_NOT_REACHED();
}

EventLoop::Event EventHandler::event()
{
    VERIFY_NOT_REACHED();
}
}