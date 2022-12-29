#pragma once

#include <AK/RefCounted.h>
#include <AK/NonnullRefPtr.h>
#include <AK/Error.h>
#include <AK/LexicalPath.h>
#include <AK/HashMap.h>
#include <AK/Vector.h>

#include "TargetConfig.h"
#include "Service.h"

class Target : public RefCounted<Target> {
public:
    ErrorOr<NonnullRefPtr<Target>> static create(LexicalPath const&);

    [[nodiscard]] String name() const { return m_config->name(); }
    [[nodiscard]] Vector<String> fulfills() const { return m_config->fulfills(); }
    [[nodiscard]] Vector<String> needs() const { return m_config->needs(); }
    [[nodiscard]] Vector<String> allows() const { return m_config->allows(); }
    [[nodiscard]] bool staged() const { return m_config->staged(); }

    ErrorOr<bool> append(NonnullRefPtr<Service>);
    HashMap<String, NonnullRefPtr<Service>> services() { return m_services; }

    void dump(StringView indent);

    explicit Target(TargetConfig*);
    ~Target();
private:
    HashMap<String, NonnullRefPtr<Service>> m_services;
    TargetConfig *m_config;
};