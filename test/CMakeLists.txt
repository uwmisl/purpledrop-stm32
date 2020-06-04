cmake_minimum_required(VERSION 3.10)
project(PurpleDropTests)

set(CMAKE_CXX_STANDARD 14)
# Do debug build for tests by default
set(CMAKE_BUILD_TYPE Debug)

include_directories(../src/app)

add_subdirectory(gtest-1.10.0)

set(BINARY PurpleDropTest)

set(TEST_SOURCES
    EventBroker-test.cpp
    MessageFramer-test.cpp
    Messages-test.cpp
)
set(SOURCES ${TEST_SOURCES})

add_executable(${BINARY} ${TEST_SOURCES})
add_test(NAME ${BINARY} COMMAND ${BINARY})

target_link_libraries(${BINARY} PUBLIC gtest_main)


