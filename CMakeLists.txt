cmake_minimum_required(VERSION "3.16.0")

# Set default build-type (AKA the configuration in other IDEs).
set(CMAKE_BUILD_TYPE_INIT Release)

# No reason to set CMAKE_CONFIGURATION_TYPES if it's not a multiconfig generator
# Also no reason mess with CMAKE_BUILD_TYPE if it's a multiconfig generator.
get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (isMultiConfig)
	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE) 
else()
	if (NOT DEFINED CMAKE_BUILD_TYPE)
		message(STATUS "MSG -- Default to Release build.")
		set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose Build Type" FORCE)
	endif()
	message(STATUS "MSG -- Build type set to: ${CMAKE_BUILD_TYPE}")
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose Build Type")

	# Set the valid options for cmake-gui drop-down list. CMake tools for vscode does not (but should) respect this.
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")	
endif()

# TacentBIP39 is the top level project. This include sets the version string variable.
include("Version.cmake.h")
project(TacentBIP39 VERSION "${TACENTBIP39_VERSION}" LANGUAGES C CXX)
message(STATUS "MSG -- ProjectName ${PROJECT_NAME} ProjectVersion ${PROJECT_VERSION}")

# We want a better default for install prefix. It is bad form to be modifying
# system files from a cmake build of anything. Really quite surprised someone
# thinks the cmake defaults are good.
#message(STATUS "init=${CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT}")
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "..." FORCE)
endif()
message(STATUS "MSG -- InstallPrefix ${CMAKE_INSTALL_PREFIX}")

