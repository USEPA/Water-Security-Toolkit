add_library(csa
  src/CSA.c
  )
target_include_directories(csa BEFORE PUBLIC include )
target_link_libraries(csa epanetbtx)

add_executable(csatest
  test/csatest.cpp
  )
target_link_libraries(csatest csa)

add_executable(csarun
  CSA_run/csarun.cpp
  CSA_run/CSArunOptions.cpp
  )
target_link_libraries(csarun csa utilib)
target_include_directories(csa BEFORE PUBLIC CSA_run )
