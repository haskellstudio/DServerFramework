cmake_minimum_required (VERSION 2.6)
project(dserverframework)

if(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
elseif(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif()

set(net_src_list
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/net/EventLoop.cpp
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/net/CurrentThread.cpp
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/net/DataSocket.cpp
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/net/SocketLibFunction.c
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/net/TCPService.cpp
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/net/WrapTCPService.cpp
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/net/Connector.cpp
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/net/fdset.c
  ${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/net/NetSession.cpp
)

set(timer_src_list
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/timer/Timer.cpp
)

set(base_src_list
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/utils/array.c
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/utils/buffer.c
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/utils/systemlib.c
)

set(http_src_list
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/net/http/http_parser.c
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/net/http/HttpParser.cpp
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/net/http/HttpService.cpp
)

set(utils_src_list
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/utils/base64.cpp
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/utils/md5calc.cpp
	${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/utils/SHA1.cpp
  ${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/utils/systemlib.c
  ${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/utils/ox_file.cpp
  ${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/utils/app_status.c
)

include_directories("${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/net")
include_directories("${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/timer")
include_directories("${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/utils")
include_directories("${PROJECT_SOURCE_DIR}/3rdparty/brynet/src/net/http")

include_directories("${PROJECT_SOURCE_DIR}/3rdparty/pb")
include_directories("${PROJECT_SOURCE_DIR}/3rdparty/spdlog/include")
include_directories("${PROJECT_SOURCE_DIR}/3rdparty/rapidjson/include")
include_directories("${PROJECT_SOURCE_DIR}/src/Common")
include_directories("${PROJECT_SOURCE_DIR}/src/Common/prototol")

set(connection_server_src_list
  ${PROJECT_SOURCE_DIR}/ServerConfig/ServerConfig.pb.cc
  ${PROJECT_SOURCE_DIR}/src/Common/UsePacketSingleNetSession.cpp
  ${PROJECT_SOURCE_DIR}/src/ConnectionServer/ClientSession.cpp
  ${PROJECT_SOURCE_DIR}/src/ConnectionServer/ClientSessionMgr.cpp
  ${PROJECT_SOURCE_DIR}/src/ConnectionServer/ConnectionServer.cpp
  ${PROJECT_SOURCE_DIR}/src/ConnectionServer/LogicServerSession.cpp
  ${PROJECT_SOURCE_DIR}/src/ConnectionServer/LogicServerSessionMgr.cpp
)

set(logic_server_src_list
  ${PROJECT_SOURCE_DIR}/ServerConfig/ServerConfig.pb.cc
  ${PROJECT_SOURCE_DIR}/src/Common/UsePacketSingleNetSession.cpp
  ${PROJECT_SOURCE_DIR}/src/LogicServer/CenterServerConnection.cpp
  ${PROJECT_SOURCE_DIR}/src/LogicServer/ClientMirror.cpp
  ${PROJECT_SOURCE_DIR}/src/LogicServer/ConnectionServerConnection.cpp
  ${PROJECT_SOURCE_DIR}/src/LogicServer/LogicServer.cpp
  ${PROJECT_SOURCE_DIR}/src/LogicServer/GlobalValue.cpp
  ${PROJECT_SOURCE_DIR}/src/test/LogicServerExt.cpp
)

set(common_src_list
  ${PROJECT_SOURCE_DIR}/src/Common/NetThreadSession.cpp
  ${PROJECT_SOURCE_DIR}/src/Common/UsePacketExtNetSession.cpp
)

set(client_src_list
  ${PROJECT_SOURCE_DIR}/src/SimulatePlayer/SimulateClient.cpp
  ${PROJECT_SOURCE_DIR}/src/Common/UsePacketSingleNetSession.cpp
)

set(center_server_src_list
  ${PROJECT_SOURCE_DIR}/src/CenterServer/CenterServer.cpp
  ${PROJECT_SOURCE_DIR}/src/CenterServer/CenterServerSession.cpp
  ${PROJECT_SOURCE_DIR}/src/Common/UsePacketSingleNetSession.cpp
  ${PROJECT_SOURCE_DIR}/src/test/CenterServerExt.cpp
  ${PROJECT_SOURCE_DIR}/ServerConfig/ServerConfig.pb.cc
)


include_directories("${PROJECT_SOURCE_DIR}/3rdparty/drpc/3rdparty/msgpack-c/include")
include_directories("${PROJECT_SOURCE_DIR}/3rdparty/drpc/include")
link_directories("${PROJECT_SOURCE_DIR}/3rdparty/pb")

add_executable(connectionserver ${net_src_list} ${timer_src_list} ${utils_src_list} ${http_src_list} ${base_src_list} ${connection_server_src_list})
if(WIN32)
  target_link_libraries(connectionserver ws2_32 libprotobufd)
elseif(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(connectionserver Threads::Threads)
endif()

add_executable(logicserver ${common_src_list} ${net_src_list} ${timer_src_list} ${utils_src_list} ${http_src_list} ${base_src_list} ${logic_server_src_list})
if(WIN32)
  target_link_libraries(logicserver ws2_32 libprotobufd)
elseif(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(logicserver Threads::Threads)
endif()

add_executable(centerserver ${common_src_list} ${net_src_list} ${timer_src_list} ${utils_src_list} ${http_src_list} ${base_src_list} ${center_server_src_list})
if(WIN32)
  target_link_libraries(centerserver ws2_32 libprotobufd)
elseif(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(centerserver Threads::Threads)
endif()

add_executable(client ${common_src_list} ${net_src_list} ${timer_src_list} ${utils_src_list} ${http_src_list} ${base_src_list} ${client_src_list})
if(WIN32)
  target_link_libraries(client ws2_32 libprotobufd)
elseif(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(client Threads::Threads)
endif()