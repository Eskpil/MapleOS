#pragma once

#include <AK/Error.h>
#include <AK/HashMap.h>
#include <AK/OwnPtr.h>
#include <AK/RefCounted.h>
#include <AK/RefPtr.h>
#include <AK/Types.h>
#include <AK/Weakable.h>

#include "EventLoop.h"

namespace Event {

enum class Signal : int {
    HUP = 1,
    INT = 2,
    QUIT = 3,
    TRAP = 4,
    ABRT = 5,
    IOT = 6,
    BUS = 7,
    FPE = 8,
    KILL = 9,
    USR1 = 10,
    SEGV = 11,
    USR2 = 12,
    PIPE = 13,
    ALRM = 14,
    TERM = 15,
    STKFT = 16,
    CHLD = 17,
    CONT = 18,

    STOP = 19,
    TSTP = 20,
    TTIN = 21,
    TTOU = 22,
    URG = 23,
    XCPU = 24,
    XFSZ = 25,
    VTALRM = 26,
    PROF = 27,
    WICH = 28,
    IO = 29,
    PWR = 30,
    SYS = 32,
    UNUSED = 31,
};

class SignalHandler : public RefCounted<SignalHandler> {
public:
    static ErrorOr<NonnullRefPtr<SignalHandler>> create(Signal);

    explicit SignalHandler() = default;

    Function<void()> on_action;
};

class Signals : public Weakable<Signals>
    , public EventHandler {
    friend SignalHandler;
    friend EventLoop;

public:
    ~Signals() override;

private:
    static ErrorOr<OwnPtr<Signals>> create();
    explicit Signals();

    static Signals* the();

    void handle_event(EventLoop::Event const&) override;
    EventLoop::Event event() override;
    void close() override;

    void handle(Signal, RefPtr<SignalHandler>);

    HashMap<Signal, RefPtr<SignalHandler>> m_handlers;

    int m_signalfd { -1 };
};

}