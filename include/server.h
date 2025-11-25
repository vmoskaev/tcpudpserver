#ifndef TCPUDPSERVER_SERVER_H
#define TCPUDPSERVER_SERVER_H

#include <iomanip>
#include <memory>
#include <vector>
#include <netinet/in.h>

#include "baseclientserver.h"

static const char *const ipServer = "127.0.0.1";

static std::vector< std::shared_ptr< struct sockaddr_in > > udpClients;
static std::vector< int > tcpClientSockets;
static int countAllClients{};

/**
 * Класс сервера TCP, UDP
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
   
   /**
    * Останавливает сервер
    * @param socket сокет
    * @param typeServer тип сервера
    * @param isStop признак останова обработки сообщений
    * @return статус сервера
    */
   static Status stop( int socket, SocketType typeServer, bool *isStop );

   /**
    * Формирует сообщение лога
    * @param port порт
    * @param typeServer тип сервера
    * @return сообщение лога
    */
   static std::string serverMessage( uint16_t port, SocketType typeServer );

private:
    // Статус сервера
    Status m_status = Status::CLOSE;

    /**
     * Обрабатывает сообщение от клиента
     * @param msg сообщение
     * @param msgSize размер сообщения
     * @param socket сокет
     * @param logMsg сообщение лога
     * @param typeServer тип сервера: TCP, UDP
     * @param addr адрес сервера
     * @param isStop признак останова обработки сообщений
     * @return статус обработки сообщения
     */
    static ProcessState processMessage( char* msg, ssize_t msgSize, int socket,
                                        const std::string& logMsg, SocketType typeServer,
                                        sockaddr_in *addr = nullptr, bool* isStop = nullptr );

    /**
     * Выводит результат отправки в консоль
     * @param result результат отправки сообщения
     * @param logMsg сообщение лога
     * @param msg сообщение клиенту
     * @param sendSuccess сообщение об успешной отправке
     * @param sendFail сообщение о неудачной отправке
     */
    static void printSendResult( ssize_t result, const std::string &logMsg,
                                 const char* msg, const char* sendSuccess,
                                 const char* sendFail );

    /**
     * Отправляет сообщение клиенту
     * @param socket сокет
     * @param logMsg сообщение лога
     * @param msg сообщение для отправки
     * @param typeServer тип сервера
     * @param addr адрес клиента
     * @param sendSuccess сообщение об успешной отправке
     * @param sendFail сообщение о неудачной отправке
     */
    static void send( int socket, const std::string& logMsg, const std::string& msg, SocketType typeServer,
               sockaddr_in* addr, const char* sendSuccess, const char* sendFail );
};

#endif //TCPUDPSERVER_SERVER_H
