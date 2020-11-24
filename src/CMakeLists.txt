set(APP_NAME minimal_glfw_bullet)

set(demo_src
	imagedata.h
	imagedata.cpp

	glshader.h
	glshader.cpp
	
	glmeshdata.h
	glmeshdata.cpp
)

add_executable(${APP_NAME} main.cpp  ${demo_src})

target_link_libraries(${APP_NAME} glfw glew OpenGL::GL glm::glm)
target_link_libraries(${APP_NAME} ${Vulkan_LIBRARIES})

target_include_directories(${APP_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

source_group("sources" FILES main.cpp)
source_group("sources\\util" FILES ${demo_src})

set(BULLET_ROOT "C:/work/bullet3/_build/") # where to find Bullet
find_package(Bullet REQUIRED)
target_link_libraries(${APP_NAME}  ${BULLET_LIBRARIES})
target_include_directories(${APP_NAME} PUBLIC ${BULLET_INCLUDE_DIR})
