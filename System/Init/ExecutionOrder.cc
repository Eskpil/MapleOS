#include <AK/NonnullRefPtr.h>

#include "Service.h"
#include "ExecutionOrder.h"
#include "Target.h"

Vector<RefPtr<Service>> filter_stage(HashMap<String, NonnullRefPtr<Target>> const& targets, RefPtr<Target> const& staged_target, bool const& ignore_needs) {
    Vector<RefPtr<Service>> services;

    if (!ignore_needs) {
        for (auto const &target_need: staged_target->needs()) {
            for (auto const &target: targets) {
                if (target.key == staged_target->name()) continue;
                if (target.key != target_need) continue;

                auto result = filter_stage(targets, target.value, false);
                if (result.size() == 0) continue;
                for (auto service: result) {
                    MUST(services.try_append(move(service)));
                }
            }
        }
    }

    // It is important we keep the order specified in the target file.
    for (auto const& fulfillment_need : staged_target->fulfills()) {
        auto service = staged_target->services().get(fulfillment_need);
        if (!service.has_value()) {
            outln("No service fulfilling: {}", fulfillment_need);
            continue;
        }
        MUST(services.try_append(service.release_value()));
    }

    for(auto const& target_allow : staged_target->allows()) {
        for (auto const &target: targets) {
            if (target.key == staged_target->name()) continue;
            if (target.key != target_allow) continue;

            auto result = filter_stage(targets, target.value, true);
            if (result.size() == 0) continue;
            for (auto service: result) {
                MUST(services.try_append(move(service)));
            }
        }
    }

    return services;
}

ExecutionOrder::ExecutionOrder(HashMap<String, NonnullRefPtr<Target>>& targets) {
    for (auto target : targets) {
        if (target.value->staged()) {
            m_staged_target = move(target.value);
        }
    }
    VERIFY(m_staged_target != nullptr);

    m_order = filter_stage(targets, m_staged_target, false);
}

void ExecutionOrder::debug_dump() {
    out("[");
    // for (size_t i = 0; m_order.size() > i; ++i) {
    //     auto service = m_order.take(i);
    //     auto is_last = m_order.size() - 1 == i;
    //     out("{}", service->name());
    //     if (!is_last) {
    //         out(" ");
    //     }
    // }

    for (auto const& service : m_order) {
        out("{}, ", service->name());
    }
    outln("]");
}

ExecutionOrder::~ExecutionOrder() {}

ErrorOr<bool> ExecutionOrder::generate_debug_graph(const LexicalPath &) {
    VERIFY_NOT_REACHED();
    return true;
}
