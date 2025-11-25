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

   /**
    * Состояние обработки полученных данных
    */
   enum class ProcessState
   {
      // Ошибка инициализации epoll
      EPOLL_CTL_ERROR,
      // Ошибка закрытия сокета
      CLOSE_ERROR,
      // Сокет закрыт
      CLOSE,
      // Успешное выполнение
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

   /**
    * Возвращает признак останова обработки данных клиентом
    * @return признак останова
    */
   bool* isStop();

   /**
    * Возвращает сокет
    * @return сокет
    */
   [[nodiscard]] int currentSocket() const;
   
   /**
    * Формирует адрес для сервера
    * @param serverAddress ip сервера
    * @param port порт сервера
    * @return адрес сервера
    */
   static sockaddr_in buildSocketAddress( const char* serverAddress, uint16_t port );

   /**
    * Формирует и возвращает текущие дату и время
    * @return текущая дата и время
    */
   static std::string currentDateTime( );
protected:
   // Сокет
   int m_socket = -1;
   // Порт
   uint16_t m_port = -1;
   // Признак останова потока обработки данных
   bool m_isStop = false;
   
   /**
    * Открывает сокет
    * @param type тип сокета
    * @return сокет
    */
   static int openSocket( SocketType type );
};

#endif //TCPUDPSERVER_BASECLIENTSERVER_H
