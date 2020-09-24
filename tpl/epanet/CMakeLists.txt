project(EPANET C CXX)


add_library(epanet2
  EPANET_/src/epanet.c
  EPANET_/src/hash.c
  EPANET_/src/hydraul.c
  EPANET_/src/inpfile.c
  EPANET_/src/input1.c
  EPANET_/src/input2.c
  EPANET_/src/input3.c
  EPANET_/src/mempool.c
  EPANET_/src/output.c
  EPANET_/src/quality.c
  EPANET_/src/report.c
  EPANET_/src/rules.c
  EPANET_/src/smatrix.c
  )
target_include_directories(epanet2 PUBLIC EPANET_/include INTERFACE EPANET_/src)
target_link_libraries(epanet2 m) 

add_executable(epanet EPANET_/src/epanet.c)
target_compile_definitions(epanet PRIVATE -DCLE) 
target_link_libraries(epanet epanet2) 

add_library(epanetmsx
  EPANET_MSX_/src/mathexpr.c 
  EPANET_MSX_/src/msxchem.c 
  EPANET_MSX_/src/msxcompiler.c 
  EPANET_MSX_/src/msxfile.c 
  EPANET_MSX_/src/msxfuncs.c 
  EPANET_MSX_/src/msxinp.c 
  EPANET_MSX_/src/msxout.c 
  EPANET_MSX_/src/msxproj.c 
  EPANET_MSX_/src/msxqual.c 
  EPANET_MSX_/src/msxrpt.c 
  EPANET_MSX_/src/msxtank.c 
  EPANET_MSX_/src/msxtoolkit.c 
  EPANET_MSX_/src/msxutils.c 
  EPANET_MSX_/src/newton.c 
  EPANET_MSX_/src/rk5.c 
  EPANET_MSX_/src/ros2.c
  )
target_include_directories(epanetmsx PUBLIC EPANET_MSX_/include)
target_link_libraries(epanetmsx epanet2 dl) 

add_executable(epanet_msx EPANET_MSX_/src/msxmain.c)
target_link_libraries(epanet_msx epanetmsx) 

add_library(epanetbtx
  EPANET_BTX_/src/backtrack.c 
  EPANET_BTX_/src/btxtoolkit.c 
  EPANET_BTX_/src/btxutil.c
)
target_include_directories(epanetbtx BEFORE PUBLIC EPANET_BTX_/include)
target_link_libraries(epanetbtx epanet2) 
