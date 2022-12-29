/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Endian.h>
#include <AK/Format.h>
#include <AK/Optional.h>
#include <AK/StringView.h>
#include <AK/Vector.h>
#include <AK/Error.h>

#include <AK/String.h>

namespace AK {

class [[gnu::packed]] IPv4Address {
    enum class SubnetClass : int {
        A = 0,
        B,
        C,
        D
    };

public:
    using in_addr_t = u32;

    constexpr IPv4Address() = default;

    constexpr IPv4Address(u32 a, u32 b, u32 c, u32 d)
    {
        m_data = (d << 24) | (c << 16) | (b << 8) | a;
    }

    constexpr IPv4Address(const u8 data[4])
    {
        m_data = (u32(data[3]) << 24) | (u32(data[2]) << 16) | (u32(data[1]) << 8) | u32(data[0]);
    }

    constexpr IPv4Address(NetworkOrdered<u32> address)
        : m_data(address)
    {
    }

    constexpr u8 operator[](int i) const
    {
        VERIFY(i >= 0 && i < 4);
        return octet(SubnetClass(i));
    }

    ErrorOr<String> to_string() const
    {
        return String::formatted("{}.{}.{}.{}",
            octet(SubnetClass::A),
            octet(SubnetClass::B),
            octet(SubnetClass::C),
            octet(SubnetClass::D));
    }

    ErrorOr<String> to_string_reversed() const
    {
        return String::formatted("{}.{}.{}.{}",
            octet(SubnetClass::D),
            octet(SubnetClass::C),
            octet(SubnetClass::B),
            octet(SubnetClass::A));
    }

    static Optional<IPv4Address> local()
    {
        return IPv4Address(127, 0, 0, 1);
    }

    static Optional<IPv4Address> empty()
    {
        return IPv4Address(0, 0, 0, 0);
    }

    static Optional<IPv4Address> from_string(StringView string)
    {
        if (string.is_null())
            return {};

        auto const parts = string.split_view('.');

        u32 a {};
        u32 b {};
        u32 c {};
        u32 d {};

        if (parts.size() == 1) {
            d = parts[0].to_uint().value_or(256);
        } else if (parts.size() == 2) {
            a = parts[0].to_uint().value_or(256);
            d = parts[1].to_uint().value_or(256);
        } else if (parts.size() == 3) {
            a = parts[0].to_uint().value_or(256);
            b = parts[1].to_uint().value_or(256);
            d = parts[2].to_uint().value_or(256);
        } else if (parts.size() == 4) {
            a = parts[0].to_uint().value_or(256);
            b = parts[1].to_uint().value_or(256);
            c = parts[2].to_uint().value_or(256);
            d = parts[3].to_uint().value_or(256);
        } else {
            return {};
        }

        if (a > 255 || b > 255 || c > 255 || d > 255)
            return {};
        return IPv4Address(a, b, c, d);
    }

    static constexpr IPv4Address netmask_from_cidr(int cidr)
    {
        VERIFY(cidr >= 0 && cidr <= 32);
        u32 value = 0xffffffffull << (32 - cidr);
        return IPv4Address((value & 0xff000000) >> 24, (value & 0xff0000) >> 16, (value & 0xff00) >> 8, (value & 0xff));
    }

    constexpr in_addr_t to_in_addr_t() const { return m_data; }
    constexpr u32 to_u32() const { return m_data; }

    constexpr bool operator==(IPv4Address const& other) const = default;
    constexpr bool operator!=(IPv4Address const& other) const = default;

    constexpr bool is_zero() const
    {
        return m_data == 0u;
    }

private:
    constexpr u32 octet(const SubnetClass subnet) const
    {
        NetworkOrdered<u32> address(m_data);
        constexpr auto bits_per_byte = 8;
        auto const bits_to_shift = bits_per_byte * int(subnet);
        return (m_data >> bits_to_shift) & 0x0000'00FF;
    }

    u32 m_data {};
};

static_assert(sizeof(IPv4Address) == 4);

template<>
struct Traits<IPv4Address> : public GenericTraits<IPv4Address> {
    static constexpr unsigned hash(IPv4Address const& address) { return int_hash(address.to_u32()); }
};

template<>
struct Formatter<IPv4Address> : Formatter<String> {
    ErrorOr<void> format(FormatBuilder& builder, IPv4Address value)
    {
        return Formatter<String>::format(builder, MUST(value.to_string()));
    }
};

}

#ifdef USING_AK_GLOBALLY
using AK::IPv4Address;
#endif