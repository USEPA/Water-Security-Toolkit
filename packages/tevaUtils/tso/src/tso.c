/*
 * Copyright � 2008 UChicago Argonne, LLC
 * NOTICE: This computer software, TEVA-SPOT, was prepared for UChicago Argonne, LLC
 * as the operator of Argonne National Laboratory under Contract No. DE-AC02-06CH11357
 * with the Department of Energy (DOE). All rights in the computer software are reserved
 * by DOE on behalf of the United States Government and the Contractor as provided in
 * the Contract.
 * NEITHER THE GOVERNMENT NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
 * ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
 *
 * This software is distributed under the BSD License.
 */
#include "tevautil.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#else
#if defined(__linux__) || defined(__CYGWIN__) || defined(__APPLE__)
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>
#endif
#endif

/* methods local to this file*/
static int x_TSO_WriteDemands(PTSO tso, PNetInfo net,PNodeInfo nodes);
static PTSO x_TSO_Open(char *filenameOrDirectory, char *pattern, char fileVersion, char storageMethod) ;
static void x_TSO_ReadSizes(PTSO tso, PNetInfo *net);
static int x_TSO_WriteDemands(PTSO tso, PNetInfo net,PNodeInfo nodes);
static int x_TSO_WriteDemandProfiles(PTSO tso, PNetInfo net,PNodeInfo nodes);
static int x_compressAndStore2(float *dtc,int nsteps,FILE *stream);
static int x_compressAndStore3(float *dtc,int nsteps,FILE *stream);
static int x_TSO_eof(PTSO tso);
static void MEMCHECK(void *x,char *msg);
static void tsoerror(int exitCode,const char *errmsg, ...);
static int x_TSO_ReadSources(FILE *fpTSO, int *nsources,PSource *sources, PNodeInfo nodes, int fileVersion);
static int x_TSO_ReadSimulationResults(FILE *fpTSO, PTSO tso, PNetInfo net, PNodeInfo nodes);


static void MEMCHECK(void *x,char *msg) {
	if(x == NULL)
		tsoerror(1,"Allocating memory: %s",msg);
}

/*
**--------------------------------------------------------------
**  tsoerror
**  Input:   exit code
**           errmsg: printf-like varargs for the error message
**           to print before exiting
**  Output:  none
**  Purpose: Print an error message and exit the program
**--------------------------------------------------------------
*/
static void tsoerror(int exitCode,const char *errmsg, ...)
{
	va_list ap;

	fprintf(stderr,"\n\n********************************************************************************\n\n");
	va_start(ap,errmsg);
	vfprintf(stderr,errmsg,ap);
	va_end(ap);
	fprintf(stderr,"\n\n********************************************************************************\n\n");

	if(exitCode >= 0) exit(exitCode);
}
static void x_TSO_getBaseNameAndDirectory(PTSO tso, char *fn)
{
	char *si,*ts;
	int len;
	si = strrchr(fn,'\\');
	if(si == NULL) si = strrchr(fn,'/');
	if(si == NULL) {
		MEMCHECK(tso->tsodir = (char *)calloc(2,sizeof(char)),"tso->tsodir in x_TSO_Open");
		tso->tsodir[1]='\0';
		tso->tsodir[0]='.';
		si=&fn[0];
	} else {
		len=(int)(si-fn);
		MEMCHECK(tso->tsodir = (char *)calloc(len+1,sizeof(char)),"tso->tsodir in x_TSO_Open");
		memcpy(tso->tsodir,fn,len);
		tso->tsodir[len] = '\0';
		si++;
	}
	ts = strrchr(si,'.');
	if(ts != 0)
		len = (int)(ts-si);
	else
		len = (int)strlen(si);
	tso->baseName = (char *)calloc(len+1,sizeof(char));
	memcpy(tso->baseName,si,len);
}

/*
**--------------------------------------------------------------
**  x_TSO_Open
**  Input:   filenameOrDirectory: the filename of the TSO file,
**           or the directory containing the set of TSO file
**           pieces to read.  The pattern parameter MUST be NULL
**           if it is a single file.
**           fileVersion and storageMethod are only needed if
**           the input file does not have the header
**  Output:  A pointer to the structure used by the TSO_* routines
**  Purpose: Opens the TSO database file or files
**--------------------------------------------------------------
*/
static PTSO x_TSO_Open(char *filenameOrDirectory, char *pattern, char fileVersion, char storageMethod) {
	PTSO tso;
	char fn[MAXFNAME+1];
	char header[32];
	MEMCHECK(tso = (PTSO)calloc(1,sizeof(TSO)),"tso in x_TSO_Open");
	if(pattern == NULL) { // a single tso file
		printf("  TSO File Name: %s \n\n", filenameOrDirectory); /* Oct 6 */
		strcpy(fn,filenameOrDirectory);
		x_TSO_getBaseNameAndDirectory(tso,fn);
	} else {
		char tsodir[MAXFNAME+1];
#ifdef WIN32
		char patt[MAXFNAME+1];
		struct _finddata_t files; 
		__int64 hFile;
		char **filenames=NULL;
		int n;

		strcpy(tsodir,filenameOrDirectory);
 		sprintf(patt,"%s\\%s",tsodir,pattern);
		
		if((hFile=_findfirst(patt,&files)) == -1L) {
			tsoerror(1,"\n   No files found matching: '%s' in directory: '%s'\n",pattern,tsodir);
		}
		n=0;
		do {
			if(filenames==NULL)
				MEMCHECK(filenames = (char **)calloc(1,sizeof(char *)),"filenames in x_TSO_Open");
			else
				MEMCHECK(filenames = (char **) realloc(filenames,(n+1)*sizeof(char *)),"filenames in x_TSO_Open");
			MEMCHECK(filenames[n] = (char *)calloc(MAXFNAME+1,sizeof(char)),"filenames[n] in x_TSO_Open");
			sprintf(filenames[n],"%s\\%s",tsodir,files.name);
			n++;
		} while( _findnext(hFile,&files) == 0);
//		for(i=0;i<n;i++) {
//			printf("%s\n",filenames[i]);
//		}
		sprintf(fn,"%s\\output.hdr.tso",tsodir);
		tso->names=filenames;
		tso->numFiles=n;
		tso->curFile=-1;
		_findclose(hFile);
#else
#ifdef __linux__
		//		char patt[MAXFNAME+1];
		struct stat info;
		DIR *dir;
		struct dirent *entry;
		char **filenames=NULL;
		int i,n;

		strcpy(tsodir,filenameOrDirectory);
		// 		sprintf(patt,"%s/%s",tsodir,pattern);
		if(!(stat(tsodir,&info) == 0 && info.st_mode & S_IFDIR))
			tsoerror(2,"TSO directory is NOT a directory: %s\n",tsodir);

		if((dir=opendir(tsodir))==NULL) {
			tsoerror(1,"\n   No files found matching: '%s' in directory: '%s'\n",pattern,tsodir);
		}
		n=0;
		while(entry = readdir(dir)) {
			if(fnmatch(pattern,entry->d_name,0) ==0) {
				if(filenames==NULL)
					MEMCHECK(filenames = (char **)calloc(1,sizeof(char *)),"filenames in x_TSO_Open");
				else
					MEMCHECK(filenames = (char **) realloc(filenames,(n+1)*sizeof(char *)),"filenames in x_TSO_Open");
				MEMCHECK(filenames[n] = (char *)calloc(MAXFNAME+1,sizeof(char)),"filenames[n] in x_TSO_Open");
				sprintf(filenames[n],"%s/%s",tsodir,entry->d_name);
				n++;
			}
		}
//		for(i=0;i<n;i++) {
//			printf("%s\n",filenames[i]);
//		}
		sprintf(fn,"%s/output.hdr.tso",tsodir);
		tso->names=filenames;
		tso->numFiles=n;
		tso->curFile=-1;
#endif
#endif
		tso->tsodir = (char *)calloc(strlen(tsodir)+1,sizeof(char));
		strcpy(tso->tsodir,tsodir);
	}
	if((tso->stream=fopen(fn,"rb"))==NULL) {
		tsoerror(0,"\nUnable to open file %s\nError: %d %s\n",filenameOrDirectory,errno,strerror(errno));
	}
	fread(&header,32,sizeof(char),tso->stream);
	if((header[0]&0xFF) != 0xFF) {
		if((fileVersion&0xFF) == 0xFF || (storageMethod&0xFF)==0xFF) {
			tsoerror(1,"TSO file does not have the header, use TSO_OpenNoHeader instead\n");
		} else {
			tso->fileVersion = fileVersion;
			tso->storageMethod = storageMethod;
			tso->nBytesForNodeID = 16;
			fseek(tso->stream,0L,SEEK_SET);
		}
	} else {
		tso->fileVersion = header[1];
		tso->storageMethod = header[2];
		if(tso->fileVersion >= 5) {
			tso->nBytesForNodeID = header[3];
		} else {
			tso->nBytesForNodeID = 16;
		}
	}
	if(tso->fileVersion < MIN_TSOFILEVERSION) {
		tsoerror(1,"TSO File version %d not supported by this version of the API (min file version supported: %d)\n",tso->fileVersion,MIN_TSOFILEVERSION);
	}
	if(tso->fileVersion > TSOFILEVERSION) {
		tsoerror(1,"TSO File version %d not supported by this version of the API (max file version supported: %d)\n",tso->fileVersion,TSOFILEVERSION);
	}
	if(tso->storageMethod > TSOSTORAGEMETHOD) {
		tsoerror(1,"TSO storage method %d not supported by this version of the API (max storage method supported: %d)\n",tso->storageMethod,TSOSTORAGEMETHOD);
	}
	if(tso->fileVersion == 8) {
		tsoerror(-1,"TSO File version 8 has a potential bug in the index file if any files are > 2GB\nAND the direct-access methods are used (TSO_GetRawSimulationResults and TSO_GetSimulationResults)\n");
	}
	tso->curIndex=-1;
	return tso;
}

LIBEXPORT(int) TSO_isTSO(const char *tsoName)
{
	FILE *fp=fopen(tsoName,"r");
	size_t nread;
	char header[HEADER_SIZE];
	if(fp==NULL) {
		fprintf(stderr,"Unable to open ERD/TSO file: %s\n",tsoName);
		exit(9);
	}
	nread=fread(header, sizeof(char), HEADER_SIZE, fp);
	fclose(fp);
	if(nread < HEADER_SIZE)
		return 0;
	return(header[1] < FIRST_ERD_VERSION);
}

/*
**--------------------------------------------------------------
**  TSO_Open
**  Input:   filenameOrDirectory: the filename of the TSO file,
**           or the directory containing the set of TSO file
**           pieces to read.  The pattern parameter MUST be NULL
**           if it is a single file.
**  Output:  A pointer to the structure used by the TSO_* routines
**  Purpose: Opens the TSO database file or files
**--------------------------------------------------------------
*/
LIBEXPORT(PTSO) TSO_Open(char *filenameOrDirectory, char *pattern) {
	return x_TSO_Open(filenameOrDirectory,pattern,(char)0xFF,(char)0xFF);
}
/*
**--------------------------------------------------------------
**  TSO_OpenNoHeader
**  Input:   filenameOrDirectory: the filename of the TSO file,
**           or the directory containing the set of TSO file
**           pieces to read.  The pattern parameter MUST be NULL
**           if it is a single file.
**           fileVersion and storageMethod are only needed if
**           the input file does not have the header
**  Output:  A pointer to the structure used by the TSO_* routines
**  Purpose: Opens the TSO database file or files.  This method
**           MUST be used when the TSO database has no header
**           (i.e. it is version 1 or 2)
**--------------------------------------------------------------
*/
LIBEXPORT(PTSO) TSO_OpenNoHeader(char *filenameOrDirectory, char *pattern, char fileVersion, char storageMethod) {
	return x_TSO_Open(filenameOrDirectory,pattern,fileVersion, storageMethod);
	
}
/*
**--------------------------------------------------------------
**  x_TSO_ReadSizes
**  Input:   tso: the pointer returned by one of the TSO_Open methods
**           net: A pointer to the PNetInfo structure to populate
**                from the TSO file
**  Output:  
**  Purpose: 
**--------------------------------------------------------------
*/
static void x_TSO_ReadSizes(PTSO tso, PNetInfo *net)
{
	int status=0;
	int nnodes,nlinks,ntanks,njunctions,qualcode,nspecies,nbulkspecies,nsteps;
    long simduration,reportstart,reportstep,simstart;
	float stepsize,fltmax;

	if ( fread(&nnodes,sizeof(int),1,tso->stream) 		< 1 )  status=1; /* nnodes */
	if ( fread(&nlinks,sizeof(int),1,tso->stream) 		< 1 )  status=1; /* nlinks */
	if ( fread(&ntanks,sizeof(int),1,tso->stream) 		< 1 )  status=1; /* ntanks */
	if ( fread(&njunctions,sizeof(int),1,tso->stream) < 1 ) status=1; /* njunctions */
	if ( fread(&nspecies,sizeof(int),1,tso->stream) < 1 ) status=1; /* nspecies */
	if ( fread(&nsteps,sizeof(int),1,tso->stream) < 1 )     status=1; /* nsteps */
	if ( fread(&stepsize,sizeof(float),1,tso->stream) < 1 ) status=1; /* stepsize */
	if ( fread(&fltmax,sizeof(float),1,tso->stream) < 1 )   status=1; /* max float */
	if ( fread(&qualcode,sizeof(int),1,tso->stream) < 1 ) status=1; /* quality simulation type */
	if ( fread(&nbulkspecies,sizeof(int),1,tso->stream) < 1 ) status=1; /* nbulkspecies */
	if(tso->fileVersion >= 9) {
		int val;
		if ( fread(&val,sizeof(int),1,tso->stream) < 1 ) status=1; /* simulation duration (seconds) */
		simduration=val;
		if ( fread(&val,sizeof(int),1,tso->stream) < 1 ) status=1; /* start of reporting period (seconds) */
		reportstart=val;
		if ( fread(&val,sizeof(int),1,tso->stream) < 1 ) status=1; /* reporting time step (seconds) */
		reportstep=val;
		if(tso->fileVersion >= 11) {
			if ( fread(&val,sizeof(int),1,tso->stream) < 1 ) status=1; /* simulation startTime (seconds after midnight) */
			simstart=val;
		} else {
			simstart=0; // assume 0
		}
	} else {
		if ( fread(&simduration,sizeof(long),1,tso->stream) < 1 ) status=1; /* simulation duration (seconds) */
		if ( fread(&reportstart,sizeof(long),1,tso->stream) < 1 ) status=1; /* start of reporting period (seconds) */
		if ( fread(&reportstep,sizeof(long),1,tso->stream) < 1 ) status=1; /* reporting time step (seconds) */
	}

	*net = TSO_AllocateNetworkInfo(nspecies);
	(*net)->numNodes = nnodes;
	(*net)->numLinks = nlinks;
	(*net)->numTanks = ntanks;
	(*net)->numJunctions = njunctions;
	(*net)->numSteps = nsteps;
	(*net)->stepSize = stepsize;
	(*net)->fltMax = fltmax;
    (*net)->qualCode = qualcode;
	(*net)->numSpecies = nbulkspecies;
	(*net)->reportStart = reportstart;
	(*net)->reportStep = reportstep;
	(*net)->simDuration = simduration;
	(*net)->simStart=simstart;

	if(status) tsoerror(1,"Reading size header in ReadSize");
}

static void x_TSO_ReadIndex(PTSO tso, PNodeInfo node)
{
	FILE *fpIndex;
	char fn[MAXFNAME+1];

	sprintf(fn,"%s/%s.sdx",tso->tsodir,tso->baseName);

	if((fpIndex=fopen(fn,"rb"))==NULL) {
		tsoerror(0,"\nUnable to open file %s\nError: %d %s\n",fn,errno,strerror(errno));
	}
	while(!feof(fpIndex)) {
		int id;
		fread(&id,sizeof(int),1,fpIndex);
		if(!feof(fpIndex)) {
			PDataIndex di;
			PTEVAIndexData tid;
			MEMCHECK(di = (PDataIndex)calloc(1,sizeof(DataIndex)),"di in x_TSO_ReadIndex");
			if(id >= tso->count) {
				int i;
				tso->index = (PDataIndex *)realloc(tso->index,sizeof(PDataIndex)*(id+1));
				for(i=tso->count;i<id+1;i++) {
					tso->index[i]=NULL;
				}
				tso->count=id+1;
			}
			tso->index[id]=di;
			di->id=id;
			fread(&di->fileIndex,sizeof(int),1,fpIndex);
			if(tso->fileVersion < 9) {
				long offs;
				fread(&offs,sizeof(long),1,fpIndex);
				di->fileOffset=offs;
			} else {
				fread(&di->fileOffset,sizeof(__file_pos_t),1,fpIndex);
			}
			fread(&di->dataLength,sizeof(int),1,fpIndex);
			tid=di->sourceData=(PTEVAIndexData)calloc(1,sizeof(TEVAIndexData));
			x_TSO_ReadSources(fpIndex,&tid->nsources,&tid->source, node, tso->fileVersion);
		}
	}
	fclose(fpIndex);
}

/*
**--------------------------------------------------------------
**  TSO_ReadPrologue
**  Input:   Pointer to TSO data structure (returned from call
**           to one of the TSO_Open routines)
**  Output:  node: populated set of NodeInfo structures (including
**           demands)
**           link: populated set of LinkInfo structures
**
**           Note: the node and link arrays are allocated by 
**           this routine
**  Purpose: Reads the file prologue and populates the node and
**           link data
**--------------------------------------------------------------
*/
LIBEXPORT(void) TSO_ReadPrologue(PTSO tso, PNetInfo *netInfo, PNodeInfo *nodes,PLinkInfo *links, int flags)
{
    int i,j,status=0;
	char msg[256];
    FILE *TSO;
	int ntanks,
		nnodes,
		nlinks,
		nsteps;
	PNodeInfo node;
	PLinkInfo link;
	PNetInfo net;
	PHydData hyd;
	char **filenames=NULL;
	int n=0;
	int len=-1;
	char tfn[256];

	TSO=tso->stream;
	x_TSO_ReadSizes(tso,netInfo);
	net=*netInfo;
	MEMCHECK(tso->nodedata = (float*)calloc(net->numNodes,sizeof(float)),"tso->nodedata in TSO_ReadPrologue");
	MEMCHECK(tso->linkdata = (float*)calloc(net->numLinks,sizeof(float)),"tso->linkdata in TSO_ReadPrologue");
	TSO_AllocateNetworkData(nodes,links,net,flags);
	node=*nodes;
	link=*links;
	ntanks = net->numTanks,
	nnodes = net->numNodes,
	nlinks = net->numLinks,
	nsteps = net->numSteps;

    for (i=0;i<ntanks;i++) {
        if(fread(&j,sizeof(int),1,TSO) < 1) status=1;
        else node[j-1].type=tank;
    }
    for (i=0;i<nnodes;i++) {
        if(fread(node[i].id,sizeof(char),tso->nBytesForNodeID,TSO) < (unsigned int)tso->nBytesForNodeID) status=1;
    }
    for (i=0;i<nlinks;i++) {
        if(fread(link[i].id,sizeof(char),tso->nBytesForNodeID,TSO) < (unsigned int)tso->nBytesForNodeID) status=1;
    }
	for (i=0;i<net->numSpecies;i++) {
		if(fread(net->species[i]->id,sizeof(char),MAXSPECIEID,TSO) < MAXSPECIEID) status=1;
	}
    for (i=0;i<nnodes;i++) {
        if(fread(&node[i].x,sizeof(float),1,TSO) < 1) status=1;
    }
    for (i=0;i<nnodes;i++) {
        if(fread(&node[i].y,sizeof(float),1,TSO) < 1) status=1;
    }
    for (i=0;i<nlinks;i++) {
        if(fread(&link[i].from,sizeof(int),1,TSO) < 1) status=1;
    }
    for (i=0;i<nlinks;i++) {
        if(fread(&link[i].to,sizeof(int),1,TSO) < 1) status=1;
    }
    for (i=0;i<nlinks;i++) {
    	if(fread(&link[i].length,sizeof(float),1,TSO) < 1) status=1;
    }
    for (i=0;i<nlinks;i++) {
		if(fread(&link[i].nv,sizeof(int),1,TSO) < 1) status=1;
		if(link[i].nv) {
			MEMCHECK(link[i].vx = (float *)calloc(link[i].nv,sizeof(float)),"link[i].vx in ReadPrologue");
			MEMCHECK(link[i].vy = (float *)calloc(link[i].nv,sizeof(float)),"link[i].vy in ReadPrologue");
			if(fread(link[i].vx,sizeof(float),link[i].nv,TSO) < (size_t)link[i].nv) status=1;
			if(fread(link[i].vy,sizeof(float),link[i].nv,TSO) < (size_t)link[i].nv) status=1;
		}
    }
    if(flags & READ_DEMANDS) {
		float** q=net->hydResults->demand;
	    for(i=0;i<nsteps && status ==0;i++) {
			size_t nread=fread(tso->nodedata,sizeof(float),nnodes,TSO);
			if(nread < (size_t)nnodes) {
				status=1;
				sprintf(msg,"Reading nodedata: attempted to read %d elements, only read %d",nnodes,nread);
			}
	        for(j=0;j<nnodes;j++) {
				q[i][j] = tso->nodedata[j];
	        }
		}
	} else {
		fseek(TSO,nsteps * nnodes * sizeof(float),SEEK_CUR);
	}
	if(tso->fileVersion >=10) {
        for(j=0;j<nnodes;j++) {
        	int len;
            if(fread(&len,sizeof(int),1,TSO) < 1) status=1;
            node[j].demandProfileLength = len;
            if(len>0) {
	            node[j].demandProfile=(float*)calloc(len,sizeof(float));
	            if(fread(node[j].demandProfile,sizeof(float),len,TSO) < (size_t)len) status=1;
            }
        }
	}
	hyd=net->hydResults;
	if(flags & READ_LINKFLOW) {
		float** flow=hyd->flow;
		for(i=0;i<nsteps;i++) {
			if(fread(tso->linkdata,sizeof(float),nlinks,TSO) < (unsigned int)nlinks) status=1;
			for(j=0;j<nlinks;j++) {
				flow[i][j] = tso->linkdata[j];
			}
		}
	} else {
		fseek(TSO,nsteps * nlinks * sizeof(float),SEEK_CUR);
	}
	if(flags & READ_LINKVEL) {
		float** vel=hyd->velocity;
		for(i=0;i<nsteps;i++) {
			if(fread(tso->linkdata,sizeof(float),nlinks,TSO) < (unsigned int)nlinks) status=1;
			for(j=0;j<nlinks;j++) {
				vel[i][j] = tso->linkdata[j];
			}
		}
	} else {
		fseek(TSO,nsteps * nlinks * sizeof(float),SEEK_CUR);
	}

    if(status) tsoerror(1,"Reading TSO prologue in ReadPrologue\n  %s",msg);
    if(fgetpos(TSO,&tso->pos_cdata)) tsoerror(1,"Reading TSO prologue in ReadPrologue");
//	printf("tso->pos_cdata: %d\n",tso->pos_cdata);
	*nodes=node;
	*links=link;

	// now read all of the filenames that make up this TSO (if any)

	while(len != 0 && !feof(TSO)) {
		fread(&len,sizeof(int),1,TSO);
		if(!feof(TSO) && len > 0) {
			fread(tfn,sizeof(char),len,TSO);
			tfn[len] = '\0';
			if(filenames==NULL)
				MEMCHECK(filenames = (char **)calloc(1,sizeof(char *)),"filenames in TSO_ReadPrologue");
			else
				MEMCHECK(filenames = (char **) realloc(filenames,(n+1)*sizeof(char *)),"filenames in TSO_ReadPrologue");
			MEMCHECK(filenames[n] = (char *)calloc(strlen(tso->tsodir)+len+2,sizeof(char)),"filenames[n] in TSO_Prologue");
			sprintf(filenames[n],"%s/%s",tso->tsodir,tfn);
			n++;
		}
	}
	tso->names=filenames;
	if(n==0) {
		n=1;
		MEMCHECK(tso->streams = (FILE **)calloc(n,sizeof(FILE *)),"tso->streams in TSO_ReadPrologue");
		tso->streams[0]=TSO;
		tso->numFiles=n;
		tso->curFile=-1;
	} else {
		MEMCHECK(tso->streams = (FILE **)calloc(n,sizeof(FILE *)),"tso->streams in TSO_ReadPrologue");
		tso->numFiles=n;
		tso->curFile=-1;
	}
	x_TSO_ReadIndex(tso,node);
}

/*
**--------------------------------------------------------------
**  TSO_PositionAfterPrologue
**  Input:   Pointer to TSO data structure (returned from call
**           to one of the TSO_Open routines)
**  Output:  
**  Purpose: Resets the TSO file pointer to the beginning of the
**           simulation data so the file can be re-processed
**--------------------------------------------------------------
*/
LIBEXPORT(void) TSO_PositionAfterPrologue(PTSO tso)
{
	if(tso->numFiles==1 && tso->streams[0]==tso->stream) {
		fsetpos(tso->stream,&tso->pos_cdata);
	} else {
		if(tso->stream != NULL)
			fclose(tso->stream);
		tso->curFile=0;
		tso->stream = fopen(tso->names[tso->curFile],"rb");
	}

}
/*
**--------------------------------------------------------------
**  x_TSO_eof
**  Input:   Pointer to TSO data structure (returned from call
**           to one of the TSO_Open routines)
**  Output:  
**  Purpose: Checks to see if the end of the TSO file or files
**           has been reached
**--------------------------------------------------------------
*/
static int x_TSO_eof(PTSO tso)
{
	fgetc(tso->stream);
	if(feof(tso->stream)){
		int singleFile=tso->numFiles==1 && tso->streams[0]==tso->stream;
		if(!singleFile) {
			fclose(tso->stream);
			tso->stream=NULL;
			tso->curFile++;
			if(tso->curFile>=tso->numFiles) return 1;
			else {
//				printf("Opening next TSO File: %s\n", tso->names[tso->curFile]);
				tso->stream=fopen(tso->names[tso->curFile],"rb");
			}
		} else {
			return 1;
		}
	} else {
		fseek(tso->stream,-1,SEEK_CUR);
	}
	return 0;
}
int x_TSO_ReadSources(FILE *fpTSO, int *nsources,PSource *sources, PNodeInfo nodes, int fileVersion)
{
	int status=0;
	int i;
    if(fread(nsources,sizeof(int),1,fpTSO) < 1) status=1;
    if(*nsources>MAXSOURCES) tsoerror(1,"Too many sources: %d.",*nsources);

	if(*sources == NULL) {
		MEMCHECK(*sources = (PSource)calloc(*nsources,sizeof(Source)),"*sources in x_TSO_ReadSource");
	}
    for(i=0;i<*nsources;i++) {
		PSource ps = &(*sources)[i];
        if(fread(&ps->sourceNodeIndex,sizeof(int),1,fpTSO) < 1) status=1;
		strcpy(ps->sourceNodeID,nodes[ps->sourceNodeIndex-1].id);
		if(fread(&ps->speciesIndex,sizeof(int),1,fpTSO) < 1) status=1;
		// this is set to 0 because it is now 0-based (in ENTU_WriteTSI)
		// and it being 1 was causing problems in comparing tso & erd files
		// this shouldn't be an issue since nothing really uses this value anyway
		// and this code will never read any files that have more than one species
		if(ps->speciesIndex==1) ps->speciesIndex=0;
		if(fread(&ps->sourceType,sizeof(int),1,fpTSO) < 1) status=1;
		if(fread(&ps->sourceStrength,sizeof(float),1,fpTSO) < 1) status=1;
		if(fileVersion >=9) {
			int val;
			if(fread(&val,sizeof(int),1,fpTSO) < 1) status=1;
			ps->sourceStart=val;
			if(fread(&val,sizeof(int),1,fpTSO) < 1) status=1;
			ps->sourceStop=val;
		} else {
			if(fread(&ps->sourceStart,sizeof(long),1,fpTSO) < 1) status=1;
			if(fread(&ps->sourceStop,sizeof(long),1,fpTSO) < 1) status=1;
		}
    }
	return status;
}
/*
**--------------------------------------------------------------
**  TSO_ReadSimulationResults
**  Input:   Pointer to TSO data structure (returned from call
**           to one of the TSO_Open routines)
**           net: pointer to network information
**  Output:  nodes: NodeInfo structures with the conecntration
**                  values poulated
**           sources: populated set of Source structures
**           net->nsources: Set to the number of sources for
**                          the current simulation results
**  Purpose: Reads output from one simulation (source information
**           and concentration data)
**--------------------------------------------------------------
*/
LIBEXPORT(int) TSO_ReadSimulationResults(PTSO tso, PNetInfo net, PNodeInfo nodes)
{

	if(x_TSO_eof(tso)) return(0);
	tso->curIndex++;
	return x_TSO_ReadSimulationResults(tso->stream,tso,net,nodes);
}
int x_TSO_positionFile(FILE *fp,__file_pos_t offs)
{
    fpos_t pos;
	memset(&pos,0,sizeof(fpos_t));
#ifdef __linux__
	pos.__pos = (int64_t)offs;
#else
#ifdef WIN32
	pos = (fpos_t)offs;
#endif
#endif
	return fsetpos(fp,&pos);
}

/*
**--------------------------------------------------------------
**  TSO_GetSimulationResults
**  Input:   ID of the source/data element to obtain.
**           Pointer to TSO data structure (returned from call
**           to one of the TSO_Open routines)
**           net: pointer to network information
**  Output:  nodes: NodeInfo structures with the conecntration
**                  values poulated
**           sources: populated set of Source structures
**           net->nsources: Set to the number of sources for
**                          the current simulation results
**  Purpose: Reads output from one simulation (source information
**           and concentration data)
**--------------------------------------------------------------
*/
LIBEXPORT(int) TSO_GetSimulationResults(int id, PTSO tso, PNetInfo net, PNodeInfo nodes)
{
	PDataIndex di;

	if(id >= tso->count || id < 0) {
		tsoerror(-1,"TSO_GetSimulationResults: id out of range: %d valid range: [0,%d]",id,tso->count-1);
		return 0;
	}
	tso->curIndex=id;
	di = tso->index[id];

	if(tso->streams[di->fileIndex]==NULL) {
		tso->streams[di->fileIndex]=fopen(tso->names[di->fileIndex],"rb");
		if(tso->streams[di->fileIndex]==NULL) {
			tsoerror(-1,"TSO_GetSimulationResults: Unable to open file: %s",tso->names[di->fileIndex]);
			return 0;
		}
	}
	if(x_TSO_positionFile(tso->streams[di->fileIndex],di->fileOffset)) {
	  return DB_UNABLE_TO_POSITION;
	}

	return x_TSO_ReadSimulationResults(tso->streams[di->fileIndex],tso,net,nodes);
}

/*
**--------------------------------------------------------------
**  TSO_GetRawSimulationResults
**  Input:   ID of the source/data element to obtain.
**           Pointer to TSO data structure (returned from call
**           to one of the TSO_Open routines)
**           net: pointer to network information
**           nodes: used by routine that loads the sources
**           len: the length of the allocated buffer
**  Output:  buf: The raw file data for the ID
**           sources: populated set of Source structures
**           net->nsources: Set to the number of sources for
**                          the current simulation results
**  Purpose: Reads output from one simulation (source information
**           and concentration data).  The concentration data is 
**           only loaded from the file - any decompression needs 
**           to be done by the calling routine.  This method was added
**           for efficiency in transmitting loaded concentration
**           data efficiently accross the network.
**--------------------------------------------------------------
*/
LIBEXPORT(int) TSO_GetRawSimulationResults(int id, PTSO tso, PNetInfo net, PNodeInfo nodes, char *buf, int len)
{
	PDataIndex di;

	if(id >= tso->count || id < 0) {
		tsoerror(-1,"TSO_GetRawSimulationResults: id out of range: %d valid range: [0,%d]",id,tso->count-1);
		return 0;
	}
	di = tso->index[id];
	if(tso->streams[di->fileIndex]==NULL) {
		tso->streams[di->fileIndex]=fopen(tso->names[di->fileIndex],"rb");
		if(tso->streams[di->fileIndex]==NULL) {
			tsoerror(-1,"TSO_GetRawSimulationResults: Unable to open file: %s",tso->names[di->fileIndex]);
			return 0;
		}
	}
	// no longer have to potentially load or copy source info (loaded from the index)
	// but still need to skip past the stored source ID (+sizeof(int) below)
	if(x_TSO_positionFile(tso->streams[di->fileIndex],di->fileOffset+sizeof(int))) {
	  return DB_UNABLE_TO_POSITION;
	}
	if(len < di->dataLength-4) {
		tsoerror(-1,"TSO_GetRawSimulationResults: data buffer too small: required %d bytes but only had %d bytes.",di->dataLength-4,len);
		return DB_DATA_BUFFER_TOO_SMALL;
	}
	fread(buf,1,di->dataLength-4,tso->streams[di->fileIndex]);
	return 1;
}

static int x_TSO_ReadSimulationResults(FILE *fpTSO, PTSO tso, PNetInfo net, PNodeInfo nodes)
{
    int idx,
        nnz,
        i,
		s,
        status=0,
		nnodes=net->numNodes,
		nsteps=net->numSteps;
	int nbulkspecies=net->numSpecies;
	size_t nread;

	// no longer have to potentially load or copy source info (loaded from the index)
	// but still need to skip past the stored source ID (+sizeof(int) below)
	if((nread=fread(&i,sizeof(int),1,fpTSO)) != 1) {
		status=1;
		tsoerror(1,"Positioning file before read");
	}

	for(s=0;s<nbulkspecies;s++) {
		float **nodeC=net->qualResults->nodeC[s];
		if(tso->storageMethod==0) {
			/* Read concentrations */
			for(i=0;i<nsteps;i++) {
				int j;
				if((nread=fread(tso->nodedata,sizeof(float),nnodes,fpTSO)) < nnodes) {
					status=1;
					tsoerror(1,"Reading TSO concentrations in ReadNextC: %d %d %d %d",nread,nnodes,s,i);
				}
				for(j=0;j<nnodes;j++) {
					nodeC[j][i] = tso->nodedata[j];
				}
			}
		} else {
			unsigned short groupTypeMask;
			unsigned short countMask;
			unsigned short countBits;
			if(tso->storageMethod==2) {
				groupTypeMask = 0x8000;
				countMask =     0x7FFF;
				countBits = 15;
			} else {
				groupTypeMask = 0xC000;
				countMask =     0x3FFF;
				countBits = 14;
			}

			for(i=0;i<nnodes;i++) {
				nodes[i].nz[s]=0;
				memset(nodeC[i],0,sizeof(float)*nsteps);
			}  /* default is all zeros */
			if((nread=fread(&nnz,sizeof(int),1,fpTSO)) < 1) {
				status=1;
				tsoerror(1,"Reading num non-zero in ReadNextC: %d %d %d %d",nread,1,s,i);
			}
			for(i=0;i<nnz;i++) {
				if((nread=fread(&idx,sizeof(int),1,fpTSO)) < 1) {
					status=1;
					tsoerror(1,"Reading idx in ReadNextC: %d %d %d %d",nread,1,s,i);
				}
				idx--;
				nodes[idx].nz[s]=1;
				if(tso->storageMethod==1) {
					// if compressed using type 1, simply read in the entire data array
					if((nread=fread(nodeC[idx],sizeof(float),nsteps,fpTSO)) < nsteps) {
						status=1;
						tsoerror(1,"Reading c in ReadNextC: %d %d %d %d",nread,nsteps,s,i);
					}

				} else {
					float *c = nodeC[idx];
					// must be type 2 or 3 - read in the compressed data
					unsigned short numGroups;
					int n=0,
						i,
						max=0;
					if(tso->storageMethod==2) {
						unsigned char ng;
						if((nread=fread(&ng,sizeof(char),1,fpTSO)) < 1) {
							status=1;
							tsoerror(1,"Reading ng in ReadNextC: %d %d %d %d",nread,1,s,i);
						}
						numGroups=ng;
					} else {
						if((nread=fread(&numGroups,sizeof(short),1,fpTSO)) < 1) {
							status=1;
							tsoerror(1,"Reading num groups in ReadNextC: %d %d %d %d",nread,1,s,i);
						}
					}
					for(i=0;i<numGroups;i++) {
						GroupHeader gh;
						short sval;
						if((nread=fread(&sval,sizeof(short),1,fpTSO)) < 1)  {
							status=1;
							tsoerror(1,"Reading sval in ReadNextC: %d %d %d %d",nread,1,s,i);
						}
						gh.groupType = (sval & groupTypeMask)>>countBits;
						gh.count = sval & countMask;
						max += gh.count;
						if (gh.groupType==1) {
							if((nread=fread(&c[n],sizeof(float),gh.count,fpTSO)) < gh.count) {
								status=1;
								tsoerror(1,"Reading &c[n] in ReadNextC: %d %d %d %d",nread,gh.count,s,i);
							}
							n = max;
						} else { 
							float v;
							if(gh.groupType==0) v = 0;
							else if (gh.groupType==2) {
								if((nread=fread(&v,sizeof(float),1,fpTSO)) < 1) {
									status=1;
									tsoerror(1,"Reading v in ReadNextC: %d %d %d %d",nread,1,s,i);
								}
							} else {
								tsoerror(1,"Invalid groupType in ReadNextC");
							}
							while(n < max) { c[n++] = v; }
						}
					}
				}
			}
		}
	}

    if(status)
		tsoerror(1,"Reading TSO concentrations in ReadNextC");
    return(1);
}
static PTSO x_TSO_Create(char *filename, int storageMethod, int fileVersion, PNetInfo net)
{
	PTSO tso;
	char fn[MAXFNAME+1];

	if(storageMethod<-1 || storageMethod>TSOSTORAGEMETHOD)
		tsoerror(1,"Invalid TSO storage method: %d\n",storageMethod);
	if(fileVersion < MIN_TSOFILEVERSION) {
		tsoerror(1,"TSO version %d will not be written - no versions below %d are supported\n",fileVersion,MIN_TSOFILEVERSION);
	}

	if(fileVersion==8) {
		tsoerror(1,"TSO version 8 will not be written due to a potential\nbug in the index file if any files are > 2GB.\nUse version 9 or later instead.\n");
	}
	MEMCHECK(tso = (PTSO)calloc(1,sizeof(TSO)),"tso in x_TSO_Create");

	tso->fileVersion=fileVersion;
	tso->storageMethod=storageMethod==-1?TSOSTORAGEMETHOD:storageMethod;
	tso->nBytesForNodeID = tso->fileVersion >= 5 ? 64 : 16;
	tso->count=0;
	tso->header[0] = (char)0xff;
	tso->header[1] = tso->fileVersion;
	tso->header[2] = tso->storageMethod;
	tso->header[3] = tso->nBytesForNodeID;
	MEMCHECK(tso->nodedata = (float *)calloc(net->numNodes,sizeof(float)),"tso->nodedata in x_TSO_Create");
	MEMCHECK(tso->linkdata = (float *)calloc(net->numLinks,sizeof(float)),"tso->linkdata in x_TSO_Create");

	if((tso->stream = fopen(filename,"wb"))==NULL) {
		tsoerror(1,"Unable to open file %s\nError: %d %s\n",filename,errno,strerror(errno));
	}
	if( fwrite(tso->header,sizeof(tso->header),1,tso->stream) < 1) return NULL;
	
	x_TSO_getBaseNameAndDirectory(tso,filename);
	if (!tso->tsodir  || (strlen(tso->tsodir)==0))
		sprintf(fn,"%s.sdx",tso->baseName);
	else
		sprintf(fn,"%s/%s.sdx",tso->tsodir,tso->baseName);
	printf("creating index file: %s\n",fn);
	tso->indexStream = fopen(fn,"wb");
	if(tso->indexStream == NULL) {
		tsoerror(1,"Unable to open index file %s for writing\nError: %d %s\n",fn,errno,strerror(errno));
	}

	return tso;
}
/*
**--------------------------------------------------------------
**  TSO_Create
**  Input:   Filename of the TSO file to create
**           The storage method to use when writing data to the
**           file
**  Output:  Pointer to TSO data structure representing
**           a TSO file to write to.
**  Purpose: Creates a new TSO file utilizing the specified
**           storage method
**--------------------------------------------------------------
*/
LIBEXPORT(PTSO) TSO_Create(char *filename, int storageMethod, PNetInfo net)
{
	return x_TSO_Create(filename,storageMethod,TSOFILEVERSION,net);
}

/*
**--------------------------------------------------------------
**  TSO_CreateSpecificVersion
**  Input:   Filename of the TSO file to create
**           The storage method to use when writing data to the
**           file
**  Output:  Pointer to TSO data structure representing
**           a TSO file to write to.
**  Purpose: Creates a new TSO file utilizing the specified
**           storage method
**--------------------------------------------------------------
*/
LIBEXPORT(PTSO) TSO_CreateSpecificVersion(char *filename, int storageMethod, int fileVersion, PNetInfo net)
{
	return x_TSO_Create(filename,storageMethod,fileVersion,net);
}

/*
**--------------------------------------------------------------
**  TSO_WritePrologue
**  Input:   Pointer to TSO data structure (returned from call
**           to TSO_Create)
**           network size information
**           node collection
**           link collection
**  Output:  
**  Purpose: Write the file prologue from the node and link data
**--------------------------------------------------------------
*/
LIBEXPORT(int) TSO_WritePrologue(PTSO tso, PNetInfo net, PNodeInfo nodes, PLinkInfo links)
{
	int i,
		status;
	int t;
	int nsteps;
	float **flow;
	int zero;

	fwrite(&net->numNodes,sizeof(int),1,tso->stream);
	fwrite(&net->numLinks,sizeof(int),1,tso->stream);
	fwrite(&net->numTanks,sizeof(int),1,tso->stream);
	fwrite(&net->numJunctions,sizeof(int),1,tso->stream);
	fwrite(&net->numSpecies,sizeof(int),1,tso->stream);

	fwrite(&net->numSteps,sizeof(int),1,tso->stream);
	fwrite(&net->stepSize,sizeof(int),1,tso->stream);
	fwrite(&net->fltMax,sizeof(int),1,tso->stream);
	fwrite(&net->qualCode,sizeof(int),1,tso->stream);
	fwrite(&net->numSpecies,sizeof(int),1,tso->stream);
	if(tso->fileVersion >= 9) {
		int val;
		val = (int)net->simDuration;
		fwrite(&val,sizeof(int),1,tso->stream);
		val=net->reportStart;
		fwrite(&val,sizeof(int),1,tso->stream);
		val=net->reportStep;
		fwrite(&val,sizeof(int),1,tso->stream);
		if(tso->fileVersion >= 11) {
			val=net->simStart;
			fwrite(&val,sizeof(int),1,tso->stream);
		}
	} else {
		fwrite(&net->simDuration,sizeof(long),1,tso->stream);
		fwrite(&net->reportStart,sizeof(long),1,tso->stream);
		fwrite(&net->reportStep,sizeof(long),1,tso->stream);
	}

	/* Tank indices */
	for (i=1; i<=net->numNodes; i++) {
		if(nodes[i-1].type==tank)
			if ( fwrite(&i,sizeof(int),1,tso->stream) < 1 ) {status = 400; return(status);}
	}
	/* Node IDs */
	if(tso->spaceFillStrings) {
		char *tid=(char*)calloc(tso->nBytesForNodeID+1,sizeof(char));
		int j;
		for (i=0; i<net->numNodes; i++) {
			strcpy(tid,nodes[i].id);
			for(j=(int)strlen(tid);j<tso->nBytesForNodeID-1;j++) tid[j]=' ';
			tid[j]='\0';
			if ( fwrite(tid,sizeof(char),tso->nBytesForNodeID,tso->stream) < (unsigned int)tso->nBytesForNodeID ) {status = 400; return(status);}
		}
		free(tid);
	} else {
		for (i=0; i<net->numNodes; i++) {
			if ( fwrite(nodes[i].id,sizeof(char),tso->nBytesForNodeID,tso->stream) < (unsigned int)tso->nBytesForNodeID ) {status = 400; return(status);}
		}
	}
	/* Link IDs */
	if(tso->spaceFillStrings) {
		char *tid=(char*)calloc(tso->nBytesForNodeID+1,sizeof(char));
		int j;
		for (i=0; i<net->numLinks; i++) {
			strcpy(tid,links[i].id);
			for(j=(int)strlen(tid);j<tso->nBytesForNodeID-1;j++) tid[j]=' ';
			tid[j]='\0';
			if ( fwrite(tid,sizeof(char),tso->nBytesForNodeID,tso->stream) < (unsigned int)tso->nBytesForNodeID ) {status = 400; return(status);}
		}
		free(tid);
	} else {
		for (i=0; i<net->numLinks; i++) {
			if ( fwrite(links[i].id,sizeof(char),tso->nBytesForNodeID,tso->stream) < (unsigned int)tso->nBytesForNodeID ) {status = 400; return(status);}
		}
	}
	if(tso->spaceFillStrings) {
		char *tid=(char*)calloc(tso->nBytesForNodeID+1,sizeof(char));
		int j;
		for (i=0; i<net->numSpecies; i++) {
			strcpy(tid,net->species[i]->id);
			for(j=(int)strlen(tid);j<MAXSPECIEID;j++) tid[j]=' ';
			if ( fwrite(tid,sizeof(char),MAXSPECIEID,tso->stream) < MAXSPECIEID ) {status = 400; return(status);}
		}
		free(tid);
	} else {
		for(i=0;i<net->numSpecies;i++) {
			if ( fwrite(net->species[i]->id,sizeof(char),MAXSPECIEID,tso->stream) < MAXSPECIEID ) {status = 400; return(status);}
		}
	}
	/* Node X locations */
	for (i=0; i<net->numNodes; i++) {
		if ( fwrite(&nodes[i].x,sizeof(float),1,tso->stream) < 1 ) {status = 400; return(status);}
	}
	/* Node Y locations */
	for (i=0; i<net->numNodes; i++) {
		if ( fwrite(&nodes[i].y,sizeof(float),1,tso->stream) < 1 ) {status = 400; return(status);}
	}
	for (i=0; i<net->numLinks; i++) {                                                         /* From link nodes */
		if ( fwrite(&links[i].from,sizeof(int),1,tso->stream) < 1 ) {status = 400; return(status);}
	}
	for (i=0; i<net->numLinks; i++) {                                                         /* To link nodes */
		if ( fwrite(&links[i].to,sizeof(int),1,tso->stream) < 1 ) {status = 400; return(status);}
	}
	for (i=0; i<net->numLinks; i++) {                                                     /* Link Lengths */
		if ( fwrite(&links[i].length,sizeof(float),1,tso->stream) < 1 ) {status = 400; return(status);}
	}
	for (i=0;i<net->numLinks;i++) {
		if(fwrite(&links[i].nv,sizeof(int),1,tso->stream) < 1) {status = 400; return(status);}
		if(links[i].nv) {
			if(fwrite(links[i].vx,sizeof(float),links[i].nv,tso->stream) < (size_t)links[i].nv) {status = 400; return(status);}
			if(fwrite(links[i].vy,sizeof(float),links[i].nv,tso->stream) < (size_t)links[i].nv) {status = 400; return(status);}
		}
	}
	x_TSO_WriteDemands(tso,net,nodes);
	x_TSO_WriteDemandProfiles(tso,net,nodes);
	nsteps = net->numSteps;
	flow=net->hydResults->flow;
	for(t=0;t<nsteps;t++) {
		if ( fwrite(flow[t],sizeof(float),net->numLinks,tso->stream) < 1 ) {status = 400; return(status);}
	}
	for(t=0;t<nsteps;t++) {
		if ( fwrite(net->hydResults->velocity[t],sizeof(float),net->numLinks,tso->stream) < 1 ) {status = 400; return(status);}
	}
	zero=0;
	// write 0 for the filename length - this will identify it as a single file TSO
	fwrite(&zero,sizeof(int),1,tso->stream);
	return 0;
}

static int x_TSO_WriteDemandProfiles(PTSO tso, PNetInfo net,PNodeInfo nodes)
{
	int n,
		nnodes=net->numNodes;
	if(tso->fileVersion<10) return 1;
	for(n=0;n<nnodes;n++) {
		int len=nodes[n].demandProfileLength;
		if( fwrite(&len,sizeof(int),1,tso->stream) < 1) return 0;
		if(len>0) {
			if( fwrite(nodes[n].demandProfile,sizeof(float),len,tso->stream) < (size_t)len) return 0;
		}
	}
	return 1;
}

/*
**--------------------------------------------------------------
**  x_TSO_WriteDemands
**  Input:   Pointer to TSO data structure (returned from call
**           to TSO_Create)
**           network size information
**           node collection with the demand data populated
**  Output:  
**  Purpose: Write the file prologue from the node and link data
**--------------------------------------------------------------
*/
static int x_TSO_WriteDemands(PTSO tso, PNetInfo net,PNodeInfo nodes)
{
	int t,
		nsteps=net->numSteps,
		nnodes=net->numNodes;
	for(t=0;t<nsteps;t++) {
		if( fwrite(net->hydResults->demand[t],sizeof(float),nnodes,tso->stream) < 1) return 0;
	}
	return 1;
}

/*
**--------------------------------------------------------------
**  x_compressAndStore3
**  Input:   Pointer to file stream to write the compressed data
**           the array of floats to "compress" using storage method 3
**           the number of elements in the data
**  Output:  
**  Purpose: "compress" the conecntration data and write the data
**           to the TSO file.
**--------------------------------------------------------------
*/
static int x_compressAndStore3(float *dtc,int nsteps,FILE *stream)
{
	char *cd;
	float *values;
	short *counts;
	// numGroups is a short (rather than a char) to allow for the rare (at least currently)
	// condition of having more than 255 groups in one record (I have recently seen one
	// with 177 groups.  I chose to make this a short rather than doing anything more complicated
	// algorithmically because adding one byte to every record is negligable, and any algorithm
	// implemented to deal with more than 255 groups would be time intensive and would use extra space,
	short numGroups=0;
	int numValues;
	int p=2;
	int i,j,rv;
	
	MEMCHECK(cd = (char *)calloc((nsteps+10)*sizeof(float),sizeof(char)),"cd in x_compressAndStore3");

	MEMCHECK(values = (float *)calloc(nsteps,sizeof(float)),"values in x_compressAndStore3");
	MEMCHECK(counts = (short *)calloc(nsteps,sizeof(short)),"counts in x_compressAndStore3");

	values[0]=dtc[0];
	counts[0]=1;
	i=1;
	j=0;
	while(i<nsteps) {
		if(dtc[i] != values[j]) {
			j++;
			values[j] = dtc[i];
		}
		counts[j]++;
		i++;
	}
	numValues=j+1;
	i=0;
	while(i<numValues) {
		if (counts[i]>1 || values[i]==0) {
			float v = values[i];
			short mask = v==0 ? (short)0x0000 : (short)0x8000;
			short sv = counts[i] | mask;
			memcpy(&cd[p],&sv,sizeof(short));
			p+=2;
			if(v!=0) {
				memcpy(&cd[p],&v,sizeof(float));
				p+=4;
			}
			i++;
		} else {
			// save the start pos (to write out the total later)
			int cp = p;
			short count=0;
			p += 2; // start after the size word
			while(counts[i]==1 && i<numValues) {
				memcpy(&cd[p],&values[i],sizeof(float));
				p+=4;
				count++;
				i++;
			}
			count |= 0x4000;
			memcpy(&cd[cp],&count,sizeof(short));
		}
		numGroups++;
	}
	memcpy(&cd[0],&numGroups,sizeof(short));
	rv = fwrite(cd,sizeof(char),p,stream) == (unsigned int)p;
	free(cd);
	free(values);
	free(counts);
	return rv ? p : -1;
}
/*
**--------------------------------------------------------------
**  x_compressAndStore2
**  Input:   Pointer to file stream to write the compressed data
**           the array of floats to "compress" using storage method 2
**           the number of elements in the data
**  Output:  
**  Purpose: "compress" the conecntration data and write the data
**           to the TSO file.
**--------------------------------------------------------------
*/
static int x_compressAndStore2(float *dtc,int nsteps,FILE *stream)
{
	char *cd;
	unsigned char numGroups=0;
	int p=1;
	int i=0,
		rv;
	MEMCHECK(cd = (char *)calloc((nsteps+10)*sizeof(float),sizeof(char)),"cd in x_compressAndStore2");
	while(i<nsteps) {
		// if we have more than 254 groups, then output all the rest as one group with group type = 1.
		// this may not be the most efficient, but it is the quickest to implement.  An example of a bad case:
		// group 255 is type 1 with one value and group #256 is a type 0 (all zeros) with 100 values
		// this logic will cause 101 floats to be written out with most f them being 0.
		// this is probably not too likely to happen currently.  The following solution is likely
		// the best solution that will be implemented for type2 storage because storage method 3
		// should be used because it is more efficient
		if(numGroups==254) {
			int cp = p;
			short count=0;
			p += 2; // start after the size word
			while(i<nsteps) {
				memcpy(&cd[p],&dtc[i],sizeof(float));
				p+=4;
				count++;
				i++;
			}
			count |= 0x8000;
			memcpy(&cd[cp],&count,sizeof(short));
		} else if(dtc[i]==0) { // start of a series of zeros
			short numZeros=0;
			while(i<nsteps && dtc[i] == 0) {numZeros++; i++;}
			// write the short to the buffer at p
			memcpy(&cd[p],&numZeros,sizeof(short));
			p+=2;
		} else {  // start of a series of non-zero data
			// save the start pos (to write out the total later)
			int cp = p;
			short count=0;
			p += 2; // start after the size word
			while(i<nsteps && dtc[i] != 0) {
				memcpy(&cd[p],&dtc[i],sizeof(float));
				p+=4;
				count++;
				i++;
			}
			count |= 0x8000;
			memcpy(&cd[cp],&count,sizeof(short));
		}
		numGroups++;
	}
	cd[0] = numGroups;
	rv = fwrite(cd,sizeof(char),p,stream) == (unsigned int)p;
	free(cd);
	return rv?p:-1;
}
int x_writeSourceInfo(int fileVersion, PTEVAIndexData source, FILE *fp) {
	int nsources=source->nsources;
	PSource sources=source->source;
	int i;
	if( fwrite(&nsources,sizeof(float),1,fp) < 1) return 0;
	for(i=0;i<nsources;i++) {
		if( fwrite(&sources[i].sourceNodeIndex,sizeof(int),1,fp) < 1) return 0;
		if( fwrite(&sources[i].speciesIndex,sizeof(int),1,fp) < 1) return 0;
		if( fwrite(&sources[i].sourceType,sizeof(int),1,fp) < 1) return 0;
		if( fwrite(&sources[i].sourceStrength,sizeof(float),1,fp) < 1) return 0;
		if(fileVersion >= 9) {
			int val;
			val = (int)sources[i].sourceStart;
			if( fwrite(&val,sizeof(int),1,fp) < 1) return 0;
			val = (int)sources[i].sourceStop;
			if( fwrite(&val,sizeof(int),1,fp) < 1) return 0;
		} else {
			long val=sources[i].sourceStart;
			if( fwrite(&sources[i].sourceStart,sizeof(long),1,fp) < 1) return 0;
			if( fwrite(&sources[i].sourceStop,sizeof(long),1,fp) < 1) return 0;
		}
	}
	return 1;
}
int x_writeIndexInfo(FILE *fp,int id, __file_pos_t offset, int fileID, int bytesWritten) {
	if(fwrite(&id,sizeof(int),1,fp) != 1) return 0;
	if(fwrite(&fileID,sizeof(int),1,fp) != 1) return 0;
	if(fwrite(&offset,sizeof(__file_pos_t),1,fp) != 1) return 0;
	if(fwrite(&bytesWritten,sizeof(int),1,fp) != 1) return 0;
	return 1;
}

/*
**--------------------------------------------------------------
**  TSO_WriteSimResults
**  Input:   Pointer to TSO data structure (returned from call
**           to TSO_Create)
**           network size information
**           source information
**           node collection
**  Output:  
**  Purpose: Write the file prologue from the node and link data
**--------------------------------------------------------------
*/
LIBEXPORT(int) TSO_WriteSimResults(PTSO tso, PNetInfo net,PTEVAIndexData source,PNodeInfo nodes)
{
	int t,
		n,
		s,
		nsteps=net->numSteps,
		nnodes=net->numNodes;
	size_t bw=0;
	__file_pos_t fpos=ERD_UTIL_getFilePosition(tso->stream);

	if(fwrite(&(tso->count),sizeof(int),1,tso->stream) != 1) return 0;
	bw+=sizeof(int);
	for(s=0;s<net->numSpecies;s++) {
		float **nodeC;
		for(n=0;n<nnodes;n++) {
			nodes[n].nz[s]=0;
		}
		nodeC=net->qualResults->nodeC[s];
		if(tso->storageMethod == 0) {
			for(t=0;t<nsteps;t++) {
				for(n=0;n<nnodes;n++) {
					if( fwrite(&nodeC[n][t],sizeof(float),1,tso->stream) < 1) return 0;
				}
			}
			bw += nsteps*nnodes*sizeof(float);
		} else {
			int n,t,nnz;

			for(n=0;n<nnodes;n++) {
				for(t=0;t<nsteps;t++) {
					float v = nodeC[n][t];
					if(v != 0) nodes[n].nz[s]=1;
				}
			}
			nnz=0;
			for(n=0;n<nnodes;n++) nnz += nodes[n].nz[s];
			if(fwrite(&nnz,sizeof(int),1,tso->stream) < 1) return 0;
			bw += sizeof(int);
			for(n=1;n<=nnodes;n++) {
				PNodeInfo node = &nodes[n-1];
				if(node->nz[s]) {
					if(fwrite(&n,sizeof(int),1,tso->stream) < 1) return 0;
					bw += sizeof(int);
					if(tso->storageMethod==1) {
						if(fwrite(nodeC[n-1],sizeof(float),nsteps,tso->stream) < (unsigned int)nsteps) return 0;
						bw += sizeof(float)*nsteps;
					} else if(tso->storageMethod==2) {
						int size=x_compressAndStore2(nodeC[n-1],nsteps,tso->stream);
						if(size==-1) return 0;
						bw+=size;
					} else if(tso->storageMethod==3) {
						int size=x_compressAndStore3(nodeC[n-1],nsteps,tso->stream);
						if(size==-1) return 0;
						bw+=size;
					}
				}
			}
		}
	}
	if(!x_writeIndexInfo(tso->indexStream, tso->count,fpos,0,(int)bw)) return 0;
	tso->count++;
	if(!x_writeSourceInfo(tso->fileVersion,source,tso->indexStream)) return 0;
	return 1;
}
/*
**--------------------------------------------------------------
**  TSO_Close
**  Input:   Pointer to TSO data structure (returned from call
**           to TSO_Create or TSO_Open)
**  Output:  
**  Purpose: Close the file and free up any memory used
**--------------------------------------------------------------
*/
LIBEXPORT(void) TSO_Close(PTSO *tso)
{
	int i;
	int singleFile=(*tso)->numFiles==1 && (*tso)->streams[0]==(*tso)->stream;
	if(tso == NULL) return;
	if(*tso == NULL) return;
	if((*tso)->nodedata != NULL)
		free((*tso)->nodedata);
	if((*tso)->linkdata != NULL)
		free((*tso)->linkdata);
	// numFiles could be > 0 and names could be null if it is a single-file TSO
	if(!singleFile) {
		for(i=0; i < (*tso)->numFiles; i++) {
			free((*tso)->names[i]);
		}
		free((*tso)->names);
	}
	// if it is a single-file TSO, streams[0] and stream are the same pointer
	// so only close it in one place.
	if(singleFile) {
		(*tso)->stream=NULL;
	}
	if((*tso)->stream != NULL) {
		fclose((*tso)->stream);
		(*tso)->stream=NULL;
	}
	if((*tso)->indexStream != NULL) {
		fclose((*tso)->indexStream);
		(*tso)->indexStream=NULL;
	}

	free((*tso)->tsodir);
	free((*tso)->baseName);
	if((*tso)->index != NULL) {
		for(i=0;i<(*tso)->count;i++) {
			free((*tso)->index[i]->sourceData->source);
			free((*tso)->index[i]->sourceData);
			free((*tso)->index[i]);
		}
		free((*tso)->index);
	}
	if((*tso)->streams != NULL) {
		for(i=0; i < (*tso)->numFiles; i++) {
			FILE *fp = (*tso)->streams[i];
			if(fp != NULL) {
				fclose(fp);
				(*tso)->streams[i]=NULL;
			}
		}
		free((*tso)->streams);
	}
	free(*tso);
	*tso=NULL;
}

/*
**--------------------------------------------------------------
**  TSO_AllocateNetworkInfo
**  Input:   number of species that will be used
**  Output:  Pointer to newly allocated NetInfo data structure
**  Purpose: Allocate memory needed by the NetInfo data structure
**--------------------------------------------------------------
*/
LIBEXPORT(PNetInfo) TSO_AllocateNetworkInfo(int nspecies) {
	PNetInfo net;
	int i;
	MEMCHECK(net = (PNetInfo)calloc(1,sizeof(NetInfo)),"net in TSO_AllocateNetworkInfo");

	net->numSpecies=nspecies;
	MEMCHECK( net->species = (PSpeciesData*)calloc(net->numSpecies,sizeof(PSpeciesData)),"net->species in TSO_AllocateNetworkInfo" );
	for(i=0;i<nspecies;i++) {
		MEMCHECK( net->species[i] = (PSpeciesData)calloc(1,sizeof(SpeciesData)),"net->species in TSO_AllocateNetworkInfo" );
		net->species[i]->index=1;
		net->species[i]->type=bulk;
		strcpy(net->species[i]->id,"Contamiant");

	}
	return net;
}

/*
**--------------------------------------------------------------
**  TSO_ReleaseNetworkInfo
**  Input:   Pointer to NetInfo data structure
**  Output:  None
**  Purpose: Release memory allocated to the NetInfo data structure
**--------------------------------------------------------------
*/
LIBEXPORT(void) TSO_ReleaseNetworkInfo(PNetInfo *net) {
	int i;
	for(i=0;i<(*net)->numSpecies;i++) {
		free((*net)->species[i]);
	}
	free((*net)->species);
	free(*net);
	*net=NULL;
}

/*
**--------------------------------------------------------------
**  TSO_AllocateNetworkData
**  Input:   NetInfo data
**  Output:  References to newly allocated memory for the
**           NodeInfo and LinkInfo data structures based on the
**           parameters in the input data
**  Purpose: Allocate memory needed by the NodeInfo and LinkInfo
**           data structures
**--------------------------------------------------------------
*/
LIBEXPORT(void) TSO_AllocateNetworkData(PNodeInfo *nodes, PLinkInfo *links, PNetInfo net, int flags) {
    int i,s;
	PNodeInfo tnodes;
	PLinkInfo tlinks;
    MEMCHECK(tnodes=*nodes = (PNodeInfo)calloc(net->numNodes,sizeof(NodeInfo)),"*nodes in TSO_AllocateNetworkData");
	MEMCHECK(net->qualResults=(PQualData)calloc(1,sizeof(QualData)),"net->qualResults in TSO_AllocateNetworkData");
	MEMCHECK(net->hydResults=(PHydData)calloc(1,sizeof(HydData)),"net->hydResults in TSO_AllocateNetworkData");
	if(flags & READ_QUALITY) {
		MEMCHECK(net->qualResults->nodeC=(float***)calloc(net->numSpecies,sizeof(float**)),"net->qualResults->nodeC in TSO_AllocateNetworkData");
		for(s=0;s<net->numSpecies;s++) {
			MEMCHECK(net->qualResults->nodeC[s]=(float**)calloc(net->numNodes,sizeof(float*)),"net->qualResults->nodeC[s] in TSO_AllocateNetworkData");
			for(i=0; i<net->numNodes; i++) {
				MEMCHECK(net->qualResults->nodeC[s][i]=(float*)calloc(net->numSteps,sizeof(float)),"net->qualResults->nodeC[s] in TSO_AllocateNetworkData");
			}
		}
	}
	if(flags & READ_DEMANDS) {
		MEMCHECK(net->hydResults->demand=(float**)calloc(net->numSteps,sizeof(float*)),"net->hydResults->demands in TSO_AllocateNetworkData");
		for(i=0; i<net->numSteps; i++) {
			MEMCHECK(net->hydResults->demand[i]=(float*)calloc(net->numNodes,sizeof(float)),"net->hydResults->demands[i] in TSO_AllocateNetworkData");
		}
	}
    for (i=0; i<net->numNodes; i++) {
        MEMCHECK(tnodes[i].nz = (int*)calloc(net->numSpecies,sizeof(int)),"tnodes[i].nz in TSO_AllocateNetworkData");
        tnodes[i].type = junction;
		tnodes[i].x = net->fltMax;
		tnodes[i].y = net->fltMax;
        memset( tnodes[i].id, '\0', 64+1 );
    }

    MEMCHECK(tlinks = *links = (PLinkInfo)calloc(net->numLinks,sizeof(LinkInfo)),"tlinks[i] in TSO_AllocateNetworkData");
	if(flags & READ_LINKFLOW) {
		MEMCHECK(net->hydResults->flow=(float**)calloc(net->numSteps,sizeof(float*)),"net->hydResults->flow in TSO_AllocateNetworkData");
		for(i=0; i<net->numSteps; i++) {
			MEMCHECK(net->hydResults->flow[i]=(float*)calloc(net->numLinks,sizeof(float)),"net->hydResults->flow[i] in TSO_AllocateNetworkData");
		}
	}
	if(flags & READ_LINKVEL) {
		MEMCHECK(net->hydResults->velocity=(float**)calloc(net->numSteps,sizeof(float*)),"net->hydResults->velocity in TSO_AllocateNetworkData");
		for(i=0; i<net->numSteps; i++) {
			MEMCHECK(net->hydResults->velocity[i]=(float*)calloc(net->numLinks,sizeof(float)),"net->hydResults->velocity[i] in TSO_AllocateNetworkData");
		}
	}
    for (i=0; i<net->numLinks; i++) {                                        /* To/From link nodes */
        tlinks[i].length=net->fltMax;
        tlinks[i].nv=0;
        tlinks[i].vx=NULL;
        tlinks[i].vy=NULL;
        memset( tlinks[i].id, '\0', 64+1 );
    }
}

/*
**--------------------------------------------------------------
**  TSO_ReleaseNetworkData
**  Input:   references to NodeInfo, LinkInfo and NetInfo data
**  Output:  None
**  Purpose: Rerlease memory previously allocated to the NodeInfo
**           and LinkInfo data structures
**--------------------------------------------------------------
*/
LIBEXPORT(void) TSO_ReleaseNetworkData(PNodeInfo *nodes, PLinkInfo *links, PNetInfo net) {
    int i,s;
	PNodeInfo tnodes=*nodes;
	PLinkInfo tlinks=*links;
	for(s=0;s<net->numSpecies;s++) {
		if(net->qualResults->nodeC[s] != NULL) {
			for (i=0; i<net->numNodes; i++) {
				free(net->qualResults->nodeC[s][i]);
			}
			free(net->qualResults->nodeC[s]);
		}
	}
	free(net->qualResults->nodeC);
	free(net->qualResults);
    for (i=0; i<net->numNodes; i++) {
        free(tnodes[i].nz);
		if(tnodes[i].demandProfile!=NULL)
        	free(tnodes[i].demandProfile);
    }
	free(*nodes);
	*nodes=NULL;

	if(net->hydResults->flow != NULL) {
		for (i=0; i<net->numSteps; i++) {
			free(net->hydResults->flow[i]);
		}
		free(net->hydResults->flow);
	}
	if(net->hydResults->velocity != NULL) {
		for (i=0; i<net->numSteps; i++) {
			free(net->hydResults->velocity[i]);
		}
		free(net->hydResults->velocity);
	}
	if(net->hydResults->demand != NULL) {
		for (i=0; i<net->numSteps; i++) {
			free(net->hydResults->demand[i]);
		}
		free(net->hydResults->demand);
	}
	free(net->hydResults);
    for (i=0; i<net->numLinks; i++) {                                        /* From link nodes */
        free(tlinks[i].vx);
        free(tlinks[i].vy);
    }
	free(*links);
	*links=NULL;
}

LIBEXPORT(char *) TSO_GetCompressionDesc(PTSO db)
{
	char *names[]={"None","Non-zero","RLE Non-zero","RLE"};
	return names[db->header[2]];
}


LIBEXPORT(int) loadSimulationResults(int id,PERD erd,PTSO tso,PNetInfo net,PNodeInfo nodes,PTEVAIndexData *source)
{
	if(tso) {
		*source=tso->index[id]->sourceData;
		return TSO_GetSimulationResults(id,tso,net,nodes);
	}
	if(erd) {
		PQualitySim qsr = erd->qualSim[id];
		*source=(PTEVAIndexData)qsr->appData;
		return ERD_getResults(qsr, erd)==0;
	}
	return 0;
}
LIBEXPORT(int) TSO_getCompressionLevel(PTSO db)
{
	return db->storageMethod;
}
