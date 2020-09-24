project(tsoutil)

add_library(tsoutil
  src/tso.c
  )
target_include_directories(tsoutil
  PUBLIC src
)
target_link_libraries(tsoutil enl utilib)
