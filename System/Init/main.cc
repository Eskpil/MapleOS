#include <AK/Format.h>

#include <LibEvent/EventLoop.h>

#include "Essentials.h"
#include "GroupFile.h"
#include "PasswdFile.h"
#include "Targets.h"

// FIXME: Avoid returning from this function because of errors
// Instead perform retry attempts or let be. When the init system
// dies the kernel does as well. That is the last thing we want.
int main(void)
{
    auto event_loop = MUST(Event::EventLoop::create());

    if (Essentials::load().is_error()) {
        outln("Failed to load the essential directories");
        return 1;
    }

    auto group_file = GroupFile::open_or_create().release_value();
    auto passwd_file = PasswdFile::open_or_create().release_value();

    auto targets_or_error = Targets::create();
    if (targets_or_error.is_error()) {
        outln("Could not create targets: {}", targets_or_error.release_error());
        return 1;
    }
    auto targets = targets_or_error.release_value();

    auto success_or_error = targets->load_directory(LexicalPath("/etc/services"));
    if (success_or_error.is_error()) {
        outln("Could not load directory: {}", success_or_error.release_error());
        return 1;
    }

    auto execution_order_or_error = targets->create_execution_order();
    if (execution_order_or_error.is_error()) {
        outln("Could not create an execution order: {}", execution_order_or_error.release_error());
        return 1;
    }
    auto execution_order = execution_order_or_error.release_value();

    execution_order->debug_dump();

    for (auto const& service : execution_order->order()) {
        success_or_error = service->start();
        if (success_or_error.is_error()) {
            outln("Failed to start service: {}: {}", service->name(), success_or_error.error());
            continue;
        }
    }

    return event_loop->exec();
}