#pragma  once

#include <AK/RefCounted.h>
#include <AK/String.h>

#include "Target.h"

Vector<RefPtr<Service>> filter_stage(HashMap<String, NonnullRefPtr<Target>> const&, RefPtr<Target> const&, bool const&);

class ExecutionOrder : public RefCounted<ExecutionOrder> {
public:
    explicit ExecutionOrder(HashMap<String, NonnullRefPtr<Target>>&);

    ErrorOr<bool> generate_debug_graph(LexicalPath const&);
    void debug_dump();

    Vector<RefPtr<Service>> order() const { return m_order; }

    ~ExecutionOrder();
private:
    RefPtr<Target> m_staged_target;

    Vector<RefPtr<Service>> m_order;
};