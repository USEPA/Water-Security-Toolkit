project(TSO)

if ( HAVE_JNI )
  add_library(TSO
    tso_jni.c
    )
  target_link_libraries(TSO tsoutil ERD_JNI AnalysisModule)
endif()
