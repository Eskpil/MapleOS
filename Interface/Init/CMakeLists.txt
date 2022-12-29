set(SOURCES main.cc)
set(HEADERS)

add_executable(interface ${SOURCES} ${HEADERS})

target_link_libraries(interface LibAK LibEvent)