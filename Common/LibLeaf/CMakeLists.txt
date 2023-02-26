include_directories(..)

SET(SOURCES
        Object.cc
        )

SET(HEADERS
        Object.h
        )

add_library(LibLeaf ${SOURCES} ${HEADERS})
target_link_libraries(LibLeaf LibAK)