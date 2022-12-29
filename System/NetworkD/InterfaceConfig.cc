#include <AK/JsonObject.h>

#include "InterfaceConfig.h"

InterfaceConfig::InterfaceConfig(bool) : m_empty(true) {}

InterfaceConfig::InterfaceConfig(JsonValue const& value) {
    m_empty = false;

    VERIFY(value.is_object());
    auto const& object = value.as_object();

    if (object.has("ipv4_address"sv)) {
        m_has_configured_ipv4_address = true;
        m_configured_ipv4_address = IPv4Address::from_string(object.get("ipv4_address"sv).as_string()).release_value();
    }

    if (object.has("ipv6_address"sv)) {
        m_has_configured_ipv6_address = true;
        m_configured_ipv6_address = IPv6Address::from_string(object.get("ipv6_address"sv).as_string()).release_value();
    }

    if (object.has("dhcp"sv)) {
        if ((!m_has_configured_ipv4_address) && object.get("dhcp"sv).as_bool())
            m_requests_dhcp = false;
        m_requests_dhcp = object.get("dhcp"sv).as_bool();
    }

    if (object.has("dhcp_identifier"sv)) {
        m_requested_dhcp_identifier = MUST(String::from_deprecated_string(object.get("dhcp_identifier"sv).as_string()));
    } else {
        m_requested_dhcp_identifier = MUST(String::formatted("mac"));
    }

    if (object.has("multicast"sv)) {
        m_multicast = object.get("multicast"sv).as_bool();
    }
}

InterfaceConfig::~InterfaceConfig() {

}
