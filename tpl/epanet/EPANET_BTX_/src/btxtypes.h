#include <stdio.h>
#define   TINY      1.E-6
#define	  PI	   3.14159
#define   Diffus    1.3E-8   /* Diffusivity of chlorine                */
                             /* @ 20 deg C (sq ft/sec)                 */
#define   Viscos    1.1E-5   /* Kinematic viscosity of water           */
                             /* @ 20 deg C (sq ft/sec)                 */
#define QUALITYSTEP  60
#define TOTALCRI   0.001

#define MIXED 0
#define MIX2 1
#define FIFO 2
#define LIFO 3


#define   LPSperCFS   28.317
#define   MperF		  0.3048

#define  Absolute(x)   ((x>=0.0) ? x:-x)



enum ErrorCodeType                    // Error codes (501-515)
          {ERR_FIRST = 600,
           ERR_MEMORY,                 // 601
           ERR_OPEN_HYD_FILE,          // 602
           ERR_READ_HYD_FILE,          // 603
		   ERR_BACKTRACK_TIME,		   // 604
		   ERR_INVALID_SOURCETYPE,     // 605
		   ERR_INVALID_INPUT,          // 606
		   ERR_INVALID_NODE,		   // 607
		   ERR_INVALID_LINK,		   // 608	
		   ERR_INVALID_TIME,       // 609
		   ERR_INVALID_PATH,		   // 610
           ERR_MAX};

struct Sadj
{
	unsigned int linkindex; //link connecting to adjacent node
	unsigned int nodeindex; //adjacent node
	int flag;		/*if the node represented by the structure is fromnode flag=1 */
	struct Sadj *next;	//next adjacent node
};
typedef struct Sadj *Padj; 

struct Spipe
{
	int link; //link index
	struct Spipe * pre;
}; 

struct Sparticle
{
	int link;	/*link index*/
	float x;	/*distance in link from From node or volume of fresher/older water in FIFO/LIFO tank*/
	float a;    /*impact coefficient*1000*/
	float t;    /*time*/
	struct Sparticle *previous;
	struct Sparticle *next;
};



struct Stank
{
	int nodeindex;
	int mixtype;
	float mixvolume;
	float initvolume;
	float decay_coeff;
};

struct Snode
{
	int inputflag;
	float * sourcestrength;
	int sourcelength;
	float patternstep;
	float * impact;
	int sourcetype;
};

typedef struct {
  int nnodes;
  int nlinks;
  int ntanks;
  int nsteps; /*number of hydraulic period*/
  int nimpactsteps;
  float impactstep;
  long dur;
} Snet;

struct Spath{
	int endnodeindex;
	float timedelay;
	float impactcoefficient;
	struct Spath *next;
};

 // BTX PROJECT VARIABLES
typedef struct                        
{
   FILE * HydFile;                     // EPANET hydraulics file
   int	HydOffset;
   float LUcf;							/*Length unit conversion factor*/   
   Padj * ADJ;		/* Node adjacency lists         */
   struct Stank * Tank;
   struct Snode * Node;
   Snet Network;
   float * TankImpactCoeff;
   float * TimeofTankImpact;
   float * TankMainImpactCoeff;
   float ** TankVolume;
   float ** LinkFlow;
   long * SaveTime;
   long * SaveStep;
   int * PathFlag;
   float Time;
   float Outtime;
   float TotalErased;
   int Nstep;
   int HydrauInforFlag;
   int ParticleNumber;
   int PathNumber;
   struct Sparticle *ParticleFirst;
   struct Sparticle *ParticleLast;
   struct Sparticle *FreeParticle;
   struct Spath *FirstPath;
   struct Spath *LastPath;
   float Erasecri; 
} BTXproject;

