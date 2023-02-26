#pragma once

#include <AK/ByteBuffer.h>
#include <AK/Error.h>
#include <AK/FixedArray.h>
#include <AK/Function.h>
#include <AK/IPv4Address.h>
#include <AK/NonnullRefPtr.h>
#include <AK/RefCounted.h>

#include "EventLoop.h"

namespace Event {

class TcpStream : public RefCounted<TcpStream>
    , EventHandler {
public:
    static ErrorOr<NonnullRefPtr<TcpStream>> create_from_fd(int);
    static ErrorOr<NonnullRefPtr<TcpStream>> connect(IPv4Address, u16);

    explicit TcpStream(int const&);

    Function<void()> on_ready;

    ErrorOr<ByteBuffer> read();
    ErrorOr<ByteBuffer> read_sized(size_t const&);

    ErrorOr<size_t> send_fds(FixedArray<int>&);
    ErrorOr<Vector<int>> read_fds(size_t const&);

    int fd() const { return m_sockfd; }

    ErrorOr<bool> write(ByteBuffer const&);

    ~TcpStream() override;

private:
    void handle_event(EventLoop::Event const&) override;
    EventLoop::Event event() override;
    void close() override;

    int m_sockfd;
};

}