add_subdirectory(anl)
add_subdirectory(tso)
add_subdirectory(erd_jni)
add_subdirectory(tso_jni)

#add_subdirectory(erd)
add_library(erd
  erd/erd/src/dpx.c
  erd/erd/src/erd.c
  erd/erd/src/erdinternal.c
  erd/erd/src/lzma_dec.c
  erd/erd/src/lzma_enc.c
  erd/erd/src/rle_dec.c
  erd/erd/src/rle_enc.c
  erd/erd/src/teva.c
  )
target_include_directories(erd
  PUBLIC erd/erd/include
)
target_link_libraries(erd lzma)

add_library(enl
  erd/enl/src/enl.c
  )
target_link_libraries(enl erd epanetmsx)
target_include_directories(enl
  PUBLIC erd/enl/include
)



add_executable(tso2text
  tso/src/tso2text.c
  )
target_link_libraries(tso2text tsoutil erd)

add_executable(erddiff
  tso/src/tsodiff.c
  )
target_link_libraries(erddiff tsoutil erd)

add_executable(erdinfo
  tso/src/tso_info.c
  )
target_link_libraries(erdinfo tsoutil erd)


