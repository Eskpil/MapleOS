#pragma once

#include <AK/JsonValue.h>
#include <AK/String.h>
#include <AK/Vector.h>

//{
//    "name": "dbus",
//    "staged": false,
//    "needs": [],
//    "fulfills": ["dbus-daemon"],
//    "allows": []
//}

class TargetConfig {
public:
    TargetConfig(JsonValue const&);

    [[nodiscard]] String name() const { return m_name; }
    [[nodiscard]] bool staged() const { return m_staged; }
    [[nodiscard]] Vector<String> needs() const { return m_needs; }
    [[nodiscard]] Vector<String> fulfills() const { return m_fulfills;}
    [[nodiscard]] Vector<String> allows() const { return m_allows; }

private:
    String m_name;
    bool m_staged;
    Vector<String> m_needs;
    Vector<String> m_fulfills;
    Vector<String> m_allows;
};
