#include <AK/Error.h>
#include <AK/HashMap.h>
#include <AK/IPv4Address.h>
#include <AK/JsonParser.h>
#include <AK/NeverDestroyed.h>
#include <AK/NonnullRefPtr.h>
#include <AK/ScopeGuard.h>
#include <AK/Vector.h>

#include <dirent.h>
#include <sys/stat.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "Interface.h"

static NeverDestroyed<HashMap<StringView, bool>> s_started_ifaces;

ErrorOr<Vector<RefPtr<Interface>>> Interface::iterate()
{
    auto interfaces = Vector<RefPtr<Interface>>();

    struct dirent* dp;
    DIR* dfd;

    if ((dfd = opendir("/sys/class/net")) == nullptr) {
        return Error::from_errno(errno);
    };

    ScopeGuard close_dir_guard = ScopeGuard([&] { closedir(dfd); });

    while ((dp = readdir(dfd)) != nullptr) {
        struct stat st {
            0
        };

        auto name = StringView(dp->d_name, strlen(dp->d_name));
        auto link = LexicalPath("/sys/class/net").append(name).string().characters();
        if (lstat(link, &st) == -1) {
            continue;
        }

        // If it is a symlink we know it must be a network interface.
        if (S_ISLNK(st.st_mode)) {
            char* raw_path = (char*)malloc(PATH_MAX);
            if (realpath(link, raw_path) == nullptr) {
                return Error::from_errno(errno);
            }

            auto path = LexicalPath(StringView(raw_path, strlen(raw_path)));
            auto interface = TRY(Interface::create(name, path));

            TRY(interfaces.try_append(interface));
        }
    };

    return interfaces;
}

Optional<MACAddress> Interface::get_interface_mac_addr(StringView const& if_name)
{
    auto path = LexicalPath("/sys/class/net/").append(if_name).append("/address"sv);

    auto f = fopen(path.string().characters(), "rb");
    if (f == nullptr) {
        return MACAddress::empty();
    }

    ScopeGuard close_file_guard = ScopeGuard([&] { fclose(f); });

    auto c = getc(f);
    if (c == '\n') {
        return MACAddress::empty();
    }

    auto raw_mac_address = (char*)malloc(MAC_ADDRESS_LENGTH);
    auto nread = fread(raw_mac_address, 1, MAC_ADDRESS_LENGTH, f);
    if (nread != MAC_ADDRESS_LENGTH) {
        if (errno != EINVAL) {
            return MACAddress::empty();
        }
    }

    return MACAddress::from_string(StringView(raw_mac_address, MAC_ADDRESS_LENGTH));
}

Optional<IPv4Address> Interface::get_interface_ipv4_addr(StringView const& if_name)
{
    struct ifreq ifr {
        0
    };
    ifr.ifr_addr.sa_family = AF_INET;

    bool fits = if_name.copy_characters_to_buffer(ifr.ifr_name, IFNAMSIZ);
    if (!fits)
        return IPv4Address::empty();

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);

    auto raw_ipv4_address = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr);
    auto ipv4 = IPv4Address::from_string(StringView(raw_ipv4_address, strlen(raw_ipv4_address)));

    return ipv4;
}

Optional<IPv4Address> Interface::get_interface_ipv4_mask(StringView const& if_name)
{
    struct ifreq ifr {
        0
    };
    ifr.ifr_addr.sa_family = AF_INET;

    bool fits = if_name.copy_characters_to_buffer(ifr.ifr_name, IFNAMSIZ);
    if (!fits)
        return IPv4Address::empty();

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    ioctl(fd, SIOCGIFNETMASK, &ifr);
    close(fd);

    auto raw_ipv4_address = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr);
    auto ipv4 = IPv4Address::from_string(StringView(raw_ipv4_address, strlen(raw_ipv4_address)));

    return ipv4;
}

// FIXME: Support ipv6
Optional<IPv6Address> Interface::get_interface_ipv6_addr(StringView const& if_name)
{
    VERIFY_NOT_REACHED();
}

ErrorOr<RefPtr<Interface>> Interface::create(StringView const& name, LexicalPath const& path)
{
    auto ipv4 = get_interface_ipv4_addr(name);
    auto netmask = get_interface_ipv4_mask(name);
    auto mac = get_interface_mac_addr(name);

    return adopt_ref(*new Interface(name, path, ipv4.release_value(), netmask.release_value(), mac.release_value()));
}

// FIXME: Find a way to get rid of all these "MUST"
Interface::Interface(StringView const& name, LexicalPath const& path, IPv4Address const& ip, IPv4Address const& netmask,
    MACAddress const& mac)
    : m_path(path)
    , m_config(load_config())
{
    m_ipv4_addr = ip;
    m_netmask = netmask;

    m_mac_addr = mac;

    m_name = MUST(String::from_deprecated_string(name));
    m_is_virtual = path.string().contains("virtual"sv, CaseSensitivity::CaseSensitive);
    m_is_loopback = name == "lo"sv;

    // In my experiences the kernel does not set the ip address and
    // netmask of loopback, so that is something we must handle ourselves.
    if (is_loopback() && m_ipv4_addr.is_zero()) {
        auto success_or_error = Interface::set_ipv4_addr(m_name, IPv4Address { 127, 0, 0, 1 });
        if (success_or_error.is_error()) {
            outln(" > failed to set ipv4 address: {}", success_or_error.release_error());
        }

        success_or_error = Interface::set_ipv4_mask(m_name, IPv4Address { 255, 0, 0, 0 });
        if (success_or_error.is_error()) {
            outln(" > failed to set ipv4 netmask: {}", success_or_error.release_error());
        }
    }

    // Right now we do not care about virtual interfaces.
    // We don't want to mess with loopback, and we don't
    // have virtual machine interfaces to worry about.
    if (is_virtual())
        return;
    if (m_config.is_empty())
        return;

    if (m_config.has_configured_ipv4_address()) {
        MUST(Interface::set_ipv4_addr(m_name, m_config.configured_ipv4_address()));
    }
}

Interface::~Interface()
{
}

bool Interface::needs_dhcp() const
{
    if (is_loopback() || is_virtual())
        return false;

    return true;
}

InterfaceConfig Interface::load_config()
{
    auto const& name_and_extension = MUST(String::formatted("{}.interface", m_name));
    auto const& path = LexicalPath("/etc/interfaces/").append(name_and_extension);

    struct stat st {
        0
    };
    if (stat(path.string().characters(), &st) == -1)
        return InterfaceConfig::empty();

    auto f = fopen(path.string().characters(), "r");
    if (f == nullptr)
        return InterfaceConfig::empty();

    ScopeGuard close_file_guard = ScopeGuard([&] { fclose(f); });

    u8* data = (u8*)malloc(st.st_size);
    ScopeGuard free_mem_guard = ScopeGuard([&] { free(data); });

    auto nread = fread(data, 1, st.st_size, f);
    if (st.st_size != nread)
        return InterfaceConfig::empty();

    auto parser = JsonParser(StringView(data, st.st_size));
    auto value = MUST(parser.parse());

    return InterfaceConfig(value);
}

ErrorOr<bool> Interface::start(StringView const& name)
{
    if (s_started_ifaces->contains(name))
        return true;

    struct ifreq ifr {
        0
    };
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (0 > fd)
        return Error::from_errno(errno);

    ScopeGuard close_fd_guard = ScopeGuard([&] { close(fd); });

    bool fits = name.to_deprecated_string().copy_characters_to_buffer(ifr.ifr_name, IFNAMSIZ);
    if (!fits)
        return Error::from_string_view("Interface name does not fit into IFNAMSIZ"sv);

    ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) == -1)
        return Error::from_errno(errno);

    s_started_ifaces->set(name, true);

    return true;
}

ErrorOr<bool> Interface::set_ipv4_addr(StringView const& name, IPv4Address const& ip)
{
    TRY(Interface::start(name));

    struct ifreq ifr {
        0
    };
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (0 > fd)
        return Error::from_errno(errno);

    ScopeGuard close_fd_guard = ScopeGuard([&] { close(fd); });

    bool fits = name.to_deprecated_string().copy_characters_to_buffer(ifr.ifr_name, IFNAMSIZ);
    if (!fits)
        return Error::from_string_view("Interface name does not fit into IFNAMSIZ"sv);

    ifr.ifr_addr.sa_family = AF_INET;
    ((sockaddr_in&)ifr.ifr_addr).sin_addr.s_addr = ip.to_in_addr_t();

    if (ioctl(fd, SIOCSIFADDR, &ifr) < 0)
        return Error::from_errno(errno);

    return true;
}

ErrorOr<bool> Interface::set_ipv4_mask(StringView const& name, IPv4Address const& netmask)
{
    TRY(Interface::start(name));

    struct ifreq ifr {
        0
    };
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (0 > fd)
        return Error::from_errno(errno);

    ScopeGuard close_fd_guard = ScopeGuard([&] { close(fd); });

    bool fits = name.to_deprecated_string().copy_characters_to_buffer(ifr.ifr_name, IFNAMSIZ);
    if (!fits)
        return Error::from_string_view("Interface name does not fit into IFNAMSIZ"sv);

    ifr.ifr_addr.sa_family = AF_INET;
    ((sockaddr_in&)ifr.ifr_netmask).sin_addr.s_addr = netmask.to_in_addr_t();

    if (ioctl(fd, SIOCSIFNETMASK, &ifr) < 0)
        return Error::from_errno(errno);

    return true;
}