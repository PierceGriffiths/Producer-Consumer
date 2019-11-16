cmake_minimum_required(VERSION 3.1.3 FATAL_ERROR)
project("Producer Consumer" LANGUAGES C)
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR} CACHE PATH "" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

set(default_build_type "MinSizeRel")
IF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
	set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel")
ENDIF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)

IF("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_C_FLAGS "-march=native -Wall -Wextra -pedantic")
	set(CMAKE_C_FLAGS_MINSIZEREL "-Os -fno-asynchronous-unwind-tables -funroll-loops -Wl,--gc-sections")
	set(CMAKE_C_FLAGS_RELEASE "-O3 -fno-unwind-tables -fno-asynchronous-unwind-tables")
	set(CMAKE_C_FLAGS_DEBUG "-Og -pg -ggdb")

	IF(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-export-dynamic")
	ENDIF(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
ELSEIF("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
	set(CMAKE_C_FLAGS "-march=native -Wall -Wextra -pedantic")
	set(CMAKE_C_FLAGS_MINSIZEREL "-Os")
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
		C_EXTENSIONS OFF
		LINKER_LANGUAGE C
		INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/src/include"
		LINK_LIBRARIES Threads::Threads
)
IF(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(producer-consumer PUBLIC NDEBUG)
ENDIF(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")

IF(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" AND "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" AND NOT WIN32)
	add_custom_command(TARGET producer-consumer
		POST_BUILD
		COMMAND strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag ${CMAKE_INSTALL_PREFIX}/producer-consumer)
ENDIF(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" AND "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" AND NOT WIN32)


install(TARGETS producer-consumer DESTINATION ${CMAKE_INSTALL_PREFIX})
