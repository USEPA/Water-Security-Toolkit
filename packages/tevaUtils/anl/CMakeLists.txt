project(AnalysisModule)

if ( HAVE_JNI )
  add_library(AnalysisModule
    src/namedData.c
    src/AggregationServerJNI.c
    src/IntAnalysisServerJNI.c 
    src/AnalysisModuleUtils.c 
    src/DiskCachedData.c
    )

  target_include_directories(AnalysisModule
    PUBLIC include ANLUtils/include ../erd/erd/include ${JNI_INCLUDE_DIRS}
    )
endif()
