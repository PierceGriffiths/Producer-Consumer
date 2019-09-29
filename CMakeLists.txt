cmake_minimum_required(VERSION 3.13.4 FATAL_ERROR)
project("Producer Consumer" LANGUAGES C)
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	SET(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR} CACHE PATH "" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

set(default_build_type "Release")
IF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
	set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
ENDIF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)

IF("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
	add_compile_options("-march=native" "-Wall" "-Wextra" "-pedantic")
	set(CMAKE_C_FLAGS_RELEASE "-O3")
	set(CMAKE_C_FLAGS_DEBUG "-Og -pg")
ELSEIF("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
	add_compile_options("-march=native" "-Wall" "-Wextra" "-pedantic")
	set(CMAKE_C_FLAGS_RELEASE "-O3")
	set(CMAKE_C_FLAGS_DEBUG "-O0")
ELSE()
	message(WARNING "You are using a compiler which might not be compatible with this project!")
ENDIF("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")

add_executable(producer-consumer src/main.c src/queue.c src/producer.c src/consumer.c src/logreader_threads.c)
find_package(Threads REQUIRED)
set_target_properties(
	producer-consumer
	PROPERTIES
		C_STANDARD 11
		C_STANDARD_REQUIRED ON
		C_EXTENSIONS ON
		LINKER_LANGUAGE C
		INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/src/include"
		LINK_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}"
)

install(TARGETS producer-consumer DESTINATION ${CMAKE_SOURCE_DIR})
