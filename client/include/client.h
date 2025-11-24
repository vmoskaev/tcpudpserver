#ifndef TCPUDPSERVER_CLIENT_H
#define TCPUDPSERVER_CLIENT_H

#include <string>

#include "baseclientserver.h"

/**
 * Класс клиента TCP и UDP
 */
class Client : public BaseClientServer
{
public:

   /**
    * Конструктор
    * @param serverAddress ip сервера
    * @param port порт сервера
    * @param typeServer тип сервера: TCP, UDP
    */
   explicit Client( const char* serverAddress, uint16_t port, SocketType typeServer );

   /**
    * Деструктор
    */
   ~Client() override = default;
   
   SocketType type() const;

   /**
    * Формирует сообщение лога клиента
    * @param serverAddress адрес сервера
    * @param port порт сервера
    * @param typeServer тип сервера
    * @return сообщение лога
    */
   static std::string clientMessage( const char *serverAddress, uint16_t port,
                                     SocketType typeServer );
   /**
    * Подключает клиента к серверу
    * @return trus - успешно подключились, false - нет
    */
   bool connectToServer();

   /**
    * Отправляет сообщение серверу
    * @param message сообщение
    * @return true - успешно отправлено, false - не отправлено
    */
   bool sendDataToServer( const char *message );

   /**
    * Обрабатывает данные, полученные от сервера, через epoll
    * @param serverAddress адрес сервера
    * @param socket открытый сокет для передачи данных
    * @param port порт сервера
    * @param isStop признак остановки обработки данных
    * @param typeServer тип сервера: TCP, UDP
    * @return
    */
   static ProcessState process( const char *serverAddress, int socket, uint16_t port,
                                const bool *isStop = nullptr,
                                SocketType typeServer = SocketType::TCP );

   /**
    * Останавливает клиент
    * @return true - клиент остановлен, false - не удалось остановить клиент
    */
   bool stop();
private:
    // IP адрес сервера
   const char* m_serverIP = nullptr;
    // Тип сервера
   SocketType m_typeServer = SocketType::TCP;
   // Адрес сервера
   sockaddr_in m_serverAddress{};
};

#endif //TCPUDPSERVER_CLIENT_H
