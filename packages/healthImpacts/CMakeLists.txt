
add_library(threatassess
  threatAssess/hia.c
  threatAssess/nrutil.c
  threatAssess/sss.c
  threatAssess/tai.c
  threatAssess/ta_input.c
  threatAssess/ta_utils.c
  threatAssess/dose.c
  threatAssess/ingest.c
  threatAssess/response.c
  threatAssess/runif_if.c
  threatAssess/spline_if.c
  threatAssess/splines.c
  threatAssess/mtwist-0.6/mtwist.c
  dcdfLib/dcdflib.c
  )
target_include_directories(threatassess 
  PUBLIC threatAssess/mtwist-0.6 hia/include
  PRIVATE dcdfLib 
  INTERFACE threatAssess cat/include)
target_link_libraries(threatassess tsoutil)

# Because of the dependence on ANL_UTIL_Log, HIAModule can only be built
# with JNI.
if ( HAVE_JNI )
  add_library(HIAModule
    hia/src/HealthImpactsAggr.c  
    hia/src/HealthImpactsIA.c  
    hia/src/HealthImpactsJNI.c 
    hia/src/HealthImpactsUtil.c 
    hia/include/HealthImpacts.h
    )
  target_link_libraries(HIAModule threatassess AnalysisModule)
endif()

add_executable(threat_assess
  threatAssess/teva-assess.c
  )
target_link_libraries(threat_assess threatassess)
