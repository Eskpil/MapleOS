#pragma once

#include <AK/Error.h>
#include <AK/RefCounted.h>

#include "EventLoop.h"

namespace Event {

class Pidfd : public RefCounted<Pidfd>
    , EventHandler {

public:
    static ErrorOr<NonnullRefPtr<Pidfd>> open(u32);

    explicit Pidfd(int);

    ErrorOr<int> getfd(int);

    ~Pidfd() override;

private:
    void handle_event(EventLoop::Event const&) override;
    EventLoop::Event event() override;
    void close() override;

    int m_pidfd;
};

}