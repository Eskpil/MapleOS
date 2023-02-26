set(SYSTEM_INIT_SOURCES main.cc Service.cc ServiceConfig.cc Targets.cc Target.cc TargetConfig.cc ExecutionOrder.cc Essentials.cc Server.cc)
set(SYSTEM_INIT_HEADERS Service.h ServiceConfig.h Targets.h Target.h TargetConfig.h ExecutionOrder.h Essentials.h GroupFile.cc GroupFile.h PasswdFile.cc PasswdFile.h Server.h)

add_executable(init ${SYSTEM_INIT_SOURCES} ${SYSTEM_INIT_HEADERS})
target_link_libraries(init LibAK LibEvent)