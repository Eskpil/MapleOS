#include <AK/Error.h>
#include <AK/NonnullRefPtr.h>
#include <AK/Time.h>

#include <sys/timerfd.h>

#include "EventLoop.h"
#include "Timer.h"

namespace Event {

ErrorOr<NonnullRefPtr<Timer>> Timer::create_single_shot(Time const& time)
{
    auto timerfd = timerfd_create(CLOCK_REALTIME, 0);
    if (timerfd == -1) {
        return Error::from_errno(errno);
    }

    struct itimerspec ispec {
        0
    };
    ispec.it_value = time.to_timespec();

    if (timerfd_settime(timerfd, 0, &ispec, nullptr) == -1) {
        return Error::from_errno(errno);
    }

    auto timer = adopt_ref(*new Timer(Kind::SingleShot, time, timerfd));

    return timer;
}

ErrorOr<NonnullRefPtr<Timer>> Timer::create_repeating(Time const& time, bool instant)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    auto timerfd = timerfd_create(CLOCK_REALTIME, 0);
    if (timerfd == -1) {
        return Error::from_errno(errno);
    }

    auto timespec = time.to_timespec();

    struct itimerspec ispec {
        0
    };
    memset(&ispec, 0, sizeof(ispec));

    if (!instant)
        ispec.it_value = timespec;
    else
        // NOTE: We must set the it_value so the timerfd knows it has to start.
        ispec.it_value.tv_nsec = 1;

    ispec.it_interval = timespec;

    if (timerfd_settime(timerfd, 0, &ispec, nullptr) == -1) {
        return Error::from_errno(errno);
    }

    auto timer = adopt_ref(*new Timer(Kind::Repeating, time, timerfd));

    return timer;
}

Timer::Timer(Kind const& kind, Time const& time, int const& timerfd)
    : m_timerfd(timerfd)
    , m_kind(kind)
    , m_time(time)
{
    VERIFY(EventLoop::the());
    MUST(EventLoop::the()->ctl(this, EventLoop::Action::Add));
}

Timer::~Timer()
{
    ::close(m_timerfd);
}

void Timer::handle_event(EventLoop::Event const& event)
{
    if (event.events == EventLoop::Event::Kind::EPOLLIN) {
        uint64_t value;
        read(m_timerfd, &value, 8);

        if (on_action)
            on_action();

        if (m_kind == Kind::SingleShot) {
            close();
        }
    }
}

void Timer::close()
{
    (void)MUST(EventLoop::the()->ctl(this, EventLoop::Action::Del));
    ::close(m_timerfd);
}

EventLoop::Event Timer::event()
{
    EventLoop::Event event { 0 };

    event.events = EventLoop::Event::Kind::EPOLLIN | EventLoop::Event::EPOLLET;
    event.data.fd = m_timerfd;

    return event;
}

}