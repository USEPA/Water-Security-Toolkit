add_executable(boostersim
  booster/src/booster/BoosterNetworkSimulator.cpp
  booster/src/booster/BoosterDataWriter.cpp
  booster/src/booster/BoosterDataWriterAMPL.cpp
  booster/src/booster/BoosterDataWriterPYOMO.cpp
  booster/src/booster/BoosterDataWriterPYSP.cpp
  booster/src/booster/BoosterSimOptions.cpp
  booster/src/booster/BoosterOptions.cpp
  booster/src/booster/BoosterSim.cpp
  )
target_include_directories(boostersim PRIVATE booster/src)
target_link_libraries(boostersim merlionUtils utilib)

add_executable(boosterquality
  booster/src/booster/BoosterNetworkSimulator.cpp
  booster/src/booster/BoosterDataWriter.cpp
  booster/src/booster/BoosterDataWriterAMPL.cpp
  booster/src/booster/BoosterDataWriterPYOMO.cpp
  booster/src/booster/BoosterQualityOptions.cpp
  booster/src/booster/BoosterOptions.cpp
  booster/src/booster/BoosterQuality.cpp
  )
target_include_directories(boosterquality PRIVATE booster/src)
target_link_libraries(boosterquality merlionUtils utilib)

add_executable(boosterimpact
  booster/src/booster/BoosterNetworkSimulator.cpp
  booster/src/booster/BoosterImpactOptions.cpp
  booster/src/booster/BoosterOptions.cpp
  booster/src/booster/BoosterImpact.cpp
  )
target_include_directories(boosterimpact PRIVATE booster/src)
target_link_libraries(boosterimpact merlionUtils utilib)

add_executable(erdval
  erdvalidate/src/erdvalidate/erdvalidate.cpp
  )
target_link_libraries(erdval merlionUtils tsoutil)

add_executable(inversionsim
  source_inversion/src/source_inversion/InversionDataWriter.cpp
  source_inversion/src/source_inversion/InversionSimOptions.cpp 
  source_inversion/src/source_inversion/InversionNetworkSimulator.cpp 
  source_inversion/src/source_inversion/InversionSim.cpp
  )
target_include_directories(inversionsim PRIVATE source_inversion/src)
target_link_libraries(inversionsim PRIVATE utilib merlionUtils enl tsoutil)

add_executable(measuregen
  measure_gen/src/measure_gen/MeasureDataWriter.cpp
  measure_gen/src/measure_gen/MeasureSimOptions.cpp
  measure_gen/src/measure_gen/MeasureNetworkSimulator.cpp
  measure_gen/src/measure_gen/MeasureSim.cpp
  )
target_include_directories(measuregen PRIVATE measure_gen/src)
target_link_libraries(measuregen PRIVATE utilib merlionUtils)

add_executable(samplelocation
  grab_sample_location/src/grab_sample_location/samplelocation.cpp 
  grab_sample_location/src/grab_sample_location/SampleLocationOptions.cpp 
  grab_sample_location/src/grab_sample_location/SampleNetworkSimulator.cpp 
  grab_sample_location/src/grab_sample_location/SampleLocationDataWriter.cpp
  )
target_include_directories(samplelocation PRIVATE grab_sample_location/src)
target_link_libraries(samplelocation PRIVATE utilib merlionUtils tsoutil)

add_executable(eventDetection
  event_detection/src/event_detection/EventDetectionOptions.cpp
  event_detection/src/event_detection/EventDetection.cpp
  )
target_include_directories(eventDetection PRIVATE event_detection/src)
target_link_libraries(eventDetection PRIVATE utilib merlionUtils epanetmsx )

add_executable(jsonwriter
  json_writer/src/json_writer/json_main.cpp
  )
target_link_libraries(jsonwriter PRIVATE epanet2)

add_executable(jsonwriter2
  json_writer2/src/json_writer2/json2_main.cpp
  )
target_link_libraries(jsonwriter2 PRIVATE tsoutil)

#add_executable(samplelocation
#  )
#target_include_directories(samplelocation PRIVATE grab_sample_location/src)
#target_link_libraries(samplelocation PRIVATE utilib merlionUtils)
