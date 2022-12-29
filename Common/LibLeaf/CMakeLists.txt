include_directories(..)

SET(SOURCES
        LeafPacket.cc
        )

SET(HEADERS
        LeafPacket.h
        )

add_library(LibLeaf ${SOURCES} ${HEADERS})
target_link_libraries(LibLeaf LibAK)