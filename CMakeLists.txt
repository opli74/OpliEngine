# Set which version of cmake we need, will throw an error if this check fails.
cmake_minimum_required(VERSION 3.16...3.23)

# Set which version of C++ we want to use
set(CMAKE_CXX_STANDARD 20)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("OpliEngine")

include_directories( "src/inc/d3d/" )
include_directories( "src/inc/" )
include_directories( "src/lib/" )

add_definitions(-DUNICODE -D_UNICODE)

# Get our files that we want via GLOB. 
	# GLOB: Searches for our files we specify
	# src_files: 
file (GLOB src CMAKE_CONFIGURE_DEPENDS
	src/*.h
	src/*.cpp
		src/lib/*.cpp
		src/inc/*.h
			src/inc/d3d/*.h

)

add_executable(${PROJECT_NAME} WIN32 ${src} )
