#ifndef _ERD2SCADA_H
#define _ERD2SCADA_H

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#define MAXFNAME 1024
#define MAX_TAG_LEN 1024
#define MAX_DB_ADDR 2048
#define MAXQUERY 1024
#define TRUE 1
#define FALSE 0

#ifdef WIN32
  #ifdef __cplusplus
  #define LIBEXPORT extern "C" __declspec(dllexport) __stdcall
  #else
  #define LIBEXPORT __declspec(dllexport) __stdcall
  #endif
#else
#define LIBEXPORT
#endif


typedef enum signalTypes
{
	OTHER,
	FLOW,
	PRESSURE,
	VALVESTAT,
	ALARM,
	WQ
} signalTypes;



typedef struct 
{	
	int epanetIndex;
	char epanetID[MAX_TAG_LEN];
	char shortID[MAX_TAG_LEN];
	char scadaID[MAX_TAG_LEN];
	signalTypes signalType;
	char units[MAX_TAG_LEN];
	char description[MAX_TAG_LEN];
} SCADAtag, *pSCADAtag;


typedef struct
{
	pSCADAtag* tags;			// array of scada tags
	char dbAddress[MAX_DB_ADDR];		// database info for connection
	int numTags;				// number of tags, for convenience.
}SCADAinfo, *pSCADAinfo;



pSCADAinfo LIBEXPORT initializeSCADAinfo(char* xmlConfigFile);
int LIBEXPORT getTagIndexByScada(char* myScadaID, pSCADAinfo mySCADAinfo);


#endif

