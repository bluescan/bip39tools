# Should really specify a range of tested versions.
cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0091 NEW)

# Set default build-type (AKA the configuration in other IDEs).
set(CMAKE_BUILD_TYPE_INIT Release)

# Setup Release and Debug build-types (only).
# No reason to set CMAKE_CONFIGURATION_TYPES if it's not a multiconfig generator
# Also no reason mess with CMAKE_BUILD_TYPE if it's a multiconfig generator.
get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (isMultiConfig)
	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
else()
	if (NOT DEFINED CMAKE_BUILD_TYPE)
		message(STATUS "Status -- Default to Release build.")
		set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose Build Type" FORCE)
	endif()
	message(STATUS "Status -- Build type set to: ${CMAKE_BUILD_TYPE}")
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose Build Type")

	# Set the valid options for cmake-gui drop-down list. CMake tools for vscode does not (but should) respect this.
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

# bip39dice is the top level project. This include sets the version string variable.
include("Src/Version.cmake.h")
project(dice2bip39 VERSION "${DICE2BIP39_VERSION}" LANGUAGES C CXX)
message(STATUS "Status -- ProjectName ${PROJECT_NAME} ProjectVersion ${PROJECT_VERSION}")

# Find Git module.
find_package(Git)
if (Git_FOUND)
	message(STATUS "Status -- Git found: ${GIT_EXECUTABLE}")
endif()

# Grab the Tacent library from github at configure time.
include(FetchContent)
FetchContent_Declare(
	tacent
	GIT_REPOSITORY https://github.com/bluescan/tacent.git
	GIT_TAG        v0.8.7
)
FetchContent_MakeAvailable(tacent)

add_executable(
	${PROJECT_NAME}
	Src/Version.cpp
	Src/BipMain.cpp
	Src/Version.cmake.h
)

# Include directories needed to build.
target_include_directories(
	"${PROJECT_NAME}"
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
		$<TARGET_PROPERTY:Foundation,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:Math,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:System,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
		ARCHITECTURE_X64
		$<$<CONFIG:Debug>:CONFIG_DEBUG>
		$<$<CONFIG:Release>:CONFIG_RELEASE>
		$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_DEPRECATE _LIB>
		$<$<PLATFORM_ID:Windows>:PLATFORM_WINDOWS>
		$<$<PLATFORM_ID:Linux>:PLATFORM_LINUX>
)

# Set compiler option flags based on specific compiler and configuration.
target_compile_options(
	${PROJECT_NAME}
	PRIVATE
		# MSVC compiler.
		$<$<CXX_COMPILER_ID:MSVC>:/W2 /GS /Gy /Zc:wchar_t /Gm- /Zc:inline /fp:precise /WX- /Zc:forScope /Gd /FC /utf-8>
		$<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:/Od>
		$<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/O2>

		# Clang compiler.

		# GNU compiler. -std=c++20
		$<$<CXX_COMPILER_ID:GNU>:-Wno-unused-result>

		# Clang and GNU.
		$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wno-multichar>
		$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wno-switch>

		$<$<AND:$<CONFIG:Debug>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-O0>
		$<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-O2>
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# This is how you set things like CMAKE_DEBUG_POSTFIX for a target.
set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	# DEBUG_POSTFIX "d"												# Add a 'd' before the extension for debug builds.
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"	# Use multithreaded or multithreaded-debug runtime on windows.
	# More prop-value pairs here.
)

# Dependencies.
target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		Foundation Math System

		# I'm a little surprised cmake doesn't do these for me on Linux. The odd thing is they are
		# unnecessary if using VSCode/Cmake Tools, even in Ubuntu. They are, however, needed when
		# building from bash.
		$<$<PLATFORM_ID:Linux>:m>
		$<$<PLATFORM_ID:Linux>:stdc++>
		$<$<PLATFORM_ID:Linux>:stdc++fs>
		$<$<PLATFORM_ID:Windows>:shlwapi.lib>
)

if (MSVC)
	# target_link_options(${PROJECT_NAME} PRIVATE "/ENTRY:mainCRTStartup")
	# target_link_options(${PROJECT_NAME} PRIVATE "/SUBSYSTEM:CONSOLE")
	if (CMAKE_BUILD_TYPE MATCHES Debug)
		target_link_options(${PROJECT_NAME} PRIVATE "/NODEFAULTLIB:LIBCMT.lib")
	endif()
endif()

# Installation.
install(
	TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/BipInstall"
)
