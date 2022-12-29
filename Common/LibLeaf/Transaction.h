#pragma once

#include <AK/Types.h>

class Example {
public:
    Example()
    {
        m_name = packet.get("name");
        m_fd = packet.get_fd("fd");

        m_name = packet.read_string();
        m_fd = packet.fd_at(0);
    }
};

namespace Leaf {
class Transaction {
public:
private:
    u32 m_transaction_id;
};
}