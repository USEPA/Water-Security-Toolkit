// --- define DLLEXPORT
//Annamaria De Sanctis - vers. 1 mod.Sept2009
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
//BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);
#ifdef WINDOWS
_declspec (dllexport) int CSAopen(double, float, int);
_declspec (dllexport) int CSAhydraulics(int);
 _declspec (dllexport) int CSAaddsensor(int, char *);
 _declspec (dllexport) int CSAsetbinary(float, char *);
 _declspec (dllexport) int CSAupdate();
 _declspec (dllexport) int CSAcandidates();
 _declspec (dllexport) int CSAclose();
#else
int CSAopen(double, float, int);
int CSAhydraulics(int);
int CSAaddsensor(int, char *);
int CSAsetbinary(float, char *);
int CSAupdate();
int CSAcandidates();
int CSAclose();
#endif

