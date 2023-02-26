#pragma once

#include <AK/RefCounted.h>
#include <AK/Vector.h>

#include <LibEvent/TcpServer.h>

class Server : public RefCounted<Server> {
public:
    static ErrorOr<NonnullRefPtr<Server>> create();

    // NOTE: setup will bind and listen for connections.
    ErrorOr<bool> setup();

    void new_connection(NonnullRefPtr<Event::TcpStream>);

    Server(Vector<NonnullRefPtr<Event::TcpServer>>);
    ~Server();

private:
    Vector<NonnullRefPtr<Event::TcpServer>> m_servers;
};