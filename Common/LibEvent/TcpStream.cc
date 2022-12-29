#include <AK/ByteBuffer.h>
#include <AK/NonnullRefPtr.h>

#include <sys/ioctl.h>
#include <unistd.h>

#include "EventLoop.h"
#include "TcpStream.h"

namespace Event {

ErrorOr<NonnullRefPtr<TcpStream>> TcpStream::create_from_fd(int fd)
{
    return adopt_ref(*new TcpStream(fd));
}

TcpStream::TcpStream(int const& fd)
    : m_sockfd(fd)
{
    VERIFY(EventLoop::the());
    MUST(EventLoop::the()->ctl(this, EventLoop::Action::Add));
}

TcpStream::~TcpStream()
{
}

ErrorOr<ByteBuffer> TcpStream::read()
{
    size_t size = 0;
    if (ioctl(m_sockfd, FIONREAD, &size) == -1) {
        return Error::from_errno(errno);
    }

    return read_sized(size);
}

ErrorOr<ByteBuffer> TcpStream::read_sized(size_t const& size)
{
    auto buffer = TRY(ByteBuffer::create_uninitialized(size));

    auto nread = ::read(m_sockfd, buffer.data(), size);
    if (nread == -1)
        return Error::from_errno(errno);

    return buffer;
}

ErrorOr<bool> TcpStream::write(ByteBuffer const& buffer)
{
    auto nwritten = ::write(m_sockfd, buffer.data(), buffer.size());
    if (nwritten == -1)
        return Error::from_errno(errno);

    return true;
}

void TcpStream::handle_event(EventLoop::Event const& event)
{
    if (event.events == EventLoop::Event::Kind::EPOLLIN) {
        if (on_ready)
            on_ready();
    }
}

EventLoop::Event TcpStream::event()
{
    EventLoop::Event event { 0 };

    VERIFY(m_sockfd >= 0);

    event.events = EventLoop::Event::Kind::EPOLLIN | EventLoop::Event::Kind::EPOLLET;
    event.data.fd = m_sockfd;

    return event;
};

}
