cmake_minimum_required(VERSION 2.8)

project( enkiTSExamples )

include_directories("enkiTS/src")
include_directories("Remotery/lib")
include_directories("imgui")
include_directories("glfw/include")
include_directories("imgui/examples/opengl_example")

set( ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "Build basic example applications" )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL  "GLFW lib only" )
set( GLFW_BUILD_TESTS OFF CACHE BOOL  "GLFW lib only" )
set( GLFW_BUILD_DOCS OFF CACHE BOOL  "GLFW lib only" )
set( GLFW_BUILD_INSTALL OFF CACHE BOOL  "GLFW lib only" )

add_subdirectory( enkiTS )
add_subdirectory( glfw )

set( ENKITSREMOTERYSAMPLE_SRC
     enkiTSRemoteryExample.cpp
     Remotery/lib/Remotery.h
     Remotery/lib/Remotery.c
     )
	 
add_executable( enkiTSRemoteryExample ${ENKITSREMOTERYSAMPLE_SRC} )
target_link_libraries(enkiTSRemoteryExample enkiTS )

if( ENKITS_BUILD_C_INTERFACE )
	set( ENKITSREMOTERYSAMPLE_C_SRC
		 enkiTSRemoteryExample.c
		 Remotery/lib/Remotery.h
		 Remotery/lib/Remotery.c
		 )
	add_executable( enkiTSRemoteryExample_C ${ENKITSREMOTERYSAMPLE_C_SRC} )
	target_link_libraries(enkiTSRemoteryExample_C enkiTS )
endif()


set( ENKITSMICROPROFILESAMPLE_SRC
     imgui/imgui.h
     imgui/imgui.cpp
     imgui/imgui_internal.h
     imgui/imgui_demo.cpp
     imgui/imgui_draw.cpp
	 imgui/imconfig.h
     imgui/examples/opengl_example/imgui_impl_glfw.h
     imgui/examples/opengl_example/imgui_impl_glfw.cpp
     imgui/examples/opengl_example/main.cpp
     )
add_executable( enkiTSMicroprofileExample ${ENKITSMICROPROFILESAMPLE_SRC} )
target_link_libraries(enkiTSMicroprofileExample enkiTS opengl32 glfw )
