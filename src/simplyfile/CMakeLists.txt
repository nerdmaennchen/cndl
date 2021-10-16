cmake_minimum_required(VERSION 3.12)
project(simplyfile)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE CPP_SRCS ./*.cpp)

add_library(simplyfile SHARED ${CPP_SRCS})

set(DEST_DIR "${CMAKE_INSTALL_PREFIX}")

execute_process(
        COMMAND git describe --tags
        OUTPUT_VARIABLE VERSION
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        ERROR_VARIABLE VERSION_ERROR
        OUTPUT_STRIP_TRAILING_WHITESPACE)

if(VERSION STREQUAL "")
    message(FATAL_ERROR "cannot generate version string")
endif()

message("simplyfile version: " ${VERSION})

configure_file(simplyfile.pc.in simplyfile.pc @ONLY)

install(TARGETS simplyfile DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES
    Epoll.h
    Event.h
    FileDescriptor.h
    INotify.h
    Process.h
    SerialPort.h
    ThreadTime.h
    Timer.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
install(FILES
    socket/Host.h
    socket/Socket.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/socket)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/simplyfile.pc
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pkgconfig)
install(FILES LICENSE
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/licenses/${PROJECT_NAME})
