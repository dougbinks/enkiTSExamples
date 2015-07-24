cmake_minimum_required(VERSION 2.8)

project( enkiTSExamples )

include_directories("enkiTS/src")
include_directories("Remotery/lib")

set( ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "Build basic example applications")

add_subdirectory( enkiTS )

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