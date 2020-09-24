// --- define DLLEXPORT
//Annamaria De Sanctis - vers. 1 mod.Sept2009
#ifndef CSA_H
#define CSA_H
#undef WINDOWS

#ifdef _WIN32
  #define WINDOWS
#endif
#ifdef __WIN32__
  #define WINDOWS
#endif

#ifndef DLLEXPORT
 #ifdef WINDOWS
   #ifdef __cplusplus
   #define DLLEXPORT extern "C" int __declspec(dllexport) __stdcall
   #else
   #define DLLEXPORT __declspec(dllexport) __stdcall
   #endif
 #else
  #ifdef __cplusplus
  #define DLLEXPORT extern "C"
  #else
  #define DLLEXPORT
  #endif
 #endif
#endif  

//BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);
//int DLLEXPORT CSAopen(double, double, int);
//int DLLEXPORT CSAhydraulics(int);
//int DLLEXPORT CSAaddsensor(int, char *);
//int DLLEXPORT CSAsetbinary(double, char *);
//int DLLEXPORT CSAupdate();
//int DLLEXPORT CSAcandidates();
//int DLLEXPORT CSAclose();

int CSAopen(double, double, float, float);
int CSAhydraulics(int);
int CSAaddsensor(int, char *);
int CSAsetbinary(double, char *);
int CSAupdate(char *);
int CSAcandidates(char *);
int CSAclose();

#endif

