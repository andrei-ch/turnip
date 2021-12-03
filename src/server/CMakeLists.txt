
add_library(server
		ModemAudioSource.cpp
		ModemAudioSource.hpp
		ModemAudioSourceParams.cpp
		ModemAudioSourceParams.hpp
		ModemContext.cpp
		ModemContext.hpp
		OnDemandMetadataSubsession.cpp
		OnDemandMetadataSubsession.hpp
		OnDemandModemSubsession.cpp
		OnDemandModemSubsession.hpp
		ProgramMetadataSource.cpp
		ProgramMetadataSource.hpp
		RTSPServer.cpp
		RTSPServer.hpp
		)

target_compile_definitions(server
	PRIVATE NO_OPENSSL
	)

if(NOT LIVEMEDIA_PRIVATE_INCLUDE_DIR)
	if(APPLE)
		set(LIVEMEDIA_PRIVATE_INCLUDE_DIR "/usr/local/include")
	else()
		set(LIVEMEDIA_PRIVATE_INCLUDE_DIR "/usr/include")
	endif()
endif()

target_include_directories(server
		PRIVATE ${LIVEMEDIA_PRIVATE_INCLUDE_DIR}/UsageEnvironment
		PRIVATE ${LIVEMEDIA_PRIVATE_INCLUDE_DIR}/groupsock
		)

add_dependencies(server
		tuners
		)

target_link_libraries(server
		tuners
		groupsock
		liveMedia
		BasicUsageEnvironment
		UsageEnvironment
		)
