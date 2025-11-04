cmake_minimum_required(VERSION 3.16)

# Find Qt Test module
if(QT_VERSION EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Test)
else()
    find_package(Qt5 REQUIRED COMPONENTS Test)
endif()

# Include source directory
include_directories(${CMAKE_SOURCE_DIR}/src)

# Test executable - consolidated test file
set(TEST_SOURCES
    tests.cpp
    ../src/message.cpp
    ../src/networkmanager.cpp
)

if(QT_VERSION EQUAL 6)
    qt_add_executable(tests ${TEST_SOURCES})
    target_link_libraries(tests
        PRIVATE
        Qt6::Core
        Qt6::Test
        Qt6::Network)
else()
    add_executable(tests ${TEST_SOURCES})
    target_link_libraries(tests
        Qt5::Core
        Qt5::Test
        Qt5::Network)
endif()

# Add test
add_test(NAME PA3_Tests COMMAND tests)

enable_testing()
