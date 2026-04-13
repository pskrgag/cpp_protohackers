cmake_minimum_required(VERSION 3.15)
project(cpp_shit VERSION 1.0.0 LANGUAGES CXX)

# -----------------------------------------------------------------------------
# Options
# -----------------------------------------------------------------------------
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TESTS "Build tests" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers (Address, Undefined)" OFF)

# -----------------------------------------------------------------------------
# C++ standard
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

# -----------------------------------------------------------------------------
# Generate compile_commands.json for clangd (and other tools)
# -----------------------------------------------------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBURING REQUIRED IMPORTED_TARGET liburing>=2.0)

# -----------------------------------------------------------------------------
# Library sources
# -----------------------------------------------------------------------------
add_library(cocur
	STATIC
	src/net/socket.cpp
)

# Public headers
target_include_directories(cocur
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

# Set properties (optional)
set_target_properties(cocur PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_link_libraries(cocur PRIVATE PkgConfig::LIBURING)

# -----------------------------------------------------------------------------
# Sanitizers
# -----------------------------------------------------------------------------
if (ENABLE_SANITIZERS)
	# For Clang/GCC
	set(SANITIZER_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer -g -DWITH_SANITIZERS")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()

if (ENABLE_THREAD_SANITIZER)
	if (ENABLE_SANITIZERS)
		message(FATAL_ERROR "Error")
	endif()

	# For Clang/GCC
	set(SANITIZER_FLAGS "-fsanitize=thread -fno-omit-frame-pointer -g -DWITH_SANITIZERS")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")
# -----------------------------------------------------------------------------
# Install rules
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS cocur
	EXPORT CocurTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

install(EXPORT CocurTargets
	FILE CocurTargets.cmake
	NAMESPACE cocur::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cocur
)

# -----------------------------------------------------------------------------
# Tests (optional)
# -----------------------------------------------------------------------------
if(BUILD_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()

# -----------------------------------------------------------------------------
# Tasks
# -----------------------------------------------------------------------------
add_subdirectory(flyio/task0)
add_subdirectory(flyio/task1)
add_subdirectory(flyio/task2)
