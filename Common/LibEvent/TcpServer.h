#pragma once

#include <AK/Error.h>
#include <AK/Function.h>
#include <AK/IPv4Address.h>
#include <AK/IPv6Address.h>
#include <AK/LexicalPath.h>
#include <AK/NonnullRefPtr.h>
#include <AK/RefCounted.h>

#include "EventLoop.h"
#include "SocketType.h"
#include "TcpStream.h"

namespace Event {

class TcpServer : public RefCounted<TcpServer>
    , EventHandler {
public:
    static ErrorOr<NonnullRefPtr<TcpServer>> create(SocketType const&);

    explicit TcpServer(SocketType const&, int const&);

    ErrorOr<bool> bind();
    ErrorOr<bool> bind(LexicalPath const&);
    ErrorOr<bool> bind(IPv4Address const&, uint16_t const&);
    ErrorOr<bool> bind(IPv6Address const&, uint16_t const&);

    Function<void(NonnullRefPtr<TcpStream>)> on_connection;

    ErrorOr<bool> listen() const;

    SocketType socket_type() const { return m_socket_type; }

    void set_address(LexicalPath const);
    void set_address(IPv4Address, u16);

    ~TcpServer() override;

private:
    union Address {
        LexicalPath as_path;
        struct {
            IPv4Address addr;
            u16 port;
        } as_ipv4;
    };

    Address* m_address;

    void handle_event(EventLoop::Event const&) override;
    EventLoop::Event event() override;
    void close() override;

    int m_sockfd;
    SocketType m_socket_type;
};

}