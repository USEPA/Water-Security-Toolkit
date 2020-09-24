
if ( HAVE_JNI )
  add_library(epanet_jni
    EPANETWaterModel.c
    EPANETJava.c
    )
  target_link_libraries(epanet_jni epanet2 AnalysisModule)
endif()
