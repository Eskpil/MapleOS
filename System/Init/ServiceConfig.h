#pragma once

#include <AK/JsonValue.h>
#include <AK/String.h>
#include <AK/Vector.h>

// {
//     "name": "resolved",
//     "bin": "/sbin/resolved",
//     "user": "resolve",
//     "group": "resolve",
//     "fulfills": "dns",
//     "needs_targets": []
// }


class ServiceConfig {
public:
    explicit ServiceConfig(JsonValue);

    [[nodiscard]] String name() const { return m_name; }
    [[nodiscard]] String bin() const { return m_bin; }
    [[nodiscard]] String user() const { return m_user; }
    [[nodiscard]] String group() const { return m_group; }
    [[nodiscard]] String fulfills() const { return m_fulfills; }
    [[nodiscard]] Vector<String> environment() const { return m_environment; }
    [[nodiscard]] Vector<String> arguments() const { return m_arguments; }

private:
    String m_name;
    String m_bin;
    String m_user;
    String m_group;
    String m_fulfills;

    Vector<String> m_environment;
    Vector<String> m_arguments;
};