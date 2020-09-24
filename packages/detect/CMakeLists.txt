add_library(detect
  Detection.cpp
)
target_link_libraries(detect tsoutil)
target_include_directories(detect PUBLIC .)