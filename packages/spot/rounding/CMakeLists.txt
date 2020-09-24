Check_Include_Files(gmp.h HAVE_GMP)
if(HAVE_GMP)
  add_executable(SNLRound
    SNLRound.cpp
    )
endif()