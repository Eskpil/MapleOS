#pragma once

#include <AK/NonnullRefPtr.h>
#include <AK/RefCounted.h>
#include <AK/Time.h>

#include "EventLoop.h"

namespace Event {

class Timer : public RefCounted<Timer>
    , EventHandler {
public:
    enum Kind : u8 {
        SingleShot,
        Repeating,
    };

    static ErrorOr<NonnullRefPtr<Timer>> create_single_shot(Time const&);
    static ErrorOr<NonnullRefPtr<Timer>> create_repeating(Time const&, bool instant = true);

    explicit Timer(Kind const&, Time const&, int const&);

    Function<void()> on_action;

    ~Timer() override;

protected:
private:
    void handle_event(EventLoop::Event const&) override;
    EventLoop::Event event() override;
    void close() override;

    Kind m_kind;
    int m_timerfd;
    Time m_time;
};

}