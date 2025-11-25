#include <sys/socket.h>
#include <sys/epoll.h>
#include <iostream>
#include <cstring>
#include <unistd.h>

#include "client.h"

using namespace std;

Client::Client( const char *serverAddress, uint16_t port, SocketType typeServer ) :
BaseClientServer( port ), m_serverIP( serverAddress ), m_typeServer( typeServer ) {
}

BaseClientServer::SocketType Client::type( ) const
{
   return m_typeServer;
}

std::string Client::clientMessage( const char *serverAddress, uint16_t port, SocketType typeServer )
{
   // Формируем сообщение с текущей датой и временем, типом клиента, ip адресом и портом сервера
   string logMsg( currentDateTime() );
   logMsg = logMsg.append( " Клиент " );
   logMsg = ( typeServer == SocketType::TCP ) ? logMsg.append( "TCP." ) : logMsg.append( "UDP." );
   logMsg = logMsg.append( " ip сервера - " ).append( serverAddress ).append( "." );
   logMsg = logMsg.append( " Порт - " ).append( to_string( port ) ).append( "." );
   
   return logMsg;
}

bool Client::connectToServer()
{
   auto logMsg = clientMessage( m_serverIP, m_port, m_typeServer );
   // Открываем сокет для взаимодействия с сервером
   if( ( m_socket = openSocket( m_typeServer ) ) < 0 ) {
      cout << logMsg << " Не удалось открыть сокет." << endl;
      return false;
   }
   // Подключаемся к серверу
   m_serverAddress = buildSocketAddress( m_serverIP, m_port );
   socklen_t servAddressLen = sizeof( struct sockaddr );
   int result = connect( m_socket, reinterpret_cast< struct sockaddr * >(
      &m_serverAddress ), servAddressLen );
   return result == 0;
}

bool Client::sendDataToServer( const char* message )
{
   // Отправляем сообщение серверу
   // Для TCP и UDP способ отправки разный
   ssize_t result;
   if( m_typeServer == SocketType::TCP )
      result = send( m_socket, message, strlen( message ), MSG_CONFIRM );
   else {
      socklen_t servAddressLen = sizeof( struct sockaddr );
      result = sendto( m_socket, message, strlen( message ), MSG_CONFIRM,
                       reinterpret_cast< const struct sockaddr * >(
                          &m_serverAddress ), servAddressLen );
   }
   return result != -1;
}

Client::ProcessState Client::process( const char *serverAddress, int socket, uint16_t port,
                                      const bool *isStopProcess, SocketType typeServer )
{
   // Обрабатываем данные от сервера с помощью epoll

   auto logMsg = clientMessage( serverAddress, port, typeServer );

   // Инициализируем epoll
   int epfd = epoll_create1( 0 );
   if( epfd < 0 ) {
      cout << logMsg << " Не удалось инициализировать epoll." << endl;
      return ProcessState::EPOLL_CTL_ERROR;
   }
   // Добавляем сокет клиента к epoll
   struct epoll_event ev{ };
   ev.events  = EPOLLIN;
   ev.data.fd = socket;
   int ret = epoll_ctl( epfd, EPOLL_CTL_ADD, socket, &ev );
   if( ret < 0 ) {
      cout << logMsg << " Не удалось добавить сокет к epoll." << endl;
      return ProcessState::EPOLL_CTL_ERROR;
   }
   
   struct epoll_event evlist[MAX];
   
   cout << logMsg << " Обработка данных от сервера." << endl;

   // Цикл обработки данных от сервера до тех пор, пока он не будет остановлен
   while( true ) {
      // Если выставлен признак останова, то прекращаем обработку
      if( *isStopProcess ) {
         cout << logMsg << " Останов обработки данных от сервера." << endl;
         break;
      }
      // Ждем получения данных от epoll 5 с
      int nfds = epoll_wait( epfd, evlist, MAX, 5000 );
      if( nfds == -1 ) {
         cout << logMsg << " Ошибка ожидания данных epoll." << endl;
         return ProcessState::EPOLL_WAIT_ERROR;
      }
      // Цикл по полученным данным
      for( int i = 0; i < nfds; i++ ) {
         // Если это данные по сокету клиента
         if( evlist[ i ].data.fd == socket ) {
            char    buf[BUFSIZ];
            int     cfd    = evlist[ i ].data.fd;
            // Читаем данные в буфер и, если он не пустой, выводим данные в консоль
            ssize_t buflen = read( cfd, buf, BUFSIZ - 1 );
            if( buflen > 0 ) {
               buf[ buflen ] = '\0';
               string msg( buf );
               cout << logMsg << " Получено сообщение от сервера: " <<
                    msg << endl;
            }
         }
      }
   }
   
   return ProcessState::SUCCESS;
}

bool Client::stop( )
{
   auto logMsg = clientMessage( m_serverIP, m_port, m_typeServer );
   cout << logMsg << " Останов клиента. " << endl;
   // Прекращаем поток обработки данных от сервера
   m_isStop = true;
   int result;
   bool isSuccess;
   // Отключаем соединение и закрываем сокет, если это TCP сервер
   if( type() == SocketType::TCP ) {
      if( shutdown( m_socket, SHUT_RDWR ) == 0 ) {
         result = close( m_socket );
         isSuccess = result != -1;
      } else
         isSuccess = false;
   } else {
      // Для UDP сервера просто закрываем сокет
      result = close( m_socket );
      isSuccess = result != -1;
   }
   
   return isSuccess;
}

