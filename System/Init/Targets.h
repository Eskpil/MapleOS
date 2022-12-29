#pragma once

#include <AK/HashMap.h>
#include <AK/Vector.h>
#include <AK/RefCounted.h>
#include <AK/NonnullRefPtr.h>

#include "Service.h"
#include "Target.h"
#include "ExecutionOrder.h"

class Targets : public RefCounted<Targets> {
public:
    ErrorOr<NonnullRefPtr<Targets>> static create();

    ErrorOr<bool> load_directory(LexicalPath const&);
    void dump();

    ErrorOr<NonnullRefPtr<ExecutionOrder>> create_execution_order();

    Targets();
    ~Targets();
private:
    HashMap<String, NonnullRefPtr<Target>> m_targets;

    ErrorOr<bool> append(NonnullRefPtr<Target>);
};