include_directories(..)

SET(SOURCES
        DnsPacket.cc
        )

SET(HEADERS
        DnsPacket.h
        Types.h
        )

add_library(LibDNS ${SOURCES} ${HEADERS})
target_link_libraries(LibDNS LibAK)