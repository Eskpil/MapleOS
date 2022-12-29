#include <AK/Error.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "TcpServer.h"
#include "TcpStream.h"

namespace Event {

ErrorOr<NonnullRefPtr<TcpServer>> TcpServer::create(SocketType const& type)
{
    auto domain = socktype_domain(type);

    int sockfd = socket(domain, SOCK_STREAM, 0);
    if (sockfd == -1) {
        return Error::from_errno(errno);
    }

    return adopt_ref(*new TcpServer(type, sockfd));
}

TcpServer::TcpServer(SocketType const& socket_type, int const& sockfd)
    : m_sockfd(sockfd)
    , m_socket_type(socket_type)
{
    VERIFY(EventLoop::the());
    MUST(EventLoop::the()->ctl(this, EventLoop::Action::Add));
}

TcpServer::~TcpServer()
{
    // We could have closed earlier because of a signal or some other reason.
    if (m_sockfd != -1)
        MUST(close());
}

ErrorOr<bool> TcpServer::bind(LexicalPath const& path)
{
    VERIFY(m_socket_type == SocketType::Local);

    struct sockaddr_un addr = { 0 };

    addr.sun_family = AF_UNIX;
    bool fits = path.string().copy_characters_to_buffer(addr.sun_path, 108);
    if (!fits)
        return Error::from_string_literal("the path does not fit into addr.sun_path max length of 108.");

    if (::bind(m_sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        return Error::from_errno(errno);

    return true;
}

ErrorOr<bool> TcpServer::bind(IPv4Address const& ip, uint16_t const& port)
{
    VERIFY(m_socket_type == SocketType::IPv4);

    struct sockaddr_in addr = { 0 };

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ip.to_in_addr_t();

    if (::bind(m_sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        return Error::from_errno(errno);

    return true;
}

ErrorOr<bool> TcpServer::bind(IPv6Address const& ip, uint16_t const& port)
{
    VERIFY(m_socket_type == SocketType::IPv6);

    struct sockaddr_in6 addr = { 0 };

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    inet_pton(AF_INET6, ip.to_deprecated_string().characters(), &addr.sin6_addr);

    if (::bind(m_sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        return Error::from_errno(errno);

    return true;
}

ErrorOr<bool> TcpServer::close()
{
    // NOTE: It could be that the socket is closed because of a previous call
    // but for example reference counting tries to close us. Or signal handlers.
    if (m_sockfd == -1)
        return false;

    TRY(EventLoop::the()->ctl(this, EventLoop::Action::Del));

    ::close(m_sockfd);

    m_sockfd = -1;

    return true;
}

ErrorOr<bool> TcpServer::listen() const
{
    if (::listen(m_sockfd, 50) == -1) {
        return Error::from_errno(errno);
    }

    return true;
}

void TcpServer::handle_event(EventLoop::Event const& event)
{
    if (event.events == EventLoop::Event::Kind::EPOLLIN) {
        int streamfd = -1;
        switch (m_socket_type) {
        case SocketType::Local: {
            struct sockaddr_un addr = { 0 };

            socklen_t socklen = sizeof(addr);
            streamfd = accept(m_sockfd, (struct sockaddr*)&addr, &socklen);
        } break;
        case SocketType::IPv4: {
            struct sockaddr_in addr = { 0 };

            socklen_t socklen = sizeof(addr);
            streamfd = accept(m_sockfd, (struct sockaddr*)&addr, &socklen);
        } break;
        case SocketType::IPv6: {
            struct sockaddr_in6 addr = { 0 };

            socklen_t socklen = sizeof(addr);
            streamfd = accept(m_sockfd, (struct sockaddr*)&addr, &socklen);
        } break;
        }

        if (streamfd == -1) {
            outln("EventLoop: Failed to handle incoming stream: {}", strerror(errno));
            return;
        }

        RefPtr<TcpStream> stream = adopt_ref(*new TcpStream(streamfd));

        if (on_connection)
            on_connection(stream);
    }
}

EventLoop::Event TcpServer::event()
{
    EventLoop::Event event { 0 };

    VERIFY(m_sockfd >= 0);

    event.events = EventLoop::Event::Kind::EPOLLIN | EventLoop::Event::EPOLLET;
    event.data.fd = m_sockfd;

    return event;
}

}