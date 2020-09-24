/*Functions that can be called in control toolbox*/
#undef WINDOWS
#ifdef _WIN32
  #define WINDOWS
#endif
#ifdef __WIN32__
  #define WINDOWS
#endif

// --- define DLLEXPORT

#ifdef WINDOWS
  #ifdef __cplusplus
  #define DLLEXPORT extern "C" __declspec(dllexport) _stdcall
  #else
  #define DLLEXPORT __declspec(dllexport) _stdcall
  #endif
#else
  #ifdef __cplusplus
  #define DLLEXPORT extern "C"
  #else
  #define DLLEXPORT
  #endif
#endif  



//int DLLEXPORT BTXopen(char *);
//int DLLEXPORT BTXinit(float, float);
//int DLLEXPORT BTXsetinput(char *, int type);
//int DLLEXPORT BTXsetinputstrength(char *, float*, int, float);
//int DLLEXPORT BTXsim(char *, float);
//int DLLEXPORT BTXgetimpact(char *, float *, int);
//int DLLEXPORT BTXout(float *);
//int DLLEXPORT BTXgetpathnumber(int *);
//int DLLEXPORT BTXgetpathinfor(int, int *, float *, float *);
//int DLLEXPORT BTXpipeinpath(char *, int *);
//int DLLEXPORT BTXclose();


int BTXopen(char *);
int BTXinit(float, float);
int BTXsetinput(char *, int type);
int BTXsetinputstrength(char *, float*, int, float);
int BTXsim(char *, float);
int BTXgetimpact(char *, float *, int);
int BTXout(float *);
int BTXgetpathnumber(int *);
int BTXgetpathinfor(int, int *, float *, float *);
int BTXpipeinpath(char *, int *);
int BTXclose();

/*


BTX TO DO:
 
 - remove epanet simulation stuff. replace with hydraulic "force-feed" state methods
 - 




*/
