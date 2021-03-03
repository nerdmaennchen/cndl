cmake_minimum_required(VERSION 3.12)
project(cndl)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(PkgConfig)

file(GLOB_RECURSE CPP_SRCS ./*.cpp)

add_library(cndl SHARED ${CPP_SRCS})

pkg_check_modules(SIMPLYFILE REQUIRED simplyfile)
include_directories(${SIMPLYFILE_INCLUDE_DIRS})
target_compile_options(cndl PRIVATE ${SIMPLYFILE_CFLAGS})
target_link_libraries(cndl ${SIMPLYFILE_LIBRARIES})

pkg_check_modules(CRYPTO REQUIRED libcrypto)
include_directories(${CRYPTO_INCLUDE_DIRS})
target_compile_options(cndl PRIVATE ${CRYPTO_CFLAGS})
target_link_libraries(cndl ${CRYPTO_LIBRARIES})

execute_process(
        COMMAND git describe --tags
        OUTPUT_VARIABLE VERSION
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        ERROR_VARIABLE VERSION_ERROR
        OUTPUT_STRIP_TRAILING_WHITESPACE)

if(VERSION STREQUAL "")
    message(FATAL_ERROR "cannot generate version string")
endif()

message("cndl version: " ${VERSION})

configure_file(cndl.pc.in cndl.pc @ONLY)

install(TARGETS cndl DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES
    base64.h
    ConnectionHandler.h
    Dispatcher.h
    Error.h
    Extractor.h
    ProtocolHandler.h
    Request.h
    Response.h
    Route.h
    Server.h
    StaticFileHandler.h
    unique_function.h
    Websocket.h
    WSRoute.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cndl)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cndl.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
