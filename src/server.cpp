#include <sys/epoll.h>
#include <unistd.h>
#include <cstdio>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

#include "server.h"

using namespace std;

Server::Server( uint16_t port ) :
BaseClientServer( port )
{
   auto clientAddress = buildSocketAddress( "127.0.0.1", 8081 );
   udpClients.push_back( make_shared< struct sockaddr_in>( clientAddress ) );
}

std::string Server::serverMessage( uint16_t port, SocketType typeServer )
{
   auto dateTime = currentDateTime();
   string logMsg(  dateTime );
   logMsg = logMsg.append( " Сервер " );
   logMsg = ( typeServer == SocketType::TCP ) ? logMsg.append( "TCP." ) : logMsg.append( "UDP." );
   logMsg = logMsg.append( " Порт - " ).append( to_string( port ) ).append( "." );
   
   return logMsg;
}

Server::Status Server::start( SocketType typeSocket )
{
   int flag{ };
   
   auto logMsg = serverMessage( m_port, typeSocket );
   
   if( m_status == Status::UP )
      stop( m_socket, &m_isStop );
   
   auto socketAddress = buildSocketAddress( ipServer, m_port );
   
   if( ( m_socket = openSocket( typeSocket ) ) == -1 ) {
      cout << logMsg << " Не удалось открыть сокет." << endl;
      return m_status = Status::INIT_ERROR;
   }
   
   flag = true;
   int resultSockOpt = setsockopt( m_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof( flag ) );
   int resultBind    = bind( m_socket, reinterpret_cast< struct sockaddr* >(  &socketAddress ),
                             sizeof( socketAddress ) );
   if( resultSockOpt == -1 || resultBind < 0 ) {
      cout << logMsg << " Не удалось подключить сокет к серверу." << endl;
      return m_status = Status::BIND_ERROR;
   }
   
   if( typeSocket == SocketType::TCP ) {
      if( listen( m_socket, SOMAXCONN ) < 0 ) {
         cout << logMsg << " Не удалось включить сервер на прослушивание." << endl;
         return m_status = Status::LISTEN_ERROR;
      }
   }
   
   m_status = Status::UP;
   
   cout << logMsg << " Сервер запущен." << endl;
   
   return m_status;
}

Server::ProcessState Server::process( int socket, uint16_t port, bool* isStop,
                                      SocketType typeServer )
{
   auto logMsg = serverMessage( port, typeServer );
   mutex lmutex;
   
   int epfd = epoll_create1( 0 );
   if ( epfd < 0 ) {
      cout << logMsg << " Не удалось инициализировать epoll." << endl;
      return ProcessState::EPOLL_CTL_ERROR;
   }
   struct epoll_event ev{};
   ev.events = EPOLLIN;
   ev.data.fd = socket;
   int ret = epoll_ctl( epfd, EPOLL_CTL_ADD, socket, &ev );
   if( ret < 0 ) {
      cout << logMsg << " Не удалось добавить сокет к epoll." << endl;
      return ProcessState::EPOLL_CTL_ERROR;
   }
   
   struct epoll_event evlist[ MAX ];
   
   cout << logMsg << " Обработка входящих подключений." << endl;

   while( true )
   {
      if( *isStop ) {
         cout << logMsg <<  " Останов обработки входящих подключений." << endl;
         break;
      }
      int nfds = epoll_wait( epfd, evlist, MAX, -1 );
      if ( nfds == -1 ) {
         cout << logMsg << " Ошибка ожидания данных epoll." << endl;
         continue;
      }

      for ( int i = 0; i < nfds; i++ )
      {
         if( evlist[i].data.fd == socket && typeServer == SocketType::TCP )
         {
            socklen_t  socketAddressLen = sizeof( struct sockaddr );
            auto socketAddress = BaseClientServer::buildSocketAddress( ipServer, port );
            int cfd = accept( socket, reinterpret_cast< struct sockaddr* >(  &socketAddress ), &socketAddressLen );
            if( cfd == -1 ) {
               cout << logMsg << " Ошибка ожидания подключения клиента к серверу." << endl;
               continue;
            } else {
               cout << logMsg << " Клиент подключился к серверу." << endl;
            }
            
            lmutex.lock();
            tcpClients.push_back( make_shared< struct sockaddr_in >( socketAddress ) );
            countAllTcpClients++;
            countConnectedTcpClients++;
            lmutex.unlock();
            
            ev.events = EPOLLIN;
            ev.data.fd = cfd;
            ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
            if( ret == -1 ) {
               cout << logMsg << " Не удалось добавить сокет к epoll." << endl;
               continue;
            }
         }
         else
         {
            char buf[BUFSIZ];
            int cfd = evlist[i].data.fd;
            ssize_t buflen{};
            ProcessState status = ProcessState::SUCCESS;
            if( typeServer == SocketType::TCP ) {
               buflen       = read( cfd, buf, BUFSIZ - 1 );
               if( ( status = processMessage( buf, buflen, cfd, logMsg,
                                              typeServer, nullptr, isStop ) ) ==
                                              ProcessState::CLOSE_ERROR )
                  return status;
            } else {
               for( const auto& clientAddr : udpClients ) {
                  auto addr = clientAddr.get( );
                  socklen_t addressLen = sizeof( struct sockaddr );
                  buflen = recvfrom( cfd, buf, BUFSIZ - 1, MSG_CONFIRM,
                                     reinterpret_cast< struct sockaddr * >( addr ),
                                     &addressLen );
                  if( ( status = processMessage( buf, buflen, cfd, logMsg, typeServer, addr,
                                                 isStop ) ) ==
                      ProcessState::CLOSE_ERROR )
                     return status;
               }
            }
         }
      }
      this_thread::sleep_for( chrono::seconds( 1 ) );
   }
   
   return ProcessState::SUCCESS;
}

Server::ProcessState Server::processMessage( char *msg, ssize_t msgSize, int socket,
                             const string& logMsg, SocketType typeServer, sockaddr_in* addr,
                             bool* isStop )
{
   if( msgSize > 0 ) {
      msg[ msgSize ] = '\0';
      string smsg( msg );
      if( smsg.starts_with( '/' ) ) {
         cout << logMsg << " Получено сообщение от клиента: " <<
                                                              smsg << endl;
         
         const char *const sendSuccess = " Отправлены текущие дата и время клиенту: ";
         const char *const sendFail = " Не удалось отправить текущие дату и время клиенту: ";
         
         if( smsg == "/time" ) {
            auto dateTime = Server::currentDateTime( );
            auto size     = dateTime.size( );
            
            if( typeServer == Server::SocketType::TCP ) {
               auto result = write( socket, dateTime.c_str( ), size );
               if( result == -1 )
                  cout << logMsg << sendFail << endl;
               else
                  cout << logMsg << sendSuccess << dateTime.c_str( ) << endl;
            } else {
               socklen_t addressLen = sizeof( struct sockaddr );
               auto result = sendto( socket, dateTime.c_str( ), size, MSG_CONFIRM,
                       reinterpret_cast< struct sockaddr * >( addr ), addressLen );
               if( result == -1 )
                  cout << logMsg << sendFail << endl;
               else
                  cout << logMsg << sendSuccess << dateTime.c_str( ) << endl;
            }
            cout << logMsg << " Отправлены текущие дата и время клиенту: " <<
                 msg << endl;
         } else if( smsg == "/shutdown" ) {
            Status retStop = Server::stop( socket, isStop );
            if( retStop == Status::CLOSE ) {
               cout << logMsg << " Сервер остановлен." << endl;
               return ProcessState::CLOSE;
            } else {
               cout << logMsg << " Ошибка остановки сервера." << endl;
               return ProcessState::CLOSE_ERROR;
            }
         }
      }
   }
   
   return ProcessState::SUCCESS;
}

Server::Status Server::stop( int socket, bool* isStop )
{
   *isStop = true;
   Status status;
   int result;
   if( shutdown( socket, SHUT_RDWR ) == 0 ) {
      result = close( socket );
      status = ( result == -1 ) ? Status::CLOSE_ERROR : Status::CLOSE;
   } else
      status = Status::CLOSE_ERROR;
   return status;
}