#include <AK/NonnullRefPtr.h>
#include <AK/ScopeGuard.h>
#include <AK/Time.h>

#include <LibEvent/EventLoop.h>
#include <LibEvent/Timer.h>

#include <stdio.h>
#include <sys/stat.h>

#include "DhcpClient.h"
#include "Interface.h"

ErrorOr<bool> ensure_resolv_conf(IPv4Address const& address)
{
    struct stat st {
        0
    };

    auto uid = getuid();
    auto gid = getgid();

    setuid(0);
    setgid(0);

    ScopeGuard reset_permissions = ScopeGuard([&] {
        setuid(uid);
        setgid(gid);
    });

    auto f = fopen("/etc/resolv.conf", "w+");
    auto fd = fileno(f);

    auto write_line = [&]() -> ErrorOr<bool> {
        auto line = TRY(String::formatted("nameserver {}\n", address.to_string().release_value()));
        fprintf(f, "%s", line.to_deprecated_string().characters());
        return true;
    };

    if (fstat(fd, &st) == -1) {
        TRY(write_line());
    }

    if (st.st_size == 0) {
        TRY(write_line());
    }

    return true;
}

int main(void)
{
    auto event_loop = MUST(Event::EventLoop::create());

    auto interfaces_or_error = Interface::iterate();
    if (interfaces_or_error.is_error()) {
        outln("Failed to initiate all interfaces: {}", interfaces_or_error.error());
        return 1;
    }

    auto interfaces = interfaces_or_error.release_value();
    auto dhcp_client = MUST(DhcpClient::create());

    for (auto const& interface : interfaces) {
        MUST(Interface::start(interface->name()));
    }

    // Starting could take some time, so we will wait 1 second
    // before we try to get its ip and netmask with DHCP.
    auto timer = MUST(Event::Timer::create_single_shot(Time::from_seconds(1)));
    timer->on_action = [&] {
        for (auto const& interface : interfaces) {
            if (interface->needs_dhcp()) {
                auto success_or_error = dhcp_client->discover_for_iface(interface);
                if (success_or_error.is_error()) {
                    outln("failed to send a discover for iface: ", interface->name());
                    continue;
                }
            }
        }
    };

    // FIXME: This will and should be much better with the introduction of a service strictly
    // in charge of handling resolution of dns records. With the introduction of a custom service
    // we should also use the DNS server provided by our router.

    // NOTE: This uses cloudflare's public dns.
    MUST(ensure_resolv_conf(IPv4Address { 1, 1, 1, 1 }));

    return event_loop->exec();
}