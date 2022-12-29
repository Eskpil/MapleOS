#pragma once

#include <AK/Error.h>
#include <AK/IPv4Address.h>
#include <AK/IPv6Address.h>
#include <AK/LexicalPath.h>
#include <AK/MACAddress.h>
#include <AK/Optional.h>
#include <AK/OwnPtr.h>
#include <AK/RefCounted.h>
#include <AK/String.h>
#include <AK/StringView.h>
#include <AK/Vector.h>

#include "InterfaceConfig.h"

// FIXME: Implement ipv6
class Interface : public RefCounted<Interface> {
public:
    static ErrorOr<RefPtr<Interface>> create(StringView const&, LexicalPath const&);
    static ErrorOr<Vector<RefPtr<Interface>>> iterate();

    static Optional<IPv4Address> get_interface_ipv4_addr(StringView const&);
    static Optional<IPv4Address> get_interface_ipv4_mask(StringView const&);
    static Optional<MACAddress> get_interface_mac_addr(StringView const&);
    static Optional<IPv6Address> get_interface_ipv6_addr(StringView const&);

    explicit Interface(StringView const&, LexicalPath const&, IPv4Address const&, IPv4Address const&, MACAddress const&);

    static ErrorOr<bool> set_ipv4_addr(StringView const&, IPv4Address const&);
    static ErrorOr<bool> set_ipv4_mask(StringView const&, IPv4Address const&);

    static ErrorOr<bool> start(StringView const&);

    IPv4Address ipv4_addr() const { return m_ipv4_addr; }
    MACAddress mac_addr() const { return m_mac_addr; }

    bool is_virtual() const { return m_is_virtual; }
    bool is_loopback() const { return m_is_loopback; }

    String name() const& { return m_name; };

    bool has_config() const { return m_config.is_empty(); }
    bool needs_dhcp() const;

    void set_handled_by_dhcp(bool is_handled) { m_is_handled_by_dhcp = is_handled; }
    bool is_handled_by_dhcp() const { return m_is_handled_by_dhcp; }

    void set_dns_server(IPv4Address const& server) { m_dns_server = server; }
    IPv4Address dns_server() const { return m_dns_server; }

    ~Interface();

private:
    InterfaceConfig load_config();

    String m_name;
    LexicalPath m_path;

    IPv4Address m_ipv4_addr;
    IPv4Address m_netmask;

    MACAddress m_mac_addr;

    bool m_is_virtual;
    bool m_is_loopback { false };

    bool m_is_handled_by_dhcp { false };

    IPv4Address m_dns_server { 1, 1, 1, 1 };

    InterfaceConfig m_config;
};