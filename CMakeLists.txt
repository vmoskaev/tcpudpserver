cmake_minimum_required( VERSION 3.25 )
project( tcpudpserver )

set( CMAKE_CXX_STANDARD 20 )

set( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin )
set( LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin )

include_directories( include )

file( GLOB TCPUDPSERVER_INCLUDES "include/*.h" )
file( GLOB TCPUDPSERVER_SOURCES "src/*.cpp" )

set( SOURCES ${TCPUDPSERVER_INCLUDES} ${TCPUDPSERVER_SOURCES} )

add_executable( tcpudpserver ${SOURCES} )
