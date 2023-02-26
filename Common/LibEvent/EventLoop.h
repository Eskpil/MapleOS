#pragma once

#include <AK/Error.h>
#include <AK/Function.h>
#include <AK/HashMap.h>
#include <AK/OwnPtr.h>
#include <AK/Weakable.h>

#include <sys/epoll.h>

namespace Event {

class EventHandler;

// NOTE: This event loop is primarily based on epoll.
class EventLoop : public Weakable<EventLoop> {

public:
    static constexpr size_t MAX_EVENTS = 512;

    enum Action : int {
        Add = 1,
        Mod = 2,
        Del = 3,
    };

    struct [[gnu::packed]] Event {
        enum Kind {
            EPOLLIN = 0x001,
            EPOLLPRI = 0x002,
            EPOLLOUT = 0x004,
            EPOLLRDNORM = 0x040,
            EPOLLRDBAND = 0x080,
            EPOLLWRNORM = 0x100,
            EPOLLWRBAND = 0x200,
            EPOLLMSG = 0x400,
            EPOLLERR = 0x008,
            EPOLLHUP = 0x010,
            EPOLLRDHUP = 0x2000,
            EPOLLEXCLUSIVE = 1u << 28,
            EPOLLWAKEUP = 1u << 29,
            EPOLLONESHOT = 1u << 30,
            EPOLLET = 1u << 31
        };
        u32 events;
        epoll_data_t data;
    };

    static EventLoop* the();
    static ErrorOr<OwnPtr<EventLoop>> create();
    explicit EventLoop(int const&);

    ErrorOr<bool> ctl(EventHandler*, Action);

    [[noreturn]] void exit(int);
    int exec();

    ~EventLoop();

private:
    int m_epollfd;

    HashMap<int, EventHandler*> m_handlers;
};

class EventHandler {
public:
    virtual void handle_event(EventLoop::Event const&);
    virtual EventLoop::Event event();
    virtual void close();

    virtual ~EventHandler() = default;
};

}