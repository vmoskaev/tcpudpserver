#include <future>

#include "server.h"

int main() {
   int tcpPort = 8080;
   bool isTcpServerStop = false;
   // Создаем TCP сервер
   Server tcpServer( tcpPort );
   // Запускаем его
   auto tcpServerStatus = tcpServer.start( Server::SocketType::TCP );
   std::future< Server::ProcessState > tcpProcessResult{};
   // Если сервер запустился, то запускаем поток обработки сообщений от клиентов
   if( tcpServerStatus == Server::Status::UP ) {
      int  tcpServerSocket  = tcpServer.currentSocket( );
      tcpProcessResult = std::async( Server::process, tcpServerSocket,
                                     tcpPort, &isTcpServerStop,
                                     Server::SocketType::TCP );
   }
   
   int    udpPort          = 8081;
   bool   isUdpServerStop  = false;
   // Создаем UDP сервер
   Server udpServer( udpPort );
   // Запускаем его
   auto   udpServerStatus  = udpServer.start( Server::SocketType::UDP );
   std::future< Server::ProcessState > udpProcessResult{ };
   // Если сервер запустился, то запускаем поток обработки сообщений от клиентов
   if( udpServerStatus == Server::Status::UP ) {
      int  udpServerSocket  = udpServer.currentSocket( );
      udpProcessResult = std::async( Server::process, udpServerSocket,
                                     udpPort, &isUdpServerStop, Server::SocketType::UDP );
   }
   // Ожидаем завершения потоков обраотки сообщений TCP и UDP серверов
   if( tcpServerStatus == Server::Status::UP )
      tcpProcessResult.wait( );
   if( udpServerStatus == Server::Status::UP )
      udpProcessResult.wait( );
}