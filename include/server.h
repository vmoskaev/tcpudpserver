#ifndef TCPUDPSERVER_SERVER_H
#define TCPUDPSERVER_SERVER_H

#include <iomanip>
#include <memory>
#include <vector>
#include <netinet/in.h>

#include "baseclientserver.h"

static const char *const ipServer = "127.0.0.1";

static std::vector< std::shared_ptr< struct sockaddr_in > > udpClients;
static std::vector< std::shared_ptr< struct sockaddr_in > > tcpClients;
static int countAllTcpClients{};
static int countConnectedTcpClients{};

/**
 * Базовый класс сервера
 */
class Server : public BaseClientServer
{
public:
   /**
    * Статус сервера
    */
   enum class Status
   {
      UP, // Работает
      INIT_ERROR, // Ошибка создания сокета
      BIND_ERROR, // Ошибка подключения сокета к серверу
      KEEP_ALIVE_ERROR,
      LISTEN_ERROR, // Ошибка включения прослушивания сокета
      CLOSE_ERROR, // Ошибка закрытия сокета
      CLOSE // Выключен
   };
   
   /**
    * Конструктор
    * @param port порт сервера
    */
   explicit Server( uint16_t port );
   
   /**
    * Деструктор
    */
   ~Server() override = default;
   
   /**
     * Запускает сервер
     * @param typeSocket тип сокета сервера
     * @return статус сервера
    */
   Status start( SocketType typeSocket );
   
   /**
    * Обрабатывает запросы от клиентов и возвращает им требуемую информацию
    * @param socket сокет сервера
    * @param port порт
    * @param isStop признак завершения работы сервера
    * @param typeServer тип сервера:
    * @return статус обработки запросов
    */
   static ProcessState process( int socket, uint16_t port, bool *isStop = nullptr,
                                SocketType typeServer = SocketType::TCP );
   
   static ProcessState processMessage( char* msg, ssize_t msgSize, int socket,
                               const std::string& logMsg, SocketType typeServer,
                                       sockaddr_in *addr = nullptr, bool* isStop = nullptr );
   
   /**
    * Останавливает сервер
    * @param socket сокет
    * @return статус сервера
    */
   static Status stop( int socket, bool *isStop );
   
   static std::string serverMessage( uint16_t port, SocketType typeServer );

private:
   // Статус сервера
   Status m_status = Status::CLOSE;
   
};

#endif //TCPUDPSERVER_SERVER_H
