#pragma once

#include <AK/Vector.h>
#include <AK/RefCounted.h>
#include <AK/NonnullRefPtr.h>
#include <AK/StringView.h>
#include <AK/LexicalPath.h>

#include "ServiceConfig.h"

class Service : public RefCounted<Service> {
public:
    ErrorOr<NonnullRefPtr<Service>> static create(LexicalPath const&);

    ErrorOr<bool> start();

    [[nodiscard]] String name() const { return m_config->name(); }
    [[nodiscard]] String user() const { return m_config->user(); }
    [[nodiscard]] String group() const { return m_config->group(); }
    [[nodiscard]] String bin() const { return m_config->bin(); }
    [[nodiscard]] String fulfills() const { return m_config->fulfills(); }
    [[nodiscard]] Vector<String> environment() const { return m_config->environment(); }
    [[nodiscard]] Vector<String> arguments() const { return m_config->arguments(); }

    ~Service();
protected:
    explicit Service(ServiceConfig*);
private:
    ErrorOr<bool> create_user();
    ErrorOr<bool> create_group();

    ServiceConfig *m_config;
};