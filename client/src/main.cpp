#include <iostream>
#include <thread>
#include <chrono>
#include <future>
#include <vector>

#include "client.h"

using namespace std;

typedef shared_ptr< Client > ClientPtr;

/**
 * Структура данных клиента
 */
struct ClientData {
   // Клиент
   ClientPtr client{};
   // Поток обработки данных от сервера
   future< Client::ProcessState > clientProcess{};
};

typedef shared_ptr< ClientData > ClientDataPtr;

/**
 * Отправляет данные серверу
 * @param client указатель на клиента
 * @param log сообщение лога
 * @param msg сообщение, которое нужно отправить
 */
void sendData( const ClientPtr& client, const string &log, const char *const msg )
{
   const char *const sendSuccess = " Сообщение успешно отправлено серверу. Сообщение - ";
   const char *const sendFail    = " Не удалось отправить сообщение серверу. Сообщение - ";
   
   if( client->sendDataToServer( msg ) ) {
      cout << log << sendSuccess << msg << endl;
   } else {
      cout << log << sendFail << msg <<  " Ошибка: " << errno << endl;
   }
}

/**
 * Создает клиента
 * @param serverIp ip сервера
 * @param port порт сервера
 * @param clientType тип клиента: TCP, UDP
 * @return умный указатель на клиента
 */
ClientDataPtr createClient( const char *const serverIp, int port, Client::SocketType clientType ) {
   
   const char *const connectSuccess = " Успешно подключились к серверу.";
   const char *const connectFail    = " Не удалось подключиться к серверу.";
   // Создаем структуру данных клиента
   ClientDataPtr clientData( new ClientData() );
   // Создаем клиента
   ClientPtr client( new Client( serverIp, port,
                                 clientType ) );
   auto      logMsg = Client::clientMessage( serverIp, port,
                                             clientType );
   // Подключаемся к серверу
   if( client->connectToServer( ) ) {
      if( clientType == Client::SocketType::TCP )
         cout << logMsg << connectSuccess << endl;
      
      int clientSocket = client->currentSocket( );
      // Запускам асинхронно поток обработки данных от сервера
      clientData->clientProcess = async( Client::process, serverIp, clientSocket,
                                         port,
                                         client->isStop(), clientType );
      // Отправляем серверу запрос несколько раз
      for( int j = 0; j < 2; ++j ) {
         // Отправляем запрос на полученик текущей даты и времени
         const char *const msgTime = "/time";
         sendData( client, logMsg, msgTime );
         // Ждем 2 секунды
         this_thread::sleep_for( chrono::seconds( 2 ) );
         // Отправляем не обрабатываемый запрос
         const char *const msgHello = "/hello";
         sendData( client, logMsg, msgHello );
         // Ждем 2 секунды
         this_thread::sleep_for( chrono::seconds( 2 ) );
      }
      
      clientData->client = client;
   } else {
      if( clientType == Client::SocketType::TCP )
         cout << logMsg << connectFail << endl;
   }
   
   return clientData;
}

/**
 * Останавливает клиентов
 * @param clients вектор клиентов
 * @param logMsg сообщение лога
 */
void closeClients( vector< ClientDataPtr >& clients, const string& logMsg ) {
   // Лямбда функция останавливает клиента и удаляет его из списка
   auto stopClient = []( vector< ClientDataPtr >& clients, size_t indexClient ) {
      auto clientData = clients[ indexClient ];
      clientData->client->stop();
      clients.erase( clients.end() );
   };
   
   /**
    * Идем в цикле по TCP клиентам с конца, останавливаем клиент и удаляем его из списка.
    * Для последнего оставшегося клиента передаем сервер команду на останов сервера
    */
   while( !clients.empty() ) {
      if( clients.size() == 1 ) {
         // Отправляем запрос на получение статистики сервера
         const char *const msgStats = "/stats";
         sendData( clients[ 0 ]->client, logMsg, msgStats );
         // Ждем 2 секунды
         this_thread::sleep_for( chrono::seconds( 4 ) );
         
         const char *const msgShutdown = "/shutdown";
         sendData( clients[ 0 ]->client, logMsg, msgShutdown );
         stopClient( clients, 0 );
      } else {
         size_t indexClient = clients.size( ) - 1;
         stopClient( clients, indexClient );
      }
   }
}

int main() {
   // Векторы TCP, UDP клиентов
   vector< ClientDataPtr > tcpClients;
   vector< ClientDataPtr > udpClients;
   
   // IP адрес сервера
   const char* const serverIp ="127.0.0.1";
   // Порты TCP и UDP сервера
   const int tcpServerPort = 8080;
   const int udpServerPort = 8081;
   // В цикле создаем TCP и UDP сервер и добавляем их в списки
   for( int i = 0; i < 2; ++i ) {
      auto clientData = createClient( serverIp, tcpServerPort, Client::SocketType::TCP );
      tcpClients.push_back( clientData );
      
      clientData = createClient( serverIp, udpServerPort, Client::SocketType::UDP );
      udpClients.push_back( clientData );
   }
   
   // Закрываем TCP и UDP клиенты
   auto      logMsg = Client::clientMessage( serverIp, tcpServerPort,
                                             Client::SocketType::TCP );
   closeClients( tcpClients, logMsg );
   
   logMsg = Client::clientMessage( serverIp, udpServerPort,
                                   Client::SocketType::UDP );
   closeClients( udpClients, logMsg );
}