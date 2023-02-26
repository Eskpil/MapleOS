#pragma once

#include <AK/ByteBuffer.h>
#include <AK/Error.h>
#include <AK/NonnullRefPtr.h>
#include <AK/RefCounted.h>

#include "EventLoop.h"

namespace Event {

class UdpServer : public RefCounted<UdpServer>
    , public EventHandler {
public:
    static ErrorOr<NonnullRefPtr<UdpServer>> create();

    explicit UdpServer(int const&);

    ErrorOr<bool> bind(IPv4Address const&, uint16_t const&);

    ErrorOr<ByteBuffer> receive();
    ErrorOr<ByteBuffer> receive_sized(size_t const&);

    ErrorOr<bool> send(ByteBuffer const&);

    Function<void()> on_ready;

    ~UdpServer() override;

private:
    void handle_event(EventLoop::Event const&) override;
    EventLoop::Event event() override;
    void close() override;

    int m_sockfd;
};

};