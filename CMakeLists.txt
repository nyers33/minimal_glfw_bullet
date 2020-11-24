cmake_minimum_required(VERSION 3.9)
project(minimal_glfw_bullet)

message("-- CMAKE_SYSTEM:                ${CMAKE_SYSTEM}")
message("-- CMAKE_SYSTEM_NAME:           ${CMAKE_SYSTEM_NAME}")
message("-- CMAKE_SYSTEM_INFO_FILE:      ${CMAKE_SYSTEM_INFO_FILE}")
message("-- CMAKE_SYSTEM_PROCESSOR:      ${CMAKE_SYSTEM_PROCESSOR}")

message("-- CMAKE_CXX_COMPILER_ID:       ${CMAKE_CXX_COMPILER_ID}")
message("-- CMAKE_CXX_COMPILER_VERSION:  ${CMAKE_CXX_COMPILER_VERSION}")

if(NOT CMAKE_VERSION VERSION_LESS 3.9)
  get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
elseif(CMAKE_CONFIGURATION_TYPES)
  set(_isMultiConfig True)
else()
  set(_isMultiConfig False)
endif()

if(_isMultiConfig)
	if(NOT DEFINED CMAKE_CONFIGURATION_TYPES)
		set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
	endif()
	message("-- CMAKE_CONFIGURATION_TYPES:   ${CMAKE_CONFIGURATION_TYPES}")
else()
	if(NOT DEFINED CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
	endif()
	message("-- CMAKE_BUILD_TYPE:            ${CMAKE_BUILD_TYPE}")
endif()

set( CMAKE_VERBOSE_MAKEFILE ON )
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

string(TOUPPER ${CMAKE_CXX_COMPILER_ID} CXX_COMPILER_ID_UPPER)
if(${CXX_COMPILER_ID_UPPER} STREQUAL MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /MP")

	#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	#option(USE_MSVC_RUNTIME_LIBRARY_DLL "Use MSVC runtime library DLL" OFF)

	# bullet3 build defaults to static runtime
	# cmake setup from bullet3\CMakeLists.txt
	set( USE_MSVC_RUNTIME_LIBRARY_DLL FALSE )
	IF (NOT USE_MSVC_RUNTIME_LIBRARY_DLL)
		#We statically link to reduce dependencies
		FOREACH(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO )
			IF(${flag_var} MATCHES "/MD")
				STRING(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
			ENDIF(${flag_var} MATCHES "/MD")
			IF(${flag_var} MATCHES "/MDd")
				STRING(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
			ENDIF(${flag_var} MATCHES "/MDd")
		ENDFOREACH(flag_var)
	ENDIF (NOT USE_MSVC_RUNTIME_LIBRARY_DLL)
endif()

IF(WIN32)
	add_definitions(-DNOMINMAX)
	add_definitions(-D_USE_MATH_DEFINES)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
ENDIF(WIN32)

### Adding GLEW
add_subdirectory(./src_extern/glew-2.1.0)
set_target_properties(glew PROPERTIES FOLDER "External Dependencies")

### Adding GLFW
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(./src_extern/glfw-3.3.2)
set_target_properties(glfw PROPERTIES FOLDER "External Dependencies")

find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

add_subdirectory(./src)