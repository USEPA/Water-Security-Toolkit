
if ( HAVE_JNI )
  add_library(epanetmsx_jni
    EPANETMSXJava.c
    )
  target_link_libraries(epanetmsx_jni epanetmsx AnalysisModule)
endif()
