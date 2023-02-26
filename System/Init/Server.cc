#include "Server.h"

#include <sys/stat.h>

ErrorOr<NonnullRefPtr<Server>> Server::create()
{
    Vector<NonnullRefPtr<Event::TcpServer>> servers;

    {
        struct stat st {
            0
        };

        if (stat("/run/initd", &st) > 0) {
            remove("/run/initd");
        }


        auto server = TRY(Event::TcpServer::create(Event::SocketType::Local));
        server->set_address(LexicalPath("/tmp/faen"));
        servers.append(server);
    }

    {
        auto server = TRY(Event::TcpServer::create(Event::SocketType::IPv4));
        server->set_address(IPv4Address { 127, 0, 0, 32 }, 8080);
        // servers.append(server);
    }

    return adopt_ref(*new Server(servers));
}

Server::Server(Vector<NonnullRefPtr<Event::TcpServer>> servers)
    : m_servers(move(servers))
{
}

Server::~Server() = default;

void Server::new_connection(NonnullRefPtr<Event::TcpStream> stream)
{
    outln("new_connection.fd = {}", stream->fd());
}

ErrorOr<bool> Server::setup()
{
    for (auto server : m_servers) {
        TRY(server->bind());
        TRY(server->listen());

        server->on_connection = [this](NonnullRefPtr<Event::TcpStream> stream) {
            new_connection(stream);
        };
    }
}