#include <arpa/inet.h>
#include <chrono>

#include "baseclientserver.h"

BaseClientServer::BaseClientServer( uint16_t port ) :
m_port( port ) {

}

bool* BaseClientServer::isStop()
{
   return &m_isStop;
}

int BaseClientServer::currentSocket( ) const
{
   return m_socket;
}

sockaddr_in BaseClientServer::buildSocketAddress( const char* serverAddress, uint16_t port )
{
   sockaddr_in socketAddress{};
   socketAddress.sin_addr.s_addr = inet_addr( serverAddress );
   socketAddress.sin_port = htons( port );
   socketAddress.sin_family = AF_INET;
   
   return socketAddress;
}

int BaseClientServer::openSocket( SocketType type )
{
   if( type == SocketType::TCP )
      return socket( AF_INET, SOCK_STREAM, 0 );
   else
      return socket( AF_INET, SOCK_DGRAM, 0 );
}

std::string BaseClientServer::currentDateTime( )
{
   // Получаем текущую дату и время, преобразуем в формат "YY-MM-DD hh:mm:ss" и возвращаем
   using namespace std;
   using namespace chrono;
   auto date  = system_clock::now( );
   auto datec = system_clock::to_time_t( date );
   
   auto   datet  = localtime( &datec );
   char   buffer[80];
   size_t result = strftime( buffer, sizeof( buffer ), "%y-%m-%d %H:%M:%S",
                             datet );
   return result > 0 ? string( buffer ) : string( );
}