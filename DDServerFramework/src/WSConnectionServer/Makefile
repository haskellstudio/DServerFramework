source = ClientSession.cpp\
		ClientSessionMgr.cpp\
        ConnectionServer.cpp\
		LogicServerSession.cpp\
		LogicServerSessionMgr.cpp\
		UseCellnetSingleNetSession.cpp\
        UseWebPacketSingleNetSession.cpp\
        proto/LogicServerWithConnectionServer.pb.cc\
        ../../ServerConfig/ServerConfig.pb.cc\
        ../Common/NetThreadSession.cpp\
        ../Common/UsePacketExtNetSession.cpp\
        ../Common/UsePacketSingleNetSession.cpp\
		../../3rdparty/accumulation-dev/src/net/CurrentThread.cpp\
		../../3rdparty/accumulation-dev/src/net/DataSocket.cpp\
		../../3rdparty/accumulation-dev/src/net/EventLoop.cpp\
		../../3rdparty/accumulation-dev/src/net/NetSession.cpp\
		../../3rdparty/accumulation-dev/src/net/SocketLibFunction.c\
		../../3rdparty/accumulation-dev/src/net/TCPService.cpp\
		../../3rdparty/accumulation-dev/src/net/WrapTCPService.cpp\
		../../3rdparty/accumulation-dev/src/utils/buffer.c\
        ../../3rdparty/accumulation-dev/src/utils/app_status.c\
		../../3rdparty/accumulation-dev/src/utils/ox_file.cpp\
		../../3rdparty/accumulation-dev/src/utils/systemlib.c\
        ../../3rdparty/accumulation-dev/src/utils/SHA1.cpp\
        ../../3rdparty/accumulation-dev/src/utils/base64.cpp\
		../../3rdparty/accumulation-dev/src/timer/Timer.cpp\
        ../../3rdparty/accumulation-dev/src/net/http/http_parser.c\
        ../../3rdparty/accumulation-dev/src/net/http/HttpServer.cpp\
        ../../3rdparty/accumulation-dev/src/net/http/HttpParser.cpp\

server:
	g++ -I../Common -I../Common/prototol\
    -I../../3rdparty/spdlog/include\
    -I../../3rdparty/accumulation-dev/src/net\
    -I../../3rdparty/accumulation-dev/src/utils\
    -I../../3rdparty/accumulation-dev/src/timer\
    -I../../3rdparty/accumulation-dev/src/net/http\
    $(source)  -D_GLIBCXX_USE_CXX11_ABI=0 -O3 -std=c++11 -lpthread -lrt -lprotobuf -o wsconnectionserver
