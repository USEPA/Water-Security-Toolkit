project(ERD_JNI)

if ( HAVE_JNI )
  add_library(ERD_JNI
    erd_jni.c
    erd_utils.c
    )
  target_link_libraries(ERD_JNI AnalysisModule)
  target_include_directories(ERD_JNI INTERFACE .)
endif()
