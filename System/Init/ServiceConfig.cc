#include <AK/JsonObject.h>
#include <AK/Error.h>

#include "ServiceConfig.h"

// {
//     "name": "resolved",
//     "bin": "/sbin/resolved",
//     "user": "resolve",
//     "group": "resolve",
//     "fulfills": "dns",
//     "needs_targets": []
// }

ServiceConfig::ServiceConfig(JsonValue value) {
    auto object = value.as_object();

    m_arguments = Vector<String>();
    m_environment = Vector<String>();

    m_name = MUST(String::from_deprecated_string(object.get("name"sv).as_string()));
    m_bin = MUST(String::from_deprecated_string(object.get("bin"sv).as_string()));
    m_user = MUST(String::from_deprecated_string(object.get("user"sv).as_string()));
    m_group = MUST(String::from_deprecated_string(object.get("group"sv).as_string()));
    m_fulfills = MUST(String::from_deprecated_string(object.get("fulfills"sv).as_string()));

    if (object.has("arguments"sv)) {
        object.get("arguments"sv).as_array().for_each([&](JsonValue const& value) {
            MUST(m_arguments.try_append(MUST(String::from_deprecated_string(value.as_string()))));
        });
    }

    if (object.has("environment"sv)) {
        object.get("environment"sv).as_array().for_each([&](JsonValue const& value){
            MUST(m_environment.try_append(MUST(String::from_deprecated_string(value.as_string()))));
        });
    }
}