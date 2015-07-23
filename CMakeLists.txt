cmake_minimum_required(VERSION 2.8)

project( enkiTSExamples )

include_directories("enkiTS/src")
include_directories("Remotery/lib")

set( ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "Build basic example applications")

add_subdirectory( enkiTS )

set( ENKITSREMOTERYSAMPLE_SRC
     EnkiTSRemoteryExample.cpp
     Remotery/lib/Remotery.h
     Remotery/lib/Remotery.c
     )
	 
add_executable( EnkiTSRemoteryExample ${ENKITSREMOTERYSAMPLE_SRC} )
target_link_libraries(EnkiTSRemoteryExample enkiTS )

if( ENKITS_BUILD_C_INTERFACE )
	set( ENKITSREMOTERYSAMPLE_C_SRC
		 EnkiTSRemoteryExample.c
		 Remotery/lib/Remotery.h
		 Remotery/lib/Remotery.c
		 )
	add_executable( EnkiTSRemoteryExample_C ${ENKITSREMOTERYSAMPLE_C_SRC} )
	target_link_libraries(EnkiTSRemoteryExample_C enkiTS )
endif()	