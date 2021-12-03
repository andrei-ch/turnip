
add_library(tuners
		AMTuner.cpp
		AMTuner.hpp
		BaseTuner.cpp
		BaseTuner.hpp
		FMTuner.cpp
		FMTuner.hpp
		HDRadioTuner.cpp
		HDRadioTuner.hpp
		TunerParams.cpp
		TunerParams.hpp
		TunerWithQueue.hpp
		)

add_dependencies(tuners
		easysdr
		)

target_link_libraries(tuners
		easysdr
		)
