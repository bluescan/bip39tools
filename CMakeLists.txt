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

# This include sets the version string variable for all the BipTools (dice2bip39, finalwordsbip39, unittestsbip39, validatebip39),
include("Src/Version.cmake.h")

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
	# GIT_TAG v0.8.12
	GIT_TAG F9B2E08A63912908BC42D77C604CC4D47EBC79EB
)
FetchContent_MakeAvailable(tacent)

function(common_tool_configuration PROJNAME)

	# Include directories needed to build.
	target_include_directories(
		"${PROJNAME}"
		PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/Src
			$<TARGET_PROPERTY:Foundation,INTERFACE_INCLUDE_DIRECTORIES>
			$<TARGET_PROPERTY:Math,INTERFACE_INCLUDE_DIRECTORIES>
			$<TARGET_PROPERTY:System,INTERFACE_INCLUDE_DIRECTORIES>
	)

	target_compile_definitions(
		${PROJNAME}
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
		${PROJNAME}
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
		${PROJNAME}
		PROPERTIES
		# DEBUG_POSTFIX "d"												# Add a 'd' before the extension for debug builds.
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"	# Use multithreaded or multithreaded-debug runtime on windows.
		# More prop-value pairs here.
	)

	# Dependencies.
	target_link_libraries(
		${PROJNAME}
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
			target_link_options(${PROJNAME} PRIVATE "/NODEFAULTLIB:LIBCMT.lib")
		endif()
	endif()

	# Installation.
	install(
		TARGETS ${PROJNAME}
		RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Bip39ToolsInstall"
	)

endfunction(common_tool_configuration)


configure_file("Src/Bip39ToolsLauncher" "Bip39ToolsLauncher" COPYONLY)
configure_file("Src/Bip39ToolsLauncher.bat" "Bip39ToolsLauncher.bat" COPYONLY)
install(
	FILES Src/Bip39ToolsLauncher Src/Bip39ToolsLauncher.bat
	DESTINATION "${CMAKE_INSTALL_PREFIX}/Bip39ToolsInstall"
)


##############
# dice2bip39 #
##############
project(dice2bip39 VERSION "${BIP39TOOLS_VERSION}" LANGUAGES C CXX)
message(STATUS "Status -- ProjectName ${PROJECT_NAME} ProjectVersion ${PROJECT_VERSION}")
add_executable(
	${PROJECT_NAME}
	Src/Version.cpp
	Src/Bip39/Bip39.h
	Src/Bip39/Bip39.cpp
	Src/Bip39/Dictionary/Dictionary.h
	Src/Bip39/Dictionary/Dictionary.cpp
	Src/Dice2Bip39.cpp
	Src/Version.cmake.h
)
common_tool_configuration(${PROJECT_NAME})


###################
# finalwordsbip39 #
###################
project(finalwordsbip39 VERSION "${BIP39TOOLS_VERSION}" LANGUAGES C CXX)
message(STATUS "Status -- ProjectName ${PROJECT_NAME} ProjectVersion ${PROJECT_VERSION}")
add_executable(
	${PROJECT_NAME}
	Src/Version.cpp
	Src/Bip39/Bip39.h
	Src/Bip39/Bip39.cpp
	Src/Bip39/Dictionary/Dictionary.h
	Src/Bip39/Dictionary/Dictionary.cpp
	Src/FinalWordsBip39.cpp
	Src/Version.cmake.h
)
common_tool_configuration(${PROJECT_NAME})


#################
# validatebip39 #
#################
project(validatebip39 VERSION "${BIP39TOOLS_VERSION}" LANGUAGES C CXX)
message(STATUS "Status -- ProjectName ${PROJECT_NAME} ProjectVersion ${PROJECT_VERSION}")
add_executable(
	${PROJECT_NAME}
	Src/Version.cpp
	Src/Bip39/Bip39.h
	Src/Bip39/Bip39.cpp
	Src/Bip39/Dictionary/Dictionary.h
	Src/Bip39/Dictionary/Dictionary.cpp
	Src/ValidateBip39.cpp
	Src/Version.cmake.h
)
common_tool_configuration(${PROJECT_NAME})


######################
# makecompliantbip39 #
######################
project(makecompliantbip39 VERSION "${BIP39TOOLS_VERSION}" LANGUAGES C CXX)
message(STATUS "Status -- ProjectName ${PROJECT_NAME} ProjectVersion ${PROJECT_VERSION}")
add_executable(
	${PROJECT_NAME}
	Src/Version.cpp
	Src/Bip39/Bip39.h
	Src/Bip39/Bip39.cpp
	Src/Bip39/Dictionary/Dictionary.h
	Src/Bip39/Dictionary/Dictionary.cpp
	Src/MakeCompliantBip39.cpp
	Src/Version.cmake.h
)
common_tool_configuration(${PROJECT_NAME})


##################
# unittestsbip39 #
##################
project(unittestsbip39 VERSION "${BIP39TOOLS_VERSION}" LANGUAGES C CXX)
message(STATUS "Status -- ProjectName ${PROJECT_NAME} ProjectVersion ${PROJECT_VERSION}")
add_executable(
	${PROJECT_NAME}
	Src/Version.cpp
	Src/Bip39/Bip39.h
	Src/Bip39/Bip39.cpp
	Src/Bip39/Dictionary/Dictionary.h
	Src/Bip39/Dictionary/Dictionary.cpp
	Src/UnitTestsBip39.cpp
	Src/Version.cmake.h
)
common_tool_configuration(${PROJECT_NAME})
