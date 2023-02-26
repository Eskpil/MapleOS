#include <AK/DeprecatedString.h>
#include <AK/Error.h>
#include <AK/NonnullRefPtr.h>
#include <AK/Random.h>

#include <LibEvent/Timer.h>

#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "DhcpClient.h"
#include "DhcpPacket.h"
#include "Interface.h"

static bool send(StringView const& iface, DhcpPacket const& packet)
{
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        dbgln("ERROR: socket :: {}", strerror(errno));
        return false;
    }

    ScopeGuard socket_close_guard = [&] { close(fd); };

    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, iface.to_deprecated_string().characters(), IFNAMSIZ) == -1) {
        outln("ERROR: setsockopt(SO_BROADCAST) :: {}", strerror(errno));
        return false;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, iface.to_deprecated_string().characters(), IFNAMSIZ) == -1) {
        outln("ERROR: setsockopt(SO_BINDTODEVICE) :: {}", strerror(errno));
        return false;
    }

    sockaddr_in dst;
    memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_port = htons(67);
    dst.sin_addr.s_addr = IPv4Address { 255, 255, 255, 255 }.to_u32();
    memset(&dst.sin_zero, 0, sizeof(dst.sin_zero));

    auto rc = sendto(fd, &packet, sizeof(packet), 0, (sockaddr*)&dst, sizeof(dst));

    if (rc < 0) {
        outln("sendto failed with {}", strerror(errno));
        return false;
    }

    return true;
}

ErrorOr<NonnullRefPtr<DhcpClient>> DhcpClient::create()
{
    return adopt_ref(*new DhcpClient);
}

DhcpClient::DhcpClient()
{
    m_server = MUST(Event::UdpServer::create());

    m_server->on_ready = [&] {
        auto data_or_error = m_server->receive_sized(sizeof(DhcpPacket));
        if (data_or_error.is_error()) {
            outln("failed to receive dhcp packet: {}", data_or_error.error());
            return;
        }

        auto data = data_or_error.release_value();
        auto* packet = (DhcpPacket*)data.data();

        auto options = packet->parse_options();

        auto message_type_or_error = options.get<DHCPMessageType>(DHCPOption::DHCPMessageType);
        if (!message_type_or_error.has_value())
            return;

        auto message_type = message_type_or_error.value();

        switch (message_type) {
        case DHCPMessageType::DHCPOffer:
            MUST(handle_offer(*packet, options));
            break;
        case DHCPMessageType::DHCPAck:
            MUST(handle_ack(*packet, options));
            break;
        case DHCPMessageType::DHCPDecline:
        case DHCPMessageType::DHCPDiscover:
        case DHCPMessageType::DHCPNak:
        case DHCPMessageType::DHCPRelease:
        case DHCPMessageType::DHCPRequest:
            outln("unimplemented: {}", (u8)message_type);
            break;
        }
    };

    MUST(m_server->bind({}, 68));
}

DhcpClient::~DhcpClient()
{
}

ErrorOr<bool> DhcpClient::handle_offer(DhcpPacket const& packet, ParsedDhcpOptions const& options)
{
    auto* transaction = const_cast<DhcpTransaction*>(m_ongoing_transactions.get(packet.xid()).value_or(nullptr));

    if (!transaction) {
        // We were offered data on a interface/ip we did not request.
        outln("handle_offer: !transaction");
        return false;
    }
    if (transaction->has_ip) {
        // We already have an ip and are just waiting for an ack.
        outln("handle_offer: has_ip");
        return false;
    }

    if (transaction->accepted_offer) {
        // We have been offered someone else's offer, but we have not got an ack.
        outln("handle_offer: accepted offer");
        return false;
    }

    transaction->offered_lease_time = options.get<u32>(DHCPOption::IPAddressLeaseTime).value();
    TRY(handle_request(packet, *transaction));

    return true;
}

ErrorOr<bool> DhcpClient::handle_request(DhcpPacket const& packet, DhcpTransaction transaction)
{
    DHCPv4PacketBuilder builder;

    DhcpPacket& outgoing = builder.peek();
    outgoing.set_op(DHCPv4Op::BootRequest);
    outgoing.set_htype(1); // 10mb ethernet
    outgoing.set_hlen(sizeof(MACAddress));
    outgoing.set_xid(transaction.id);
    outgoing.set_flags(DHCPv4Flags::Broadcast);
    outgoing.ciaddr() = transaction.interface->ipv4_addr();
    outgoing.set_chaddr(transaction.interface->mac_addr());
    outgoing.set_secs(65535); // we lie

    builder.set_message_type(DHCPMessageType::DHCPRequest);
    builder.add_option(DHCPOption::RequestedIPAddress, sizeof(IPv4Address), &packet.yiaddr());

    auto maybe_dhcp_server_ip = packet.parse_options().get<IPv4Address>(DHCPOption::ServerIdentifier);
    if (maybe_dhcp_server_ip.has_value())
        builder.add_option(DHCPOption::ServerIdentifier, sizeof(IPv4Address), &maybe_dhcp_server_ip.value());

    AK::Array<DHCPOption, 2> parameter_request_list = {
        DHCPOption::SubnetMask,
        DHCPOption::Router,
    };

    builder.add_option(DHCPOption::ParameterRequestList, parameter_request_list.size(), &parameter_request_list);

    auto& dhcp_packet = builder.build();

    if (!send(transaction.interface->name(), dhcp_packet))
        return false;

    transaction.accepted_offer = true;

    return true;
}

ErrorOr<bool> DhcpClient::handle_ack(DhcpPacket const& packet, ParsedDhcpOptions const& options)
{
    auto* transaction = m_ongoing_transactions.get(packet.xid()).value_or(nullptr);
    if (!transaction) {
        // We are not looking for the provided packet/ack/transaction;
        return false;
    }
    transaction->has_ip = true;
    auto interface = transaction->interface;
    auto new_ip = packet.yiaddr();
    auto lease_time = AK::convert_between_host_and_network_endian(options.get<u32>(DHCPOption::IPAddressLeaseTime).value_or(transaction->offered_lease_time));

    // FIXME: We should try to renew our lease.
    auto timer = TRY(Event::Timer::create_single_shot(Time::from_seconds(lease_time)));
    timer->on_action = [&] {
    };

    Optional<IPv4Address> gateway;
    auto router = options.get_many<IPv4Address>(DHCPOption::Router, 1);
    if (!router.is_empty())
        gateway = router.first();

    TRY(Interface::set_ipv4_addr(interface->name(), new_ip));
    TRY(Interface::set_ipv4_mask(interface->name(), options.get<IPv4Address>(DHCPOption::SubnetMask).value()));

    return true;
}

ErrorOr<bool> DhcpClient::discover_for_iface(RefPtr<Interface> const& iface)
{
    auto transaction_id = get_random<u32>();

    DHCPv4PacketBuilder builder;

    DhcpPacket& packet = builder.peek();
    packet.set_op(DHCPv4Op::BootRequest);
    packet.set_htype(1); // 10mb ethernet
    packet.set_hlen(sizeof(MACAddress));
    packet.set_xid(transaction_id);
    packet.set_flags(DHCPv4Flags::Broadcast);
    packet.ciaddr() = iface->ipv4_addr();
    packet.set_chaddr(iface->mac_addr());
    packet.set_secs(65535); // we lie

    builder.set_message_type(DHCPMessageType::DHCPDiscover);
    auto& dhcp_packet = builder.build();

    if (!send(iface->name(), dhcp_packet))
        return false;
    m_ongoing_transactions.set(transaction_id, make<DhcpTransaction>(iface, transaction_id));
    return true;
};
