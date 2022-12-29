#include <AK/Error.h>
#include <AK/IPv4Address.h>
#include <AK/NonnullRefPtr.h>

#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "EventLoop.h"
#include "UdpServer.h"

namespace Event {

ErrorOr<NonnullRefPtr<UdpServer>> UdpServer::create()
{
    auto sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        return Error::from_errno(errno);
    }

    return adopt_ref(*new UdpServer(sockfd));
}

UdpServer::UdpServer(int const& sockfd)
    : m_sockfd(sockfd)
{
    MUST(EventLoop::the()->ctl(this, EventLoop::Action::Add));
}

UdpServer::~UdpServer()
{
}

ErrorOr<bool> UdpServer::bind(IPv4Address const& ip, uint16_t const& port)
{
    struct sockaddr_in addr = { 0 };

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ip.to_in_addr_t();

    if (::bind(m_sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        return Error::from_errno(errno);
    }

    return true;
}

ErrorOr<ByteBuffer> UdpServer::receive()
{
    size_t size = 0;
    if (ioctl(m_sockfd, FIONREAD, &size) == -1) {
        return Error::from_errno(errno);
    }

    return receive_sized(size);
}

ErrorOr<ByteBuffer> UdpServer::receive_sized(size_t const& size)
{
    auto buffer = TRY(ByteBuffer::create_uninitialized(size));

    auto nread = recv(m_sockfd, buffer.data(), size, 0);
    if (nread == -1)
        return Error::from_errno(errno);

    return buffer;
}

ErrorOr<bool> UdpServer::send(ByteBuffer const& buffer)
{
    auto size = buffer.size();

    auto nwritten = ::send(m_sockfd, buffer.data(), size, 0);
    if (nwritten != size)
        return Error::from_errno(errno);

    return true;
}

void UdpServer::handle_event(EventLoop::Event const& event)
{
    if (event.events == EventLoop::Event::Kind::EPOLLIN) {
        if (on_ready)
            on_ready();
    }
}

EventLoop::Event UdpServer::event()
{
    EventLoop::Event event { 0 };

    VERIFY(m_sockfd >= 0);

    event.events = EventLoop::Event::Kind::EPOLLIN | EventLoop::Event::EPOLLET;
    event.data.fd = m_sockfd;

    return event;
}

}
