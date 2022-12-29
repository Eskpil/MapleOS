include_directories(..)

SET(SOURCES
        EventLoop.cc
        Timer.cc
        UdpServer.cc
        TcpServer.cc
        TcpStream.cc
        Signals.cc
        )

SET(HEADERS
        EventLoop.h
        Timer.h
        UdpServer.h
        TcpServer.h
        SocketType.h
        TcpStream.h
        Signals.h
        )

add_library(LibEvent ${SOURCES} ${HEADERS})
target_link_libraries(LibEvent LibAK)