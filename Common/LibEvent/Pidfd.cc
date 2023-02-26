#include "Pidfd.h"

#include <sys/syscall.h>
#include <unistd.h>

namespace Event {

ErrorOr<NonnullRefPtr<Pidfd>> Pidfd::open(u32 pid)
{
    int pidfd = syscall(SYS_pidfd_open, pid, 0);

    if (pidfd == -1)
        return Error::from_errno(errno);

    return adopt_ref(*new Pidfd(pidfd));
}

Pidfd::Pidfd(int fd)
    : m_pidfd(fd)
{
}

Pidfd::~Pidfd() = default;

ErrorOr<int> Pidfd::getfd(int target_fd)
{
    int fd = syscall(SYS_pidfd_getfd, m_pidfd, target_fd, 0);
    if (fd == -1)
        return Error::from_errno(errno);

    return fd;
}

void Pidfd::handle_event(EventLoop::Event const& event)
{
    // FIXME: Figure out what to do with this. It should return when the remote
    // process exits but currently we have no use case for that.
}

EventLoop::Event Pidfd::event()
{
    EventLoop::Event event = { 0 };

    event.data.fd = m_pidfd;
    event.events = EventLoop::Event::Kind::EPOLLIN;

    return event;
}

void Pidfd::close()
{
    ::close(m_pidfd);
}

}
