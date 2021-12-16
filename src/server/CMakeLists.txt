
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
		Server.cpp
		Server.hpp
		)

target_compile_definitions(server
	PRIVATE NO_OPENSSL
	)

if(NOT LIVEMEDIA_PRIVATE_INCLUDE_DIR)
	set(LIVEMEDIA_PRIVATE_INCLUDE_DIR "/usr/local/include")
endif()

if(NOT LIVEMEDIA_PRIVATE_LIB_DIR)
	set(LIVEMEDIA_PRIVATE_LIB_DIR "/usr/local/lib")
endif()

target_include_directories(server
		PRIVATE ${LIVEMEDIA_PRIVATE_INCLUDE_DIR}/UsageEnvironment
		PRIVATE ${LIVEMEDIA_PRIVATE_INCLUDE_DIR}/groupsock
		)

target_link_directories(server
		PUBLIC ${LIVEMEDIA_PRIVATE_LIB_DIR}
		)

add_dependencies(server
		tuners
		)

target_link_libraries(server
		tuners
		liveMedia
		groupsock
		BasicUsageEnvironment
		UsageEnvironment
		)
