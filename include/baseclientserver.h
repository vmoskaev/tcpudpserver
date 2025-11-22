#ifndef TCPUDPSERVER_BASECLIENTSERVER_H
#define TCPUDPSERVER_BASECLIENTSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>

#define MAX 1024

/**
 * Базовый класс для клиента и сервера
 */
class BaseClientServer
{
public:
   
   /**
    * Тип сокета сервера
    */
   enum class SocketType
   {
      TCP,
      UDP
   };
   
   enum class ProcessState
   {
      EPOLL_CTL_ERROR,
      EPOLL_WAIT_ERROR,
      ACCEPT_ERROR,
      CLOSE_ERROR,
      CLOSE,
      SUCCESS
   };
   
   /**
    * Конструктор
    */
   explicit BaseClientServer( uint16_t port );
   
   /**
    * Деструктор
    */
   virtual ~BaseClientServer() = default;
   
   [[nodiscard]] int currentSocket() const;
   
   /**
    * Формирует адрес для сервера
    * @param port порт сервера
    * @return адрес сервера
    */
   static sockaddr_in buildSocketAddress( const char* serverAddress, uint16_t port );
   
   static std::string currentDateTime( );
protected:
   // Сокет
   int m_socket = -1;
   // Порт
   uint16_t m_port = -1;
   bool m_isStop = false;
   
   
   /**
    * Открывает сокет
    * @param type тип сокета
    * @return сокет
    */
   static int openSocket( SocketType type );
};

#endif //TCPUDPSERVER_BASECLIENTSERVER_H
