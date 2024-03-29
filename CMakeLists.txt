cmake_minimum_required(VERSION 2.8)

project( enkiTSExamples )

find_package(OpenGL)

include_directories("enkiTS/src")
include_directories("Remotery/lib")
include_directories("imgui")
include_directories("microprofile")
include_directories("glfw/include")
include_directories("imgui/examples")
include_directories( ${OPENGL_INCLUDE_DIRS} )

set( ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "Build basic example applications" )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL  "GLFW lib only" )
set( GLFW_BUILD_TESTS OFF CACHE BOOL  "GLFW lib only" )
set( GLFW_BUILD_DOCS OFF CACHE BOOL  "GLFW lib only" )
set( GLFW_BUILD_INSTALL OFF CACHE BOOL  "GLFW lib only" )

add_subdirectory( enkiTS )

if(OpenGL_FOUND)
	add_subdirectory( glfw )
endif()

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

if(UNIX)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()
if(APPLE)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()
if( MSVC )
    SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup" )
endif()

if(OpenGL_FOUND)
	set( ENKITSMICROPROFILESAMPLE_SRC
		 microprofile/microprofile.h
		 microprofile/microprofileui.h
		 imgui/imgui.h
		 imgui/imgui.cpp
		 imgui/imgui_internal.h
		 imgui/imgui_demo.cpp
		 imgui/imgui_draw.cpp
		 imgui/imgui_widgets.cpp
		 imgui/imconfig.h
		 imgui/examples/imgui_impl_glfw.h
		 imgui/examples/imgui_impl_glfw.cpp
		 imgui/examples/imgui_impl_opengl2.h
		 imgui/examples/imgui_impl_opengl2.cpp
		 enkiTSMicroprofileExample.cpp
		 )
	add_executable( enkiTSMicroprofileExample WIN32 ${ENKITSMICROPROFILESAMPLE_SRC} )
	target_link_libraries(enkiTSMicroprofileExample enkiTS ${OPENGL_LIBRARIES} glfw ${GLFW_LIBRARIES} )
endif()

if( MSVC )
    if(${CMAKE_VERSION} VERSION_LESS "3.6.0") 
        message( "\n\t[ WARNING ]\n\n\tCMake version lower than 3.6.\n\n\t - Please update CMake and rerun; OR\n\t - Manually set one of the 'Example' projects as StartUp Project in Visual Studio.\n" )
    else()
        set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT enkiTSMicroprofileExample )
    endif()
endif()