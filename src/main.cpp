#include <future>

#include "server.h"

int main() {
    int tcpPort = 8080;
    bool isTcpServerStop = false;
    Server tcpServer( tcpPort );
    auto tcpServerStatus = tcpServer.start( Server::SocketType::TCP );
    std::future< Server::ProcessState > tcpProcessResult{};
    if( tcpServerStatus == Server::Status::UP ) {
       int  tcpServerSocket  = tcpServer.currentSocket( );
       tcpProcessResult = std::async( Server::process, tcpServerSocket,
                                           tcpPort, &isTcpServerStop,
                                           Server::SocketType::TCP );
    }
    
    int    udpPort          = 8081;
    bool   isUdpServerStop  = false;
    Server udpServer( udpPort );
    auto   udpServerStatus  = udpServer.start( Server::SocketType::UDP );
    std::future< Server::ProcessState > udpProcessResult{ };
    if( udpServerStatus == Server::Status::UP ) {
       int  udpServerSocket  = udpServer.currentSocket( );
       udpProcessResult = std::async( Server::process, udpServerSocket,
                                           udpPort, &isUdpServerStop, Server::SocketType::UDP );
    }
   
   if( tcpServerStatus == Server::Status::UP )
      tcpProcessResult.wait( );
   if( udpServerStatus == Server::Status::UP )
      udpProcessResult.wait( );
}