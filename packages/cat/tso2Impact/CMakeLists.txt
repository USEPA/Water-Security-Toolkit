add_library(sp
  impacts.cpp 
  scenario.cpp 
  SPProblem.cpp 
  tso2ImpactAPI.cpp  
  EventImpacts.cpp 
  )
target_link_libraries(sp utilib epanet2 tsoutil threatassess detect)
target_include_directories(sp PUBLIC ../../sim/tevasim .)


add_executable(tso2Impact
  tso2Impact.cpp
)
target_link_libraries(tso2Impact sp)
target_include_directories(tso2Impact PRIVATE ../../sim/tevasim)

add_executable(erd2Signals
  erd2Signals.cpp
)
target_link_libraries(erd2Signals sp)
target_include_directories(erd2Signals PRIVATE ../../sim/tevasim)


add_executable(evalsensor
  evalsensor.cpp
)
target_link_libraries(evalsensor sp)
target_include_directories(evalsensor PRIVATE ../../sim/tevasim)


add_executable(createIPData
  createIPData.cpp
)
target_link_libraries(createIPData sp)


add_executable(createLagData
  createLagData.cpp
)
target_link_libraries(createLagData sp)
#target_include_directories(createLagData PRIVATE sp)


add_executable(aggregateImpacts
  aggregateImpacts.cpp
)
target_link_libraries(aggregateImpacts sp)
target_include_directories(aggregateImpacts PRIVATE ../../sim/tevasim)


add_executable(preprocessImpacts
  preprocessImpacts.cpp
)
target_link_libraries(preprocessImpacts sp)
#target_include_directories(preprocessImpacts PRIVATE sp ../../sim/tevasim)


add_executable(scenarioAggr
  scenarioAggr.cpp
)
target_link_libraries(scenarioAggr sp)
#target_include_directories(scenarioAggr PRIVATE sp ../../sim/tevasim)
