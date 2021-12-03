
add_library(easysdr
		core/Graph.cpp
		core/Graph.hpp
		core/Latch.cpp
		core/Latch.hpp
		core/Metadata.cpp
		core/Metadata.hpp
		core/Node.hpp
		core/Queue.cpp
		core/Queue.hpp
		core/QueueIn.hpp
		core/QueueOut.hpp
		nodes/AudioAutoGain.cpp
		nodes/AudioAutoGain.hpp
		nodes/Convert.cpp
		nodes/Convert.hpp
		nodes/DecodeNRSC5.cpp
		nodes/DecodeNRSC5.hpp
		nodes/DemodulateAM.cpp
		nodes/DemodulateAM.hpp
		nodes/DemodulateFM.cpp
		nodes/DemodulateFM.hpp
		nodes/DemodulateFMS.cpp
		nodes/DemodulateFMS.hpp
		nodes/FrequencyShift.cpp
		nodes/FrequencyShift.hpp
		nodes/Liquid.hpp
		nodes/LiquidImpl.hpp
		nodes/MP3Encode.cpp
		nodes/MP3Encode.hpp
		nodes/MuxFMS.cpp
		nodes/MuxFMS.hpp
		nodes/Resample.cpp
		nodes/Resample.hpp
		nodes/SDRPlayInput.cpp
		nodes/SDRPlayInput.hpp
		nodes/WAVFileOutput.cpp
		nodes/WAVFileOutput.hpp
		)

target_compile_definitions(easysdr
	PRIVATE BOOST_BIND_GLOBAL_PLACEHOLDERS
	)

if(NOT APPLE)
	target_compile_options(easysdr
		PRIVATE -Wno-multichar
		)
endif()

add_dependencies(easysdr
		sdrplay
		)

target_link_libraries(easysdr
		sdrplay
		liquid
		mp3lame
		nrsc5
		)
