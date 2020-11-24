cmake_minimum_required(VERSION 2.8.11)
cmake_policy(VERSION 2.8.11)

project(glew)

SET(SRC_GLEW
	glew.c
	GL/glew.h
	GL/glxew.h
	GL/wglew.h
)

if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -fno-common -fPIC -Wall")
endif()

add_library(glew STATIC ${SRC_GLEW})
target_include_directories(glew PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(glew PUBLIC GLEW_NO_GLU GLEW_STATIC)