# CMakeLists.txt for WST third-party packages

add_subdirectory(acro)
add_subdirectory(epanet)
#add_subdirectory(pyepanet)
add_subdirectory(sundry)
#add_subdirectory(vpykit)

#add_subdirectory(lzma)
add_library(lzma
  lzma/C/Alloc.c
  lzma/C/LzFind.c
  lzma/C/LzmaDec.c
  lzma/C/LzmaEnc.c
  lzma/C/LzmaLib.c
)
target_compile_definitions(lzma PRIVATE -D_7ZIP_ST) 
target_include_directories(lzma INTERFACE lzma/C)

