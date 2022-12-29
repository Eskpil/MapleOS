#pragma once

#include <AK/Types.h>

#include <sys/socket.h>

namespace Event {
enum class SocketType : u8 {
    Local,
    IPv4,
    IPv6,
};

ALWAYS_INLINE int socktype_domain(SocketType type)
{
    switch (type) {
    case SocketType::Local: {
        return AF_UNIX;
    }
    case SocketType::IPv4: {
        return AF_INET;
    }
    case SocketType::IPv6: {
        return AF_INET6;
    }
    }
}
}