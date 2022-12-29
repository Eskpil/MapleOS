#pragma once

#include <AK/IPv4Address.h>
#include <AK/IPv6Address.h>
#include <AK/JsonValue.h>

class InterfaceConfig {
public:
    explicit InterfaceConfig(JsonValue const&);
    explicit InterfaceConfig(bool);

    static InterfaceConfig empty() {
        return InterfaceConfig(true);
    }

    [[nodiscard]] bool has_configured_ipv4_address() const { return m_has_configured_ipv4_address; }
    [[nodiscard]] bool has_configured_ipv6_address() const { return m_has_configured_ipv6_address; }

    [[nodiscard]] IPv4Address configured_ipv4_address() const { return m_configured_ipv4_address; }
    [[nodiscard]] IPv6Address configured_ipv6_address() const { return m_configured_ipv6_address; }

    [[nodiscard]] bool has_requested_dhcp() const { return m_requests_dhcp; }
    [[nodiscard]] String requested_dhcp_identifier() const { return m_requested_dhcp_identifier; }

    [[nodiscard]] bool has_requested_mutlicast() const { return m_multicast; }

    [[nodiscard]] bool is_empty() const { return m_empty; }

    ~InterfaceConfig();
private:
    bool m_has_configured_ipv4_address;
    IPv4Address m_configured_ipv4_address;

    bool m_has_configured_ipv6_address;
    IPv6Address m_configured_ipv6_address;

    bool m_requests_dhcp { true };
    String m_requested_dhcp_identifier;

    bool m_multicast { true };

    bool m_empty { true };
};