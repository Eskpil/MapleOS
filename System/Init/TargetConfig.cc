#include <AK/JsonObject.h>

#include "TargetConfig.h"

//{
//    "name": "dbus",
//    "staged": false,
//    "needs": [],
//    "fulfills": ["dbus-daemon"],
//    "allows": []
//}


TargetConfig::TargetConfig(JsonValue const& value) {
    auto object = value.as_object();

    m_needs = Vector<String>();
    m_fulfills = Vector<String>();
    m_allows = Vector<String>();

    m_name = MUST(String::from_deprecated_string(object.get("name"sv).as_string()));
    m_staged = object.get("staged"sv).as_bool();

    object.get("needs"sv).as_array().for_each([&](JsonValue const& value) {
        m_needs.append(MUST(String::from_deprecated_string(value.as_string())));
    });

    object.get("fulfills"sv).as_array().for_each([&](JsonValue const& value) {
        m_fulfills.append(MUST(String::from_deprecated_string(value.as_string())));
    });

    object.get("allows"sv).as_array().for_each([&](JsonValue const& value) {
        m_allows.append(MUST(String::from_deprecated_string(value.as_string())));
    });
}