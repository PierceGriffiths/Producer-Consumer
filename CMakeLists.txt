cmake_minimum_required(VERSION 3.13.4 FATAL_ERROR)
project("Producer Consumer")
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR} CACHE PATH "" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "-march='native' -Wall -pedantic")

add_executable(producer-consumer src/main.c src/queue.c src/producer.c src/consumer.c src/logreader_threads.c)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(producer-consumer "${CMAKE_THREAD_LIBS_INIT}")
include_directories(PUBLIC "${CMAKE_SOURCE_DIR}/src/include/")

install(TARGETS producer-consumer DESTINATION ${CMAKE_SOURCE_DIR})
