set(SYSTEM_NETWORKD_SOURCES main.cc Interface.cc InterfaceConfig.cc DhcpClient.cc DhcpPacket.cc)
set(SYSTEM_NETWORKD_HEADERS Interface.h InterfaceConfig.h DhcpClient.h DhcpClient.h)

add_executable(networkd ${SYSTEM_NETWORKD_SOURCES} ${SYSTEM_NETWORKD_HEADERS})

target_link_libraries(networkd LibAK LibEvent)