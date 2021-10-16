cmake_minimum_required(VERSION 3.12)
project(qrqma)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE CPP_SRCS ./*.cpp)

add_library(qrqma SHARED ${CPP_SRCS})

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

message("qrqma version: " ${VERSION})

configure_file(qrqma.pc.in qrqma.pc @ONLY)

install(TARGETS qrqma DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES
    loader.h
    template.h
    unique_function.h
    symbol.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qrqma)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qrqma.pc
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pkgconfig)
install(FILES LICENSE
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/licenses/${PROJECT_NAME})
