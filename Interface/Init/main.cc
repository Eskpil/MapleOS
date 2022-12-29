#include <AK/LexicalPath.h>
#include <AK/Vector.h>

#include <LibEvent/EventLoop.h>
#include <LibEvent/Signals.h>
#include <LibEvent/TcpServer.h>
#include <LibEvent/TcpStream.h>

int main(void)
{
    auto event_loop = MUST(Event::EventLoop::create());

    auto tcp_server = MUST(Event::TcpServer::create(Event::SocketType::IPv4));
    // MUST(tcp_server->bind(LexicalPath("/run/interface.sock")));

    auto success_or_error = tcp_server->bind(IPv4Address { 127, 0, 0, 1 }, 8090);
    if (success_or_error.is_error()) {
        outln("bind: {}", success_or_error.error());
        return 1;
    }

    auto streams = Vector<RefPtr<Event::TcpStream>>();

    tcp_server->on_connection = [&](RefPtr<Event::TcpStream> stream) {
        stream->on_ready = [stream] {
            auto data_or_error = stream->read();
            if (data_or_error.is_error()) {
                outln("failed to read: {}", data_or_error.error());
                return;
            }
            auto data = data_or_error.release_value();
            MUST(stream->write(data));

            return;
        };

        streams.append(stream);
    };

    auto sigint_handler = MUST(Event::SignalHandler::create(Event::Signal::INT));

    sigint_handler->on_action = [&] {
        MUST(tcp_server->close());

        exit(1);
    };

    MUST(tcp_server->listen());

    return event_loop->exec();
}