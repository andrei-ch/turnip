cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

project(Turnip VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(APPLE)
	add_compile_options(-Wno-return-type-c-linkage)
endif()

if(NOT APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

add_compile_options(-Wno-deprecated-declarations)

if(NOT BOOST_INCLUDE_DIR)
	set(BOOST_INCLUDE_DIR "/usr/local/include")
endif()

include_directories(SYSTEM
		${PROJECT_BINARY_DIR}/include
		${PROJECT_SOURCE_DIR}/src
		${BOOST_INCLUDE_DIR}
		)

configure_file(src/version.h.in include/version.h)

add_subdirectory(src/easysdr)
add_subdirectory(src/sdrplay)
add_subdirectory(src/server)
add_subdirectory(src/tuners)

add_executable(turnip
		src/main.cpp
		)

add_dependencies(turnip
		sdrplay
		server
		)

target_link_libraries(turnip
		sdrplay
		server
		)

install(TARGETS turnip)
