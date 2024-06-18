cmake_minimum_required(VERSION 3.24)
project(TpxServer)

set(CMAKE_CXX_STANDARD 20)

find_package(Qt6 REQUIRED COMPONENTS
        Core
        Widgets)

qt_standard_project_setup()

option(BUILD_SHARED OFF)
option(BUILD_STATIC ON)
set(ENABLE_PRECOMPILED OFF)
option(BUILD_TESTS OFF)
option(CPPZMQ_BUILD_TESTS OFF)

add_executable(TpxServer
        src/main.cpp
        src/ui/MainWindow.cpp
        src/ui/SettingsPanel.cpp
        src/ui/StatusPanel.cpp
        src/ui/LogWindow.cpp
        src/log.cpp
        src/BgThread.cpp
        src/server/CommsThread.cpp

        include/BgThread.h
        include/ui/LogWindow.h
        include/server/CommsThread.h

        include/server/TimepixConnectionManager.h
        src/server/TimepixConnectionManager.cpp
        include/server/TimepixCodes.h
        include/server/PythonConnectionManager.h
        src/server/PythonConnectionManager.cpp
        include/server/ServerCodes.h

        ext/asio_imp.cpp
        include/common_defs.h
        include/server/UdpThread.h
        src/server/UdpThread.cpp
        include/server/UdpConnectionManager.h
        src/server/UdpConnectionManager.cpp
        include/server/TimepixCommandInfo.h
        include/server/SecondaryThread.h
        src/server/SecondaryThread.cpp)

target_include_directories(TpxServer PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/ext/asio-1.28.0/include
        ${CMAKE_SOURCE_DIR}/ext/toml/include
        ${CMAKE_SOURCE_DIR}/ext/libzmq/include
        ${CMAKE_SOURCE_DIR}/ext/cppzmq)

set(BUILD_STATIC ON CACHE BOOL "" FORCE)

add_subdirectory(ext/libzmq)
add_subdirectory(ext/cppzmq)

add_dependencies(cppzmq libzmq-static)

SET(ZeroMQ_DIR ${CMAKE_CURRENT_BINARY_DIR}/ext/libzmq)

target_link_libraries(TpxServer PUBLIC
        libzmq-static
        iphlpapi.dll
        Qt::Core
        Qt::Widgets)

target_compile_definitions(TpxServer PUBLIC ASIO_SEPARATE_COMPILATION)

target_compile_options(TpxServer PUBLIC -D_WIN32_WINNT=0xA00)

target_link_options(TpxServer PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)

#project(DummySpidr)
#
#set(CMAKE_CXX_STANDARD 20)
#
#add_executable(DummySpidr
#        test/DummySpidr.h
#        test/DummySpidr.cpp
#
#        ext/asio_imp.cpp)
#
#target_include_directories(DummySpidr PUBLIC
#        ${CMAKE_SOURCE_DIR}/include
#        ${CMAKE_SOURCE_DIR}/ext/asio-1.28.0/include
#        ${CMAKE_SOURCE_DIR}/ext/toml/include)
#
#target_compile_definitions(DummySpidr PUBLIC ASIO_SEPARATE_COMPILATION)
#
#target_compile_options(DummySpidr PUBLIC -D_WIN32_WINNT=0xA00)
#
