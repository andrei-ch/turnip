
add_library(sdrplay
		api.cpp
		api.hpp
		base_stream.cpp
		base_stream.hpp
		device.cpp
		device.hpp
		stream.cpp
		stream.hpp
		)

if(NOT SDRPLAY_API_LIB_DIR)
	set(SDRPLAY_API_LIB_DIR "/usr/local/lib")
endif()

target_link_directories(sdrplay
		PUBLIC ${SDRPLAY_API_LIB_DIR}
		)

target_link_libraries(sdrplay
		sdrplay_api
		)
