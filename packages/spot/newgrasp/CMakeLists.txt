add_library(new_grasp_common STATIC
  ImpactCache.cpp
  ProblemInstance.cpp
  SideConstraintInstance.cpp
  ImperfectInstance.cpp
  ProblemSolver.cpp
  solution.cpp
  InitialSolution.cpp
  MeanSolution.cpp
  WorstSolution.cpp
  VarSolution.cpp
  TceSolution.cpp
  PreProcess.cpp
  )
target_link_libraries(new_grasp_common sp)


add_executable(new_randomsample new_randomsample.cpp )
target_link_libraries(new_randomsample new_grasp_common)

add_executable(new_imperfect new_imperfect.cpp )
target_link_libraries(new_imperfect new_grasp_common)
