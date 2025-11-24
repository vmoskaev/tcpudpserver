cmake_minimum_required( VERSION 3.25 )
project( tcpudpclient )

set( CMAKE_CXX_STANDARD 20 )

set( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin )
set( LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin )

include_directories( include )

file( GLOB TCPUDPCLIENT_INCLUDES "include/*.h" )
file( GLOB TCPUDPCLIENT_SOURCES "src/*.cpp" )

set( SOURCES ${TCPUDPCLIENT_INCLUDES} ${TCPUDPCLIENT_SOURCES} )

add_executable( tcpudpclient ${SOURCES} )
