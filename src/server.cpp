#include <sys/epoll.h>
#include <unistd.h>
#include <cstdio>
#include <string>
#include <iostream>
#include <thread>
#include <mutex>
#include <algorithm>

#include "server.h"

using namespace std;

Server::Server( uint16_t port ) :
        BaseClientServer( port )
{
    // Инициализируем вектор udp клиентов
    auto clientAddress = buildSocketAddress( "127.0.0.1", 8081 );
    udpClients.push_back( make_shared< struct sockaddr_in>( clientAddress ) );
}

std::string Server::serverMessage( uint16_t port, SocketType typeServer )
{
    // Формируем сообщение с текущей датой и временем, типом сервера и портом сервера
    auto dateTime = currentDateTime();
    string logMsg(  dateTime );
    logMsg = logMsg.append( " Сервер " );
    logMsg = ( typeServer == SocketType::TCP ) ? logMsg.append( "TCP." ) : logMsg.append( "UDP." );
    logMsg = logMsg.append( " Порт - " ).append( to_string( port ) ).append( "." );

    return logMsg;
}

Server::Status Server::start( SocketType typeSocket )
{
    auto logMsg = serverMessage( m_port, typeSocket );

    // Если сервер работает, то вырубаем его
    if( m_status == Status::UP )
        stop( m_socket, typeSocket,  &m_isStop );
    m_isStop = false;

    // Формируем адрес сервера
    auto socketAddress = buildSocketAddress( ipServer, m_port );
    // Открываем сокет
    if( ( m_socket = openSocket( typeSocket ) ) == -1 ) {
        cout << logMsg << " Не удалось открыть сокет." << endl;
        return m_status = Status::INIT_ERROR;
    }
    // Подключаем сокет к серверу по его ip адресу и порту
    int resultBind    = bind( m_socket, reinterpret_cast< struct sockaddr* >(  &socketAddress ),
                              sizeof( socketAddress ) );
    if( resultBind < 0 ) {
        cout << logMsg << " Не удалось подключить сокет к серверу." << endl;
        return m_status = Status::BIND_ERROR;
    }
    // Для TCP сокета начинаем прослушивание его
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
    // Обрабатываем сообщения от клиента с помощью epoll

    auto logMsg = serverMessage( port, typeServer );
    mutex lmutex;

    // Инициализируем epoll
    int epfd = epoll_create1( 0 );
    if ( epfd < 0 ) {
        cout << logMsg << " Не удалось инициализировать epoll." << endl;
        return ProcessState::EPOLL_CTL_ERROR;
    }
    // Добавляем сокет сервера к epoll
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

    // Цикл обработки сообщений от клиентов до тех пор, пока он не будет остановлен
    while( true )
    {
        // Если выставлен признак останова, то прекращаем обработку
        if( *isStop ) {
            cout << logMsg <<  " Останов обработки входящих подключений." << endl;
            break;
        }
        // Ждем получения данных от epoll 5 с
        int nfds = epoll_wait( epfd, evlist, MAX, 5000 );
        if ( nfds == -1 ) {
            cout << logMsg << " Ошибка ожидания данных epoll." << endl;
            continue;
        }
        // Цикл по полученным данным
        for ( int i = 0; i < nfds; i++ )
        {
            // Если это данные по сокету сервера и это TCP сервер
            if( evlist[i].data.fd == socket && typeServer == SocketType::TCP )
            {
                // Формируем адрес сервера
                socklen_t  socketAddressLen = sizeof( struct sockaddr );
                auto socketAddress = BaseClientServer::buildSocketAddress( ipServer, port );
                // Ожидаем подключения клиента к серверу и открываем новый сокет для взаимодействия
                int cfd = accept( socket, reinterpret_cast< struct sockaddr* >(  &socketAddress ), &socketAddressLen );
                if( cfd == -1 ) {
                    cout << logMsg << " Ошибка ожидания подключения клиента к серверу." << endl;
                    continue;
                } else {
                    cout << logMsg << " Клиент подключился к серверу." << endl;
                }

                // Увеличиваем число всех подключившихся клиентов и подключенных в данный момент
                lmutex.lock();
                if( std::find( tcpClientSockets.begin( ), tcpClientSockets.end( ), cfd
                ) == tcpClientSockets.end( ) )
                   tcpClientSockets.push_back( cfd );
                ++countAllClients;
                lmutex.unlock();

                // Добавляем новый сокет для взаимодействия к epoll
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
                ssize_t buflen;
                ProcessState status;
                if( typeServer == SocketType::TCP ) {
                    // Если это TCP сервер, то читаем сообщение из сокета и обрабатываем его
                    buflen       = read( cfd, buf, BUFSIZ - 1 );
                    if( buflen > 0 ) {
                        if ((status = processMessage(buf, buflen, cfd, logMsg,
                                                     typeServer, nullptr, isStop)) ==
                            ProcessState::CLOSE_ERROR)
                            return status;
                    } else if( buflen == 0 ) {
                        // Клиент отключился от сервера
                        // В таком случае уменьшаем число подключенных клиентов
                       auto socketPos = std::find( tcpClientSockets.begin( ),
                                                   tcpClientSockets.end( ), cfd );
                       if( socketPos != tcpClientSockets.end( ) ) {
                           lmutex.lock( );
                           tcpClientSockets.erase( socketPos );
                           lmutex.unlock( );
                        }
                    }
                } else {
                    // Если это UDP сервер, то тут нет механизма подключения клиентов

                    // Идем по вектору клиентов
                    for( const auto& clientAddr : udpClients ) {
                        // Получаем адрес и сообщение от клиента
                        auto addr = clientAddr.get( );
                        socklen_t addressLen = sizeof( struct sockaddr );
                        buflen = recvfrom( cfd, buf, BUFSIZ - 1, MSG_CONFIRM,
                                           reinterpret_cast< struct sockaddr * >( addr ),
                                           &addressLen );
                        // Обрабатываем сообщение от клиента
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

Server::ProcessState Server::processMessage( char *msg, ssize_t msgSize, int socket, const string& logMsg,
                                             SocketType typeServer, sockaddr_in* addr, bool* isStop )
{
    // Обрабатывает сообщение от клиента

    if( msgSize > 0 ) {
        msg[ msgSize ] = '\0';
        string smsg( msg );
        // Обрабатываем все сообщения, которые начинаются на символ /
        if( smsg.starts_with( '/' ) ) {
            cout << logMsg << " Получено сообщение от клиента: " << smsg << endl;

            if( smsg == "/time" ) {
               const char *const sendSuccess = " Отправлены текущие дата и время клиенту: ";
               const char *const sendFail    =
                             " Не удалось отправить текущие дату и время клиенту: ";
               
                // Получаем текущую дату и время в нужном нам формате
                auto dateTime = Server::currentDateTime( );
                send( socket, logMsg, dateTime, typeServer, addr, sendSuccess, sendFail );
            } else if( smsg == "/stats" ) {
               const char *const sendSuccess = " Отправлена статистика сервера клиенту: ";
               const char *const sendFail    =
                             " Не удалось отправить статистику сервера клиенту: ";
               
                // Формируем статистику
                string stats;
                if( typeServer == SocketType::TCP )
                  stats = "Общее число подключившихся клиентов: " + to_string( countAllClients ) +
                               ", число клиентов, подключенных в данный момент: " + to_string(
                                  tcpClientSockets.size() );
                else
                   stats = "Общее число клиентов: " + to_string( udpClients.size() );
                // Отправляем статистику клиенту
                send( socket, logMsg, stats, typeServer, addr, sendSuccess, sendFail );
            } else if( smsg == "/shutdown" ) {
                // Останавливаем сервер
                Status retStop = Server::stop( socket, typeServer, isStop );
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

void Server::printSendResult( ssize_t result, const std::string& logMsg, const char* const msg,
                              const char *const sendSuccess, const char *const sendFail )
{
    // Выводит результат отправки в консоль
    if( result == -1 )
        cout << logMsg << sendFail << endl;
    else
        cout << logMsg << sendSuccess << msg << endl;
}

void Server::send( int socket, const std::string& logMsg, const std::string& msg,
                   SocketType typeServer, sockaddr_in* addr, const char *const sendSuccess,
                   const char *const sendFail )
{
    // Отправляем данные клиенту и выводим статус отправки в консоль
    // TCP сервер отправляет, используя функцию write
    // UDP сервер отправляет, используя функцию sendto
    if( typeServer == Server::SocketType::TCP ) {
        auto result = write( socket, msg.c_str( ), msg.size() );
        printSendResult( result, logMsg, msg.c_str(), sendSuccess, sendFail );
    } else {
        socklen_t addressLen = sizeof( struct sockaddr );
        auto result = sendto( socket, msg.c_str( ), msg.size(), MSG_CONFIRM,
                              reinterpret_cast< struct sockaddr * >( addr ), addressLen );
        printSendResult( result, logMsg, msg.c_str(), sendSuccess, sendFail );
    }
}

Server::Status Server::stop( int socket, SocketType typeServer, bool* isStop )
{
    // Останавливаем сервер
    
    // Выставляем признак останова в true для останова потока обработки сообщений от клиентов
    *isStop = true;
    Status status;
    int result;
    // Отключаем соединение и закрываем сокет, если это TCP сервер
    if( typeServer == SocketType::TCP ) {
       if( shutdown( socket, SHUT_RDWR ) == 0 ) {
          result = close( socket );
          status = ( result == -1 ) ? Status::CLOSE_ERROR : Status::CLOSE;
       } else
          status = Status::CLOSE_ERROR;
    } else {
       // Для UDP сервера просто закрываем сокет
       result = close( socket );
       status = ( result == -1 ) ? Status::CLOSE_ERROR : Status::CLOSE;
    }
    return status;
}