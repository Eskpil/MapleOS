#pragma once

#include <AK/HashMap.h>
#include <AK/NonnullRefPtr.h>
#include <AK/OwnPtr.h>
#include <AK/RefCounted.h>
#include <AK/RefPtr.h>

#include <LibEvent/UdpServer.h>

#include "DhcpPacket.h"
#include "Interface.h"

struct DhcpTransaction {
    explicit DhcpTransaction(RefPtr<Interface> iface, u32 id)
        : interface(iface)
        , id(id)
    {
    }

    RefPtr<Interface> interface;
    bool accepted_offer { false };
    bool has_ip { false };
    u32 id;
    u32 offered_lease_time { 0 };
};

class DhcpClient : public RefCounted<DhcpClient> {
public:
    static ErrorOr<NonnullRefPtr<DhcpClient>> create();
    DhcpClient();

    ErrorOr<bool> discover_for_iface(RefPtr<Interface>);

    ~DhcpClient();

private:
    HashMap<u32, OwnPtr<DhcpTransaction>> m_ongoing_transactions;

    RefPtr<Event::UdpServer> m_server;

    ErrorOr<bool> handle_offer(DhcpPacket const&, ParsedDhcpOptions const&);
    ErrorOr<bool> handle_request(DhcpPacket const&, DhcpTransaction);
    ErrorOr<bool> handle_ack(DhcpPacket const&, ParsedDhcpOptions const&);
};
