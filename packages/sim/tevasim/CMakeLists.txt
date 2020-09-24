project(tevasim)

add_executable(tevasim
  teva.cpp 
  run_teva.cpp
)
target_link_libraries(tevasim enl tsoutil merlionUtils epanet2 epanetmsx)