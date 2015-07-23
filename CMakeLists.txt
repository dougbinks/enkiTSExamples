cmake_minimum_required(VERSION 2.8)

project( enkiTSExamples )

include_directories("enkiTS/src")
include_directories("Remotery/lib")

set( ENKITS_BUILD_C_INTERFACE  OFF CACHE BOOL "Build C interface")
set( ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "Build example applications")

add_subdirectory( enkiTS )

set( ENKITSREMOTERYSAMPLE_SRC
     EnkiTSRemoteryExample.cpp
     Remotery/lib/Remotery.h
     Remotery/lib/Remotery.c
     )
	 
add_executable( EnkiTSRemoteryExample ${ENKITSREMOTERYSAMPLE_SRC} )
target_link_libraries(EnkiTSRemoteryExample enkiTS )