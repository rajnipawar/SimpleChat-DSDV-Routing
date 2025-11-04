cmake_minimum_required(VERSION 3.16)
project(SimpleChat_P2P_PA3 VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try Qt6 first, fallback to Qt5
find_package(Qt6 COMPONENTS Core Widgets Network)
if(NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network)
    set(QT_VERSION 5)
else()
    set(QT_VERSION 6)
endif()

if(QT_VERSION EQUAL 6)
    qt_standard_project_setup()
else()
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
endif()

set(SOURCES
    src/main.cpp
    src/simplechat.cpp
    src/chatwindow.cpp
    src/message.cpp
    src/networkmanager.cpp
)

set(HEADERS
    src/simplechat.h
    src/chatwindow.h
    src/message.h
    src/networkmanager.h
)

if(QT_VERSION EQUAL 6)
    qt_add_executable(SimpleChat_PA3 ${SOURCES} ${HEADERS})
    target_link_libraries(SimpleChat_PA3
        PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network)
    target_include_directories(SimpleChat_PA3 PRIVATE src)
else()
    add_executable(SimpleChat_PA3 ${SOURCES} ${HEADERS})
    target_link_libraries(SimpleChat_PA3 Qt5::Core Qt5::Widgets Qt5::Network)
    target_include_directories(SimpleChat_PA3 PRIVATE src)
endif()

# Option to build tests
option(BUILD_TESTS "Build test suite" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
