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
/*
 * Modified 2010 Sandia National Laboratories
 * Modified to run flushing commands if decfname is provided and present
 */
/* TEVA header */
#include "merlion/Merlion.hpp"
#include "merlion/BlasWrapper.hpp"
#include "merlion/TriSolve.hpp"
#include "merlion/SparseMatrix.hpp"
#include "merlionUtils/EpanetLinker.hpp"
#include "merlionUtils/Scenarios.hpp"

#include <iostream>
#include <algorithm>
extern "C" {
#include "teva.h"
}

// Defining a cpp compatable version of ERDCHECK. The 'else' value
// is not needed since ERD_Error causes program to exit anyway.
#define ERDCHECKCPP(x) if ((ERRNUM=x) > 0) {ERD_Error(ERRNUM);}
// Added to capture the 'unlink' (now remove) call that was in the C version of this file
#include <cstdio>

#ifdef WIN32
#include "mem_mon.h"
#endif

#ifdef WIN32
#include <fcntl.h>
int mkstemp(char *tmpl)
{
	int ret=-1;
	mktemp(tmpl);
	ret=open(tmpl,O_RDWR|O_BINARY|O_CREAT|O_EXCL|_O_SHORT_LIVED,_S_IREAD|_S_IWRITE);
	return ret;
}
#endif
int run_flushing(FILE *decfile);
int run_hydraulics(PERD db,FILE *decfile);
int run_quality(PERD db, FILE *tsifile, PSourceData source);
int run_quality_msx(PERD db, FILE *tsifile, const char *msx_species, PSourceData source, double **initQual);
void run_quality_merlion(PERD db, FILE *tsifile, PSourceData source_data, merlionUtils::MerlionModelContainer &model, int max_rhs=100);
void printSpecies(PERD db);

void run_flushsim(const char* tsgfname, const char* tsifname, const char* erdName, const char* epanetinpfname, const char* epanetoutfname, const char* decfname, const char *msxfname, const char *msx_species, bool isMERLION, int merlion_max_rhs, bool merlion_ignore_warnings, bool isRLE)
{
    //Merlion variables
    merlionUtils::MerlionModelContainer merlion;
    
	// original tevasim variables
    int version;
    int storageMethod=-1, runensemble=0;
    time_t cpustart, cpustop;
    double cpuelapsed;
//	PNetInfo net;
	PERD db;
//	PNodeInfo nodes;
//	PLinkInfo links;
	PSourceData source=(PSourceData)calloc(1,sizeof(SourceData));
	int isMSX;
	FILE *tsgfile=NULL, *tsifile=NULL;
    FILE *decfile=NULL;
	char tTSIFname[32]="";

	double **initQual;  // species,node: initial quality from msx input file

    source->source=(PSource)calloc(MAXSOURCES,sizeof(Source));
    source->nsources=0;
#ifdef WIN32
	startMemoryMonitorThread("memlog.txt",1000);
#endif
	isMSX=msxfname!=NULL;
    if (tsgfname) {
       TEVAFILEOPEN(tsgfile = fopen(tsgfname, "r"),tsgfname);
       runensemble=1;
       }
    if (tsifname) {
       tsifile = fopen(tsifname, "r+t");
       if (!tsifile)
          TEVAFILEOPEN(tsifile = fopen(tsifname, "w+t"),tsifname);
    }
	if (decfname) {
		decfile = fopen(decfname, "r");
		if (!decfile)
			TEVAFILEOPEN(decfile = fopen(decfname, "r"),decfname);
	}

    /* Open EPANET/MSEPANET and process input file */
    printf("T H R E A T  E N S E M B L E  V U L N E R A B I L I T Y  A N A L Y S I S\n");
    printf("                               (T E V A)\n");
    printf("                    T H R E A T  S I M U L A T O R\n");
    printf("                              Version %d.%d\n\n",TSMAJORVERSION,TSMINORVERSION);
    ENCHECK( (ENopen((char*)epanetinpfname,(char*)epanetoutfname, "")) );
	if(isMSX) {
		MSXCHECK( (MSXopen((char*)msxfname)) );
	}
    ENgetversion(&version);   /* MS/Epanet version */
	if(isMSX) {
        printf("T E V A is using MSEpanet release %d\n",version);
        printf("Initializing MSEpanet \n");
	} else {
        printf("T E V A is using Epanet release %d\n",version);
        printf("Initializing Epanet \n");
	}

	if (isRLE) {ERDCHECKCPP(ERD_create(&db, erdName, teva, rle));}
	else {ERDCHECKCPP(ERD_create(&db, erdName, teva, lzma));}
	ERDCHECKCPP(ENL_getNetworkData(db, (char*)epanetinpfname, (char*)msxfname, (char*)msx_species));
	// set hydraulic storage flags
	ERDCHECKCPP(ERD_setHydStorage(db, TRUE, TRUE, TRUE, FALSE, TRUE));
	if(isMSX)
		ERDCHECKCPP(ENL_saveInitQual(db->network,&initQual));
//    ENTU_Initialize(epanetinpfname, tsofname, storageMethod, tsofileversion, &tso, &net, &nodes, &links, &sources);

    /*****************************************************/
    /* Process demands and demand patterns to create     */
    /* proper demand patterns and agerage demands for    */
    /* use in demand-based population and ingestion.     */
    /*****************************************************/
	ENL_createDemandProfiles(db);
    /* (Optionally) Process threat simulation generator file and write simulation input file */
    if(tsgfile) {
        if(!tsifile) { /* temporary tsi file */
			int tsifd;
			strcpy(tTSIFname,"tsiXXXXXX");
			tsifd=mkstemp(tTSIFname);
            if( (tsifile=fdopen(tsifd,"w+b") ) == NULL ) {
                TEVASimError(1,"Can not create temporary TSI file\n");
			}
        }
		ENL_writeTSI(db->network, db->nodes, source->source, tsgfile, tsifile);
    }
    /*****************************************************/
    /*             Hydraulic Simulation                  */
    /*****************************************************/
    if (isMERLION){
        ENclose();
        Merlion *merlion_model(NULL);
        merlion_model = merlionUtils::CreateMerlionFromEpanet(const_cast<char*>(epanetinpfname), const_cast<char*>(epanetoutfname), "", db->network->simDuration/60.0 , db->network->reportStep/60.0, -1, -1, merlion_ignore_warnings);
        if (!merlion_model) {
                TEVASimError(1,"Error creating the Merlion object from the EPANET inp file.\n");
        }
        ENCHECK( (ENopen(const_cast<char*>(epanetinpfname), const_cast<char*>(epanetoutfname), "")) );
	merlion_model->clear();
        // This class will call delete on merlion_model when it is destroyed
        merlion.GetMerlionMembers(merlion_model);
    }
    
	run_hydraulics(db,decfile);

    /* That's it if no -tsi or -tsg flag */
    time( &cpustart );
    if(tsifile == NULL) goto stop;

    /************************************************************/
    /*             Water Quality Simulations                    */
    /************************************************************/
	if (isMERLION){
           run_quality_merlion(db, tsifile, source, merlion, merlion_max_rhs);
        }
        else if(isMSX){
	  run_quality_msx(db,tsifile,msx_species,source,initQual);
	}
	else {
	  run_quality(db,tsifile,source);
	}
	printSpecies(db);
stop:
    time( &cpustop );
    cpuelapsed = difftime(cpustop,cpustart);
    printf("Elapsed time for quality simulations (seconds): %e\n", cpuelapsed);
    printf("T E V A  Normal  Termination\n");

	/* release all memory allocated */
    if(isMSX){ERDCHECKCPP(ENL_releaseInitQual(db->network,&initQual));}

    free(source->source); /* I'd rather this be part of ReleaseNetworkData... */

    free(source); /* I'd rather this be part of ReleaseNetworkData... */

    /* close any files that have been opened */

    if(tsgfile) {fclose(tsgfile);}

    if(tsifile) {fclose(tsifile);}
    if(strlen(tTSIFname)!=0) {
      std::remove(tTSIFname);
    }

    ERD_close(&db);

    if(isMSX) {MSXclose();}

    /* release EPANET memory */
    ENclose();
}

void printSpecies(PERD db)
{
	size_t maxLen=7; // length of header
	int i;
	for(i=0;i<db->network->numSpecies;i++) {
		size_t idLen=strlen(db->network->species[i]->id);
		maxLen=idLen>maxLen?idLen:maxLen;
	}
	printf("\nSpecies generated/saved:\n");
	printf("  %-*s idx stored?\n",maxLen,"Species");
	for(i=0;i<db->network->numSpecies;i++) {
		PSpeciesData sp=db->network->species[i];
		printf("  %-*s %3d %sstored\n",maxLen,sp->id,sp->index,sp->index==-1?"not ":"");
	}
}
char *TEVAclocktime(char *atime, long time)
{
    int h, m;
    h = time/3600;
    m = (time%3600)/60;
    sprintf(atime, "%01d:%02d", h, m);
    return(atime);
}

/*
**--------------------------------------------------------------
**  TEVASimError
**  Input:   exit code
**           errmsg: printf-like varargs for the error message
**           to print before exiting
**  Output:  none
**  Purpose: Print an error message and exit the program
**--------------------------------------------------------------
*/
void TEVASimError(int exitCode,const char *errmsg, ...)
{
        va_list ap;
 
        va_start(ap,errmsg);
        vfprintf(stderr,errmsg,ap);
        va_end(ap);
 
        exit(exitCode);
}

int run_flushing(FILE *decfile) {
	// flushing variables
	int cidx, nidx, pidx, patLen, ndPatIdx, nPatInSim, period, ival, nodeID, pipeID;
	int status,wrncnt=0;
	int runflushing=0, prnDebugFile=0;
    long pstep, simtime;
	int nFlushNodes, nClosePipes, flushLen, responseDelay, pipeDelay, detectTime, newPeriod;
	float cvtime, ovtime, val, baseDem, flushrate;
	float* patvals;
	char flushPatName[] = "FLUSH-PAT-0000";
	int *flushNodes, *closePipes;
	// read in decision variables
	fscanf(decfile,"%d",&prnDebugFile);
	fscanf(decfile,"%d",&detectTime);
	fscanf(decfile,"%d",&responseDelay);
	fscanf(decfile,"%d",&pipeDelay);
	fscanf(decfile,"%f",&flushrate);
	fscanf(decfile,"%d",&flushLen);
	fscanf(decfile,"%d",&ival);
	// Read decision variables for flushing
	fscanf(decfile,"%d",&nFlushNodes);
	if (ival > 0) {
		simtime = (long)ival * 60 * 60;
		ENsettimeparam(EN_DURATION,simtime);
	}
	flushNodes = (int*)calloc(1+nFlushNodes,sizeof(int));
	if(!flushNodes){
		TEVASimError(1,"Insufficient memory to load decision variables for\nanalysis. Check your system memory.\n");
	}
	for (pidx = 0; pidx < nFlushNodes; pidx++)
	{
		fscanf(decfile,"%d",&ival);
		flushNodes[pidx] = ival;
	}
	// Read decision variables for closing pipes
	fscanf(decfile,"%d",&nClosePipes);
	closePipes = (int*)calloc(1+nClosePipes,sizeof(int));
	if(!closePipes){
		TEVASimError(1,"Insufficient memory to load decision variables for\nanalysis. Check your system memory.\n");
	}
	for (pidx = 0; pidx < nClosePipes; pidx++)
	{
		fscanf(decfile,"%d",&ival);
		closePipes[pidx] = ival;
	}

	fclose(decfile);
	ENgettimeparam(EN_PATTERNSTEP,&pstep);
	ENgettimeparam(EN_DURATION,&simtime);
	nPatInSim = (int) simtime / pstep;
	flushLen = flushLen * 60 * 60;
	responseDelay = responseDelay * 60 * 60;
	pipeDelay = pipeDelay * 60  * 60;
	detectTime = detectTime * 60 * 60;
	cidx = 0;
	pidx = 1;
	cvtime = (float)((detectTime+responseDelay)/pstep); // TODO: Check this - is this for pipes or nodes?
	ovtime = (float)((detectTime+responseDelay+flushLen)/pstep);
	printf(" Flushing: Adjusting junction demands.\n");
	for ( nidx = 0; nidx < nFlushNodes; nidx++)
	{
		nodeID = flushNodes[nidx];
		cidx++;
		sprintf(flushPatName,"FLUSH-PAT-%.4d",cidx);
		if ( status = ENaddpattern(flushPatName) )
		{
			printf(" Flushing: Status %d Adding pattern %s.\n",status,flushPatName);
			if (status < 100) wrncnt++;
			else epanetError(status);
		}
		ENgetpatternindex(flushPatName, &pidx);
		ENgetnodevalue(nodeID,EN_PATTERN,&val);
		ndPatIdx = (int)val;
		ENgetpatternlen(ndPatIdx,&patLen);
		patvals = (float*)calloc(nPatInSim,sizeof(float));
		ENgetnodevalue(nodeID,EN_BASEDEMAND,&val);
		baseDem = val;
		newPeriod = 0;
		while ( newPeriod < nPatInSim ) {
			for ( period = 0; period < patLen && newPeriod < nPatInSim ; period++ )
			{
				ENgetpatternvalue(ndPatIdx,period+1,&val);
				val = val * baseDem;
				if ( newPeriod < cvtime || newPeriod >= ovtime )
				{
					patvals[newPeriod] = val;
				} else {
					patvals[newPeriod] = val + flushrate;
				}
				newPeriod++;
			}
		}
		ENsetpattern(pidx,patvals,nPatInSim);
		free(patvals);
		val = (float)pidx;
		ENsetnodevalue(nodeID,EN_PATTERN,val);
		ENsetnodevalue(nodeID,EN_BASEDEMAND,1.0);
	}
	// close pipes
	printf(" Flushing: Closing pipes.\n");
	cvtime = (float)(detectTime + responseDelay + pipeDelay);
	ovtime = (float)(detectTime + responseDelay + flushLen + pipeDelay);
	cidx = 0;
	for ( nidx = 0; nidx < nClosePipes; nidx++)
	{
		pipeID = closePipes[nidx];
		cidx++;
		ENsetcontrol(cidx, EN_TIMER, pipeID, 0, 0, cvtime);
		cidx++;
		ENsetcontrol(cidx, EN_TIMER, pipeID, 1, 0, ovtime);
	}
	free(flushNodes);
	free(closePipes);
	if (prnDebugFile) {
		ENsaveinpfile("ts_debug.inp");
	}
	return wrncnt;
}
int run_hydraulics(PERD db,FILE *decfile)
{
    long entime, tstep, rtime, rstep, astep;
	int status,wrncnt,hsteps;
    char atime[10];
	PNetInfo net=db->network;
	PNodeInfo nodes=db->nodes;
	PLinkInfo links=db->links;

	tstep = net->reportStep - net->reportStart; /* initial averaging step when reportstart < reportstep */
    hsteps = 0;
    wrncnt = 0;
    ENCHECK( ENopenH() );
    /*****************************************************/
    /*             Change Flushing Patterns              */
    /*             and Close Pipes                       */
    /* -dbhart, 2010-Jan                                 */
    /*****************************************************/
	if (decfile != NULL) {
		wrncnt+=run_flushing(decfile);
	}
	ENCHECK( ENinitH(1) );
    printf("Computing network hydraulics at time = ");
    do {
        if ( status = ENrunH(&entime) )
        {
            if (status < 100) wrncnt++;
            else epanetError(status);
        }
        /* Compute time of, and step to, next reporting interval boundary */
        if (entime <= net->reportStart) rtime = net->reportStart;
        else if (MOD(entime - net->reportStart,net->reportStep) != 0) {
            rtime = net->reportStart + net->reportStep*(1 + (entime - net->reportStart)/net->reportStep);
        } 
        else rtime = entime;
        rstep = rtime - entime;

        printf("%-7s", TEVAclocktime(atime, entime));
        if (rstep < net->reportStep && hsteps < net->numSteps ) {
            /* Accumulate time averaged node and link data in TSO data structures */
            astep = MIN(tstep,net->reportStep - rstep);
			ENL_getHydResults(hsteps,astep,db);
            if ( rstep == 0 ) hsteps++; /* End of current averaging report interval */
        }

        if ( status = ENnextH(&tstep) )
        {
            if (status < 100) wrncnt++;
            else epanetError(status);
        }
        printf("\b\b\b\b\b\b\b");
    } while (tstep > 0);
    ENcloseH();
    ENsavehydfile("hydraulics.hyd");
    MSXusehydfile("hydraulics.hyd");

    /* Check for hydraulic simulation errors and issue warnings */
    printf("\n");
    if (wrncnt) {
        printf("One or more warnings occurred during the hydraulic\n");
        printf("analysis.  Check the EPANET output file.\n");
    }
    if ( hsteps != net->numSteps ) TEVASimError(1,"Inconsistent calculated/actual hydraulic time steps");

	ERDCHECKCPP(ERD_newHydFile(db));

	ERDCHECKCPP(ERD_writeHydResults(db));

	return wrncnt;
}


int run_quality(PERD db, FILE *tsifile, PSourceData source)
{
    int  is, wqsteps, wrncnt, status, scenario;
    int  storageMethod=-1, runensemble=0;
    long entime, tstep, rtime, rstep, astep;
	char atime[10];
	PNetInfo net=db->network;
	PNodeInfo nodes=db->nodes;
	PLinkInfo links=db->links;

	ENCHECK( ENopenQ() );
    printf("Computing network water quality:\n");
    scenario=0;
	while ( ENL_setSource(source, net, tsifile,0) ) /* Load the next scenario define in TSI file */
    {
    	PSource sources=source->source;
    	int prevEntime;
		PTEVAIndexData indexData;
    	int stopTime=net->simDuration;
        tstep = net->reportStep - net->reportStart; /* initial averaging step when reportstart < reportstep */
        wrncnt = 0;
        wqsteps = 0;
        entime=0;
        scenario++;
        printf("Scenario %05d, injection nodes ",scenario);
        for (is=0; is < source->nsources; is++) printf("%+15s, ",sources[is].sourceNodeID);
        printf(" time");

		ERD_clearQualityData(db);
        ENCHECK( ENinitQ(0) );
        do {
        	prevEntime=entime;
            if ( status = ENrunQ(&entime) ) {
                if (status < 100)
                    wrncnt++;
                else epanetError(status);
            }
            printf("%-7s", TEVAclocktime(atime, entime));

            /* Compute time of, and step to, next reporting interval boundary */
            if (entime <= net->reportStart) rtime = net->reportStart;
            else if (MOD(entime - net->reportStart,net->reportStep) != 0) {
                rtime = net->reportStart + net->reportStep*(1 + (entime - net->reportStart)/net->reportStep);
            } 
            else rtime = entime;
            rstep = rtime - entime;

            /* Set source strengths and types - order is important if duplicate source nodes */
			for (is=0; is < source->nsources; is++) {
				if ( entime >= sources[is].sourceStart && entime < sources[is].sourceStop ) { /* Source is on */
					ENCHECK( ENsetnodevalue(sources[is].sourceNodeIndex,EN_SOURCEQUAL,sources[is].sourceStrength) );
					ENCHECK( ENsetnodevalue(sources[is].sourceNodeIndex,EN_SOURCETYPE,(float)sources[is].sourceType) );
				} else { /* Source is off */
					ENCHECK( ENsetnodevalue(sources[is].sourceNodeIndex,EN_SOURCEQUAL,0.0) );
					ENCHECK( ENsetnodevalue(sources[is].sourceNodeIndex,EN_SOURCETYPE,EN_CONCEN) );
				}
			}
            /* Save results */
            if ( rstep < net->reportStep && wqsteps < net->numSteps ) {

                /* Accumulate time averaged node data in TSO data structures */
                astep = MIN(tstep,net->reportStep - rstep);
                astep=entime-prevEntime;
                ENL_getQualResults(wqsteps,astep,db);
                if ( rstep == 0 ) wqsteps++; /* End of current averaging report interval */
            }

            if(tstep > 0) {
				if ( status = ENstepQ(&tstep) ) {
					if (status < 100)
						wrncnt++;
					else epanetError(status);
				}

				printf("\b\b\b\b\b\b\b");
            }
        } while (entime < stopTime);


        /* Check for errors and issue warnings */
        printf("\n");
        if (wrncnt) {
            printf("\nOne or more warnings occurred during the water quality\n");
            printf("analysis, check MS/EPANET output file\n");
        }
        if ( wqsteps != net->numSteps ) TEVASimError(1,"Inconsistent calculated/actual water quality time steps");

        /* Write scenario results to TSO database */
		indexData=newTEVAIndexData(source->nsources,source->source);
		ERD_writeQualResults(db,indexData);
    } /* End of while */

	ENcloseQ();
	return wrncnt;
}
int run_quality_msx(PERD db, FILE *tsifile, const char *msx_species, PSourceData source, double **initQual)
{
    int  is, wqsteps, wrncnt, status, scenario;
    int  storageMethod=-1, runensemble=0;
    long entime, tstep, rtime, rstep, astep;
	char atime[10];
	PNetInfo net=db->network;
	PNodeInfo nodes=db->nodes;
	PLinkInfo links=db->links;

    printf("Computing network water quality:\n");
    scenario=0;
	while ( ENL_setSource(source, net, tsifile,1) ) /* Load the next scenario define in TSI file */
	{
		PSource sources=source->source;
    	int prevEntime;
		PSourceData indexData;
    	int stopTime=net->simDuration;
        tstep = net->reportStep - net->reportStart; /* initial averaging step when reportstart < reportstep */
        wrncnt = 0;
        wqsteps = 0;
        entime=0;
        scenario++;
        printf("Scenario %05d, injection nodes ",scenario);
        for (is=0; is < source->nsources; is++) printf("%+15s, ",sources[is].sourceNodeID);
        printf(" time");

		ERD_clearQualityData(db);
		ENL_restoreInitQual(db->network,initQual);
        ENCHECK( MSXinit(0) );
        ENL_getQualResults(wqsteps,net->reportStep,db);
		wqsteps++;
        do {
            /* Set source strengths and types - order is important if duplicate source nodes */
			for (is=0; is < source->nsources; is++) {
				if ( entime >= sources[is].sourceStart && entime < sources[is].sourceStop ) { /* Source is on */
					ENCHECK( MSXsetsource(sources[is].sourceNodeIndex,sources[is].speciesIndex+1,sources[is].sourceType,sources[is].sourceStrength,0) );
				} else { /* Source is off */
					ENCHECK( MSXsetsource(sources[is].sourceNodeIndex,sources[is].speciesIndex+1,MSX_NOSOURCE,0,0) );
				}
			}
        	prevEntime=entime;
            if ( status = MSXstep(&entime,&tstep) ) {
                if (status < 100)
                    wrncnt++;
                else epanetError(status);
            }
            printf("%-7s", TEVAclocktime(atime, entime));
			printf("\b\b\b\b\b\b\b");

            /* Compute time of, and step to, next reporting interval boundary */
            if (entime <= net->reportStart) rtime = net->reportStart;
            else if (MOD(entime - net->reportStart,net->reportStep) != 0) {
                rtime = net->reportStart + net->reportStep*(1 + (entime - net->reportStart)/net->reportStep);
            } 
            else rtime = entime;
            rstep = rtime - entime;

            /* Save results */
            if ( rstep < net->reportStep && wqsteps < net->numSteps ) {

                /* Accumulate time averaged node data in TSO data structures */
                astep = MIN(tstep,net->reportStep - rstep);
                astep=entime-prevEntime;
                ENL_getQualResults(wqsteps,astep,db);
                if ( rstep == 0 ) wqsteps++; /* End of current averaging report interval */
            }

		} while (entime < stopTime);


        /* Check for errors and issue warnings */
        printf("\n");
        if (wrncnt) {
            printf("\nOne or more warnings occurred during the water quality\n");
            printf("analysis, check MS/EPANET output file\n");
        }
        if ( wqsteps != net->numSteps ) TEVASimError(1,"Inconsistent calculated/actual water quality time steps");

        /* Write scenario results to TSO database */
		indexData=newTEVAIndexData(source->nsources,source->source);
		ERD_writeQualResults(db,indexData);
    } /* End of while */

	return wrncnt;
}


/////////////////////////////// merlion ///////////////////////////////////
void MerlionError(int exitCode, const char *errmsg, ...) //similar to enlError from enl.c
{
        va_list ap;
 
        va_start(ap, errmsg);
        vfprintf(stderr, errmsg, ap);
        va_end(ap);
 
        exit(exitCode);
}

static int getTokens(char *s, char **tok) //from enl.c
{
    int  len, m, n;
    char *c;

    /* Begin with no tokens */
    for (n=0; n<MAXTOKS; n++) tok[n] = NULL;
    n = 0;

    /* Truncate s at start of comment */
    c = strchr(s,';');
    if (c) *c = '\0';
    len = (int)strlen(s);

    /* Scan s for tokens until nothing left */
    while (len > 0 && n < MAXTOKS)
    {
        m = (int)strcspn(s,SEPSTR);         /* Find token length */
        len -= m+1;                    /* Update length of s */
        if (m == 0) s++;               /* No token found */
        else
        {
            if (*s == '"')               /* Token begins with quote */
            {
                s++;                       /* Start token after quote */
                m = (int)strcspn(s,"\"\n\r");   /* Find end quote (or EOL) */
            }
            s[m] = '\0';                 /* Null-terminate the token */
            tok[n] = s;                  /* Save pointer to token */
            n++;                         /* Update token count */
            s += m+1;                    /* Begin next token */
        }
    }
    return(n);
}

static int getFloat(char *s, float *y) //from enl.c
{
    char *endptr;
    *y = (float) strtod(s,&endptr);
    if (*endptr > 0) return(0);
    return(1);
}

static int getInt(char *s, int *y) //from enl.c
{
    char *endptr;
    *y = (int)strtol(s,&endptr,10);
    if (*endptr > 0) return(0);
    return(1);
}


static int getLong(char *s, long *y) //from enl.c
{
    char *endptr;
    *y = strtol(s,&endptr,10);
    if (*endptr > 0) return(0);
    return(1);
}

int MERLION_setSource(PSourceData source_data, PNetInfo net, FILE *simin, int isMSX) //similar to ENL_setSource from enl.c
/*
**--------------------------------------------------------------
**  Input:   TSO Sources and Network structures, and TSI file ptr.
**  Output:  structure sources contains name of Epanet source
**           node ID, source start time (seconds), source stop
**           time (seconds), source strength, and species index
**           for all sources.  net->nsources
**           contains the number of sources.
**           Returns 1 if successful; 0 if not
**  Purpose: Reads one line of threat scenario input file and returns
**           source parameters
**--------------------------------------------------------------
*/
{
    if(isMSX) {
            MerlionError(1, "Merlion does not support MULTISPECIES");
    }
        
    int   is;
    char  line[MAXLINE+1],     /* Line from input data file       */
          wline[MAXLINE+1];    /* Working copy of input line      */
    int   ntokens = 0,         /* # tokens in input line          */
          tokspersource = 6,   /* # tokens to describe one source */
          nreadtoks;           /* # of read tokens                */
    char  *tok[MAXTOKS];       /* Array of token strings          */

    /* Read a line from input file. */
    while (fgets(line,MAXLINE,simin) != NULL)
    {
        PSource sources=source_data->source;
        /* Make copy of line and scan for tokens */
        strcpy(wline,line);
        ntokens = getTokens(wline,tok);

        /* Skip blank lines and comments */
        if (ntokens == 0) continue;
        if (*tok[0] == ';') continue;

        /* Check if max. length exceeded */
        if (strlen(line) >= MAXLINE) MerlionError(1,"TSI file error");

        /* Check for proper number of tokens */
        if ( ntokens%tokspersource != 0 ) MerlionError(1,"TSI file error");

        /* Otherwise process input line */
        if ( (source_data->nsources = ntokens/tokspersource) > MAXSOURCES) MerlionError(1,"TSI file error");          /* Max # of quality sources   */
        for (is = 0; is < source_data->nsources; is++) {
            nreadtoks = is*tokspersource;
            if ( strlen(tok[nreadtoks]) > 64 ) MerlionError(1,"TSI file error: Node ID too long");
            ENCHECK( ENgetnodeindex(tok[nreadtoks],&sources[is].sourceNodeIndex) );                    /* Node index of source       */
            strcpy(sources[is].sourceNodeID,tok[nreadtoks]);                                           /* source node ID             */
            if ( !getInt(tok[nreadtoks+1],&sources[is].sourceType) ) MerlionError(1,"TSI file error"); /* Source type                */
            if ( sources[is].sourceType < EN_CONCEN || sources[is].sourceType > EN_FLOWPACED ) MerlionError(1,"TSI file error");
            if ( !getInt(tok[nreadtoks+2],&sources[is].speciesIndex) ) MerlionError(1,"TSI file error"); /* Species Index                */
            if ( sources[is].speciesIndex < 0 || sources[is].speciesIndex > net->numSpecies ) MerlionError(1,"TSI file error");
            if ( !getFloat(tok[nreadtoks+3],&sources[is].sourceStrength) ) MerlionError(1,"TSI file error");  /* source strength            */
            if ( !getLong(tok[nreadtoks+4],&sources[is].sourceStart) ) MerlionError(1,"TSI file error");      /* start time                 */
            if ( sources[is].sourceStart < 0 || sources[is].sourceStart > net->simDuration ) MerlionError(1,"TSI file error: SourceStart");
            if ( !getLong(tok[nreadtoks+5],&sources[is].sourceStop) ) MerlionError(1,"TSI file error");       /* stop time                  */
            if ( sources[is].sourceStop < sources[is].sourceStart || sources[is].sourceStop > net->simDuration ) MerlionError(1,"TSI file error: SourceStop");
        }
        return(1);
    }   /* End of while */
  return(0);
}

void run_quality_merlion(PERD db, FILE *tsifile, PSourceData source_data, merlionUtils::MerlionModelContainer &model, int max_rhs/*=100*/) // similar to run_quality
{

    /// TODO: We need to make this more general. It may not be appropriate for all scenario types.
    const float zero_conc_tol_mgpL = 1e-10;
    ///

    // In order for concentration profiles to match up with EPANET, we
    // need to shift all injection start and stop times over by a
    // single timestep
    double EPANET_injection_offset_hack = 60.0*model.qual_step_minutes;

    // There is alot of tricky logic that goes into setting the
    // rhs array for a merlion linear solve. This is all handled
    // by the Injection class, so we utilize it here
    merlionUtils::Injection injection;

    PNetInfo net=db->network;
    int n_nodes = model.n_nodes;
    int n_steps = model.n_steps;
    int N = model.N;
    const SparseMatrix *G = model.G;
    const int *perm_nt_to_upper = model.perm_nt_to_upper;
    const int *perm_upper_to_nt = model.perm_upper_to_nt;
    const int *D_rhs = model.D;
    const float *node_flow_m3pmin = model.flow_m3pmin;


    // **NOTE: Running the multiple rhs routine with values not much
    //         larger than 1 (e.g. 2-8) will actually degrade
    //         performance. So rather than try to explain this in the
    //         command line options, we will just prevent this from
    //         happening here. The magic number chosen here is in
    //         reality dependent on a number of factors (e.g. system
    //         architecture, network model, etc.). However, speedup
    //         from the multiple rhs routine really only becomes
    //         apparent at larger numbers (e.g 50,100,etc.).
    if (max_rhs > 8) {
       int scenario=0;
       
       // TODO: Figure out how many scenarios exist in the tsifile and
       //       reduce the max_rhs if greater than that.  We could
       //       also make the last block of mrhs simulations more
       //       efficient if we knew the number of scenarios left.
       /*
       if (max_rhs > __NUMBER_OF_SCENARIOS__) {
          max_rhs= __NUMBER_OF_SCENARIOS__;
       }
       */

       float* X = new float[N*max_rhs];
       float** node_conc = new float*[n_nodes];
       float* C = new float[N*max_rhs];
       BlasInitZero(N*max_rhs, X);
       BlasInitZero(N*max_rhs, C);   
       
       printf("Computing network water quality with MERLION\n");
       
       if(net->qualCode == MULTISPECIES) {
          MerlionError(1, "Merlion does not support MULTISPECIES");
       }

       PTEVAIndexData* pindexData(NULL);
       pindexData = new PTEVAIndexData[max_rhs];
    
       bool done = false;
       while (!done) {
          int rhs_cnt = 0;

          // the upper triangular matrix is in "reverse-time"
          // order. Therefore the earliest injection time affects the
          // max_row we need in the solve.
          //   - min_row corresponds to the end of the simulation
          //   - max_row corresponds to the beginning of the first
          //     injection over all the right hand sides
          int max_row = 0;
          const int min_row = 0;

          while (rhs_cnt < max_rhs) {
             if (!MERLION_setSource(source_data, net, tsifile,0)) {
                done = true;
                break;
             }
             PSource sources=source_data->source;
             for (int i=0; i < source_data->nsources; ++i) {

                injection.NodeName() = model.NodeName(sources[i].sourceNodeIndex-1);
                injection.StartTimeSeconds() = sources[i].sourceStart+EPANET_injection_offset_hack;
                injection.StopTimeSeconds() = sources[i].sourceStop+EPANET_injection_offset_hack;
                if (sources[i].sourceType == EN_MASS) {
                   injection.Type() = merlionUtils::InjType_Mass;
                   // MASS units from tsg are in mg/min
                   // Merlion takes in g/min
                   injection.Strength() = sources[i].sourceStrength*0.001;
                } else if (sources[i].sourceType == EN_FLOWPACED) {
                   injection.Type() = merlionUtils::InjType_Flow;
                   // FLOWPACED units from tsg are in mg/L
                   // Merlion takes in g/m^3 (no conversion required)
                   injection.Strength() = sources[i].sourceStrength;
                } else {
                   MerlionError(1, "Merlion does not support source type");
                }
                float mc_g;
                int max_row_scen;
                injection.SetMultiArray(model,max_rhs,rhs_cnt,X,mc_g,max_row_scen);
                if (max_row_scen > max_row) {
                   max_row = max_row_scen;
                }

             }
             pindexData[rhs_cnt] = newTEVAIndexData(source_data->nsources,sources);
             ++rhs_cnt;
          }

          // do backsolve
          usolvem(N, min_row, max_row, max_rhs, G->Values(), G->iRows(), G->pCols(), X);

          //  mix_X_idx is the starting X index that was used in the
          //  "trimmed" backsolve
          int min_X_idx = min_row*max_rhs;
          int max_X_idx = (max_row+1)*max_rhs-1;
          // Loop through solution
          for (int u_rhs_idx = min_X_idx; u_rhs_idx <= max_X_idx; ++u_rhs_idx) {
             if (X[u_rhs_idx] >= zero_conc_tol_mgpL) {
                int rhs = u_rhs_idx % max_rhs; // which rhs are we considering
                int u_idx = u_rhs_idx/max_rhs; // which permuted block are we in
                int nt_idx = perm_upper_to_nt[u_idx];
                C[rhs*N+nt_idx] = X[u_rhs_idx];
             }
          }

          // This is totally goofy - we need to backup the existing structure
          // and restore it after we are done
          float** nodeC_backup = db->network->qualResults->nodeC[0];
          for (int rhs = 0; rhs < rhs_cnt; ++rhs) {
             int group = rhs*N;
             for (int n=0; n<n_nodes; ++n) {
                node_conc[n] = &(C[group + n*n_steps]);
             }
             db->network->qualResults->nodeC[0] = node_conc;
             ERD_writeQualResults(db,pindexData[rhs]);
          }
          db->network->qualResults->nodeC[0] = nodeC_backup;
          
          // reset to zero only the values where X,C
          // was possibly modified by linear solver, this saves time
          const int num_elements = max_rhs*(max_row-min_row+1);
          const int p_offset = min_row*max_rhs;
          BlasResetZero(num_elements, X+p_offset);
          BlasResetZero(N*max_rhs, C);
          
       }
       
       delete [] pindexData;
       pindexData = NULL;
       
       delete [] X;
       X = NULL;
       delete [] C;
       C = NULL;
       delete [] node_conc;
       node_conc = NULL;
    }
    else {
       int scenario=0;
       
       PNetInfo net=db->network;
       
       float* X = new float[n_nodes*n_steps];
       BlasInitZero(n_nodes*n_steps, X);
       
       printf("Computing network water quality with MERLION:\n");
       
       if(net->qualCode == MULTISPECIES) {
          MerlionError(1, "Merlion does not support MULTISPECIES");
       }

       bool do_loop = MERLION_setSource(source_data, net, tsifile,0);
       
       while ( do_loop ) {/* Load the next scenario defined in TSI file */
          PSource sources=source_data->source;
          
          ERD_clearQualityData(db);
          
          // the upper triangular matrix is in "reverse-time"
          // order. Therefore the min_time affects the max_row we need
          // in the solve.  min_row corresponds to the end of the
          // simulation max_row corresponds to the beginning of the
          // first injection in the scenario
          int max_row = 0;
          const int min_row = 0;

          for (int i=0; i < source_data->nsources; i++) {

             injection.NodeName() = model.NodeName(sources[i].sourceNodeIndex-1);
             injection.StartTimeSeconds() = sources[i].sourceStart+EPANET_injection_offset_hack;
             injection.StopTimeSeconds() = sources[i].sourceStop+EPANET_injection_offset_hack;
             if (sources[i].sourceType == EN_MASS) {
                injection.Type() = merlionUtils::InjType_Mass;
                // MASS units from tsg are in mg/min
                // Merlion takes in g/min
                injection.Strength() = sources[i].sourceStrength*0.001;
             } else if (sources[i].sourceType == EN_FLOWPACED) {
                injection.Type() = merlionUtils::InjType_Flow;
                // FLOWPACED units from tsg are in mg/L
                // Merlion takes in g/m^3 (no conversion required)
                injection.Strength() = sources[i].sourceStrength;
             } else {
                MerlionError(1, "Merlion does not support source type");
             }
             float mc_g;
             injection.SetArray(model,X,mc_g,max_row);

          }
          
          // do backsolve
          usolve(N, min_row, max_row, G->Values(), G->iRows(), G->pCols(), X);

          // fill in the teva solution vector in nt format
          float *C = net->qualResults->nodeC[0][0];
          for (int i=min_row; i<=max_row; ++i) {
             if (X[i] >= zero_conc_tol_mgpL) {
                C[perm_upper_to_nt[i]] = X[i];
             }
          }

          /* Write scenario results to ERD database */
          PTEVAIndexData indexData = newTEVAIndexData(source_data->nsources,sources);
          ERD_writeQualResults(db,indexData);
          
          // Reset the merlion rhs vector to all zeros reset to zero
          // only the values where X was possibly modified by linear
          // solver, this saves time
          const int num_elements = max_row-min_row+1;
          const int p_offset = min_row;
          BlasResetZero(num_elements, X+p_offset);
          
          do_loop = MERLION_setSource(source_data, net, tsifile,0);
       } /* End of while */

       delete [] X;
       X = NULL;
    }
}
