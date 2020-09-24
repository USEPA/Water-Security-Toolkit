project(Merlion)

add_library(merlion
  src/merlion/TriSolve.cpp
  src/merlion/BlasWrapper.cpp
  src/merlion/DirectedGraph.cpp
  src/merlion/SparseMatrix.cpp
  src/merlion/Network.cpp
  src/merlion/WaterQualityModel.cpp
  src/merlion/Merlion.cpp
  )
target_include_directories(merlion PUBLIC src)

add_library(merlionUtils
  merlionUtils/src/merlionUtils/EpanetLinker.cpp 
  merlionUtils/src/merlionUtils/ModelWriter.cpp 
  merlionUtils/src/merlionUtils/TSG_Reader.cpp 
  merlionUtils/src/merlionUtils/SimTools.cpp 
  merlionUtils/src/merlionUtils/Scenarios.cpp 
  merlionUtils/src/merlionUtils/NetworkSimulator.cpp
  )
target_link_libraries(merlionUtils merlion epanet2)
target_include_directories(merlionUtils PUBLIC merlionUtils/src)

add_subdirectory(applications)