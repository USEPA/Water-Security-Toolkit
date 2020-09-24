add_library(grasp_common STATIC
  basics.cpp
  bossa_random.cpp 
  bossa_timer.cpp 
  constructive.cpp 
  cover_instance.cpp 
  distance.cpp 
  elite.cpp 
  euclidean_instance.cpp 
  geo_instance.cpp 
  graph_instance.cpp 
  hybrid_instance.cpp
  impact_instance.cpp 
  ImpactCache.cpp 
  instance_factory.cpp 
  matrix_instance.cpp
  path_relink.cpp 
  search.cpp 
  search_tables.cpp 
  solution.cpp
  )
target_include_directories(grasp_common PUBLIC .)
target_link_libraries(grasp_common utilib)


add_executable(grasp grasp.cpp )
target_link_libraries(grasp grasp_common utilib)

add_executable(pmgen pmgen.cpp )
target_link_libraries(pmgen grasp_common)

add_executable(impact2pmm impact2pmm.cpp )
target_link_libraries(impact2pmm grasp_common utilib)

add_executable(randomsample randomsample.cpp )
target_link_libraries(randomsample grasp_common sp)

add_executable(sideconstraints sideconstraints.cpp )
target_link_libraries(sideconstraints grasp_common sp)

add_executable(imperfect imperfect.cpp )
target_link_libraries(imperfect grasp_common sp)
