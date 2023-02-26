#include <AK/ByteBuffer.h>
#include <AK/FixedArray.h>
#include <AK/IPv4Address.h>
#include <AK/NonnullRefPtr.h>
#include <AK/ScopeGuard.h>

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "EventLoop.h"
#include "TcpStream.h"

namespace Event {

ErrorOr<NonnullRefPtr<TcpStream>> TcpStream::create_from_fd(int fd)
{
    return adopt_ref(*new TcpStream(fd));
}

ErrorOr<NonnullRefPtr<TcpStream>> TcpStream::connect(IPv4Address address, u16 port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        return Error::from_errno(errno);
    }

    struct sockaddr_in addr = { 0 };

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = address.to_in_addr_t();

    if (::connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        return Error::from_errno(errno);
    }

    return TcpStream::create_from_fd(sockfd);
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

    buffer.resize(nread);

    return buffer;
}

ErrorOr<size_t> TcpStream::send_fds(FixedArray<int>& fds)
{
    struct msghdr msg = {
        .msg_control = malloc(CMSG_SPACE(fds.size() * sizeof(size_t))),
        .msg_controllen = CMSG_SPACE(fds.size() * sizeof(size_t)),
    };

    auto* fds_size = (size_t*)malloc(sizeof(size_t));
    fds_size[0] = fds.size();

    auto fds_size_guard = ScopeGuard([&] { free(fds_size); });

    struct iovec io = { .iov_base = fds_size, .iov_len = sizeof(size_t) };

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);

    VERIFY(cmsg);

    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int) * fds.size());

    *((int**)CMSG_DATA(cmsg)) = (int*)(fds.data());

    msg.msg_controllen = CMSG_SPACE(sizeof(int) * fds.size());
    msg.msg_iov = &io;

    if (sendmsg(m_sockfd, &msg, 0) == -1) {
        return Error::from_errno(errno);
    }

    return fds.size();
}

ErrorOr<Vector<int>> TcpStream::read_fds(size_t const& amount)
{
    auto result = Vector<int>();

    struct msghdr msg = { 0 };
    u8 fds_sent = { 0 };

    struct iovec io = { .iov_base = &fds_sent, .iov_len = sizeof(fds_sent) };
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    if (recvmsg(m_sockfd, &msg, 0) == -1) {
        return Error::from_errno(errno);
    }

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    unsigned char* data = CMSG_DATA(cmsg);

    if (!data)
        return result;

    outln("data = {}", (void const*)data);

    int** fds = ((int**)data);
    for (size_t i = 0; fds_sent > i; ++i) {
        if (fds[i])
            TRY(result.try_append(*fds[i]));
    }

    return result;
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

void TcpStream::close()
{
    (void)EventLoop::the()->ctl(this, EventLoop::Action::Del);
    ::close(m_sockfd);
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
