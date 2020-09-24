/*
 * Copyright © 2008 UChicago Argonne, LLC
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
#include <stdlib.h>
#include <string.h>

#include "nrutil.h"
#include "tai.h"
#include "ta_utils.h"
#include "ta_input.h"

/*
**-------------------------------------------------------------
** Input data file section headers 
**-------------------------------------------------------------
*/
static int x_sortFloatAsc(const void *a, const void *b);

enum secttype 
{
	_SPECIES_NAME,  /* the name of the species - if unspecified, then assume index 0 */
	_DR_LD50,       /* LD/ID 50 value for chemical */
	_DR_BETA,       /* Beta value for Probit equation */
	_DR_TYPE,       /* Dose Response type (either "old" or "probit") */
	_DR_A,          /* Sigmoid dose-response coefficient */
	_DR_M,          /* Sigmoid dose-response coefficient */
	_DR_N,          /* Sigmoid dose-response coefficient */
	_DR_TAU,        /* Sigmoid dose-response coefficient */
	_DOSE_THRESHOLDS, /* Dose thresholds - the number of people over theses thresholds will be reported.  more than one is allowed separated by whitespace */
	_DOSE_BINDATA,  /* Dose Histogram bin values - each value specifies the endpoint of a bin that will contain the number of people receiving a dose between those thresholds.  The values are specified as response values and the dose values are calculated using the DR data */
	_BODYMASS,      /* Body mass (Kg) */
	_NORMALIZE,     /* YES|NO Normalize dose by body mass */
	_LATENCYTIME,   /* Time from exposure to disease symptoms */
	_FATALITYTIME,  /* Time from symptoms to death */
	_FATALITYRATE,  /* Fraction exposed that die */
	_INGESTIONRATE, /* Volumetric ingestion rate */
	_TSONAME,       /* Name of TSO file */
	_TAONAME,       /* Name of output file */
	_SAPNAME,       /* Name of sensor parameter output file */
	_SAPVARS,       /* Sensor parameters to output.  Current valid values: MeanInfections  MaxInfections*/
	_DOSETYPE,      /* TOTAL (total mass dose) */
	_POPULATION,    /* Population source ('demand usagerate' or 'file filename') */
	_INGESTIONTYPE, /* DEMAND (ingestion proportional to demand) */
    _THRESHOLD,     /* Concentration threshold - all concentration values below the specified value are set to zero */
};

char 
    *secttxt[]   = {
	"SPECIES_NAME",    /* the name of the species - if unspecified, then assume index 0 */
	"DR:LD50",         /* LD/ID 50 value for chemical */
	"DR:BETA",         /* Beta value for Probit equation */
	"DR:TYPE",         /* Dose Response type (either "old" or "probit") */
    "DR:A",            /* dose-response 'a' coefficient */
    "DR:M",            /* dose-response 'm' coefficient */
    "DR:N",            /* dose-response 'n' coefficient */
    "DR:TAU",          /* dose-response 'tau' coefficient */
	"DOSE_THRESHOLDS", /* Dose thresholds - the number of people over theses thresholds will be reported.  more than one is allowed separated by whitespace */
	"DOSE_BINDATA",    /* Dose Histogram bin values - each value specifies the endpoint of a bin that will contain the number of people receiving a dose between those thresholds.  The values are specified as response values and the dose values are calculated using the DR data */
    "BODYMASS",        /* exposed individual body mass (Kg) */
    "NORMALIZE",       /* Response computed using mg/Kg ('yes') or mg ('no') */
    "LATENCYTIME",     /* time from being exposed to exhibiting definitive symptoms (hours) */
    "FATALITYTIME",    /* time from exhibiting symptoms till death or recovery (hours) */
    "FATALITYRATE",    /* Fraction of exposed population that die */
    "INGESTIONRATE",   /* volumetric ingestion rate (liters/day) */
    "TSONAME",         /* name of TSO file */
    "TAONAME",         /* name of TAO file */
	"SAPNAME",         /* Name of sensor parameter output file */
	"SAPVARS",         /* Sensor parameters to output.  Current valid values: MeanInfections  MaxInfections*/
    "DOSETYPE",        /* 'total' for total ingested mass */
	"POPULATION",      /* Population source ('demand usagerate' or 'file filename') */
    "INGESTIONTYPE",   /* 'demand' for ingestion probability proportional to node demand */
    "THRESHOLD",       /* Concentration threshold - all concentration values below the specified value are set to zero */
    NULL
};

/*
**-----------------------------------------------------------
**  loadTAIFileData
**  Input:   tai data file name
**  Output:  Pointer to input file data contents (for later
**           processing)
**  Purpose: Loads data file contents into memory for later
**           processing
**-----------------------------------------------------------
*/
LIBEXPORT(PInputFileData) loadTAIFileData(const char *fn)
{
	return loadInputFile(fn,secttxt);
}


void loadPopulationFile_impl(char *fn,PopulationDataPtr pd, char *srcFN)
{
	FILE *fp;
	char line[1024];
        char *tmpc;

	fp=fopen(fn,"r");
	if((fp == NULL) && ( srcFN != NULL )) {
           // Attempt a relative path name match
           tmpc = strrchr(srcFN,'/');
           if ( tmpc == NULL )
              tmpc = strrchr(srcFN,'\\');
           if ( tmpc != NULL ) {
              if ( strlen(fn) + 1+(tmpc-srcFN) + 1 > 1024 )
                 ta_error(1,"Buffer overflow processing TAI population file: %s\n",fn);
              strncpy(line, srcFN, 1+(tmpc-srcFN));
              strcpy(line+1+(tmpc-srcFN), fn);
              fp=fopen(line,"r");
           }
	}
	if(fp == NULL) {
           ta_error(1,"Unable to open population file: %s\n",fn);
        }
	while(!feof(fp)) {
		char *nodeid,*spv;
		float popValue;
		int i;

		fgets(line,sizeof(line),fp);
		if(!feof(fp)) {
			nodeid = strtok(line," \t");

			spv=strtok(NULL," \t");
			popValue = (float)atof(spv);
			i=pd->popLength;
			pd->popLength++;
			pd->pop = (PNodePopulation)realloc(pd->pop,pd->popLength*sizeof(NodePopulation));
			pd->pop[i].nodeid = (char *)calloc(strlen(nodeid)+1,sizeof(char));
			strcpy(pd->pop[i].nodeid,nodeid);
			pd->pop[i].population = popValue;
			pd->pop[i].used=0;
		}
	}
	fclose(fp);
}


/*
**-----------------------------------------------------------
**  processSensorSetData
**  Input:   Pointer to input file data contents
**  Output:  Updated assessment parameters based on data
**           file contents
**  Purpose: Processes TAI data and updates parameters
**-----------------------------------------------------------
*/
LIBEXPORT(void) processTAIData(PInputFileData taiIFD, PMem mem)
{
    FILE * fp;
    int   
        i;
    float 
        x;

	mem->dr->speciesIndex=-1;
    /* data model declaration */
	for(i=0;i<taiIFD->num;i++)
	{
		PInputLine il = &taiIFD->inputLines[i];
        /* Process line depending on input keyword */
        int sect = il->idx;
		int ntokens = il->ntokens;
		char **tok=il->tokens;

        switch( sect )
        {
		case _SPECIES_NAME:
			if( ntokens != 2 ) ta_error(1,"Invalid input SPECIES");
			if(mem->dr->speciesName != NULL) ta_error(1,"SPECIES already specified");
			mem->dr->speciesName = (char *)calloc(strlen(tok[1])+1,sizeof(char));
			strcpy(mem->dr->speciesName,tok[1]);
			if(mem->tso!=NULL) {
				mem->dr->speciesIndex=0;
			} else {
				mem->dr->speciesIndex=ERD_getSpeciesIndex(mem->erd,tok[1]);
			}
			break;
		case _DR_LD50:
			if( ntokens != 2 ) ta_error(1,"Invalid input DR:LD50");
				  if ( TAgetfloat(tok[1],&x) ) mem->dr->ld50=x;
			else ta_error(1,"Invalid input DR:LD50");
					break;
		case _DR_BETA:
			if( ntokens != 2 ) ta_error(1,"Invalid input DR:BETA");
				  if ( TAgetfloat(tok[1],&x) ) mem->dr->beta=x;
			else ta_error(1,"Invalid input DR:BETA");
					break;
		case _DR_TYPE:
			if( ntokens != 2 ) ta_error(1,"Invalid input DR:TYPE");
			if(!strcasecmp(tok[1],"PROBIT")) mem->dr->responseType = Probit;
			else if(!strcasecmp(tok[1],"OLD")) mem->dr->responseType = Old;
			else ta_error(1,"Invalid input DR:TYPE");
				break;
        case _DR_A:
            if( ntokens != 2 ) ta_error(1,"Invalid input DR:A");
            if ( TAgetfloat(tok[1],&x) ) mem->dr->a=x;
            else ta_error(1,"Invalid input DR:A");
            break;
        case _DR_M:
            if( ntokens != 2 ) ta_error(1,"Invalid input DR:M");
            if ( TAgetfloat(tok[1],&x) ) mem->dr->m=x;
            else ta_error(1,"Invalid input DR:M");
            break;
        case _DR_N:
            if( ntokens != 2 ) ta_error(1,"Invalid input DR:N");
            if ( TAgetfloat(tok[1],&x) ) mem->dr->n=x;
            else ta_error(1,"Invalid input DR:N");
            break;
        case _DR_TAU:
            if( ntokens != 2 ) ta_error(1,"Invalid input DR:TAU");
            if ( TAgetfloat(tok[1],&x) ) mem->dr->tau=x;
            else ta_error(1,"Invalid input DR:TAU");
            break;
        case _BODYMASS:
            if( ntokens != 2 ) ta_error(1,"Invalid input BODYMASS");
            if ( TAgetfloat(tok[1],&x) ) mem->dr->bodymass=x;
            else ta_error(1,"Invalid input BODYMASS");
            break;
        case _NORMALIZE:
            if( ntokens != 2 ) ta_error(1,"Invalid input NORMALIZE");
            if(!strcasecmp(tok[1],"YES"))
                mem->dr->normalize = 1;
            else if(!strcasecmp(tok[1],"NO"))
                mem->dr->normalize = 0;
            else ta_error(1,"Invalid input NORMALIZE");
            break;
        case _LATENCYTIME:
            if( ntokens != 2 ) ta_error(1,"Invalid input LATENCYTIME");
            if ( TAgetfloat(tok[1],&x) ) mem->dr->nL=ROUND(x/mem->net->info->stepSize);
            else ta_error(1,"Invalid input LATENCYTIME");
            break;
        case _FATALITYTIME:
            if( ntokens != 2 ) ta_error(1,"Invalid input FATALITYTIME");
            if ( TAgetfloat(tok[1],&x) ) mem->dr->nF=ROUND(x/mem->net->info->stepSize);
            else ta_error(1,"Invalid input FATALITYTIME");
            break;
        case _INGESTIONRATE:
            if( ntokens != 2 ) ta_error(1,"Invalid input INGESTIONRATE");
            if ( TAgetfloat(tok[1],&x) ) {
	      mem->ingestionData->phi=(float)(x/24.);
	      mem->ingestionData->meanVolume=x;
	    }
            else ta_error(1,"Invalid input INGESTIONRATE");
            break;
        case _FATALITYRATE:
            if( ntokens != 2 ) ta_error(1,"Invalid input FATALITYRATE");
            if ( TAgetfloat(tok[1],&x) ) mem->dr->frate=x;
            else ta_error(1,"Invalid input FATALITYRATE");
            break;
        case _DOSETYPE:
            if( ntokens != 2 ) ta_error(1,"Invalid input DOSETYPE");
            if(!strcasecmp(tok[1],"TOTAL"))
                mem->dr->dtype = totalmass;
            else ta_error(1,"Invalid input DOSETYPE");
            break;
        case _INGESTIONTYPE:
            if( ntokens < 2 || ntokens > 4) ta_error(1,"Invalid input INGESTIONTYPE");
            if(!strcasecmp(tok[1],"DEMAND")) {
				if(ntokens != 2) ta_error(1,"Invalid input INGESTIONTYPE");
                mem->ingestionData->timingMode = IMDemand;
                mem->ingestionData->volumeMode = VMDemand;
			} else {
				if(ntokens == 2) ta_error(1,"Invalid input INGESTIONTYPE");
				if(!strcasecmp(tok[1],"ATUS")) {
					mem->ingestionData->timingMode=IMATUS;
				} else if(!strcasecmp(tok[1],"FIXED5")) {
					mem->ingestionData->timingMode=IMFixed5;
				} else ta_error(1,"Invalid input INGESTIONTYPE");

				if(!strcasecmp(tok[2],"RANDOM")) {
					if(ntokens != 3) ta_error(1,"Invalid input INGESTIONTYPE");
					mem->ingestionData->volumeMode=VMRandom;
				} else if(!strcasecmp(tok[2],"MEAN")) {
					mem->ingestionData->volumeMode=VMMean;
#if 0
//					if(ntokens == 4) {
//			            if ( TAgetfloat(tok[3],&x) ) mem->ingestionData->meanVolume=x;
//						else ta_error(1,"Invalid input INGESTIONRATE");
//					} else {
//						mem->ingestionData->meanVolume=1000.0;
//					}
#endif
				} else { ta_error(1,"Invalid input INGESTIONTYPE"); }

			}
            break;
        case _POPULATION:
            if( ntokens == 3 && strcasecmp(tok[1],"DEMAND")==0)
               mem->popData->population = DemandBased;
            if ( TAgetfloat(tok[2],&x) ) 
               mem->popData->pcu=x;
            else if(ntokens == 3 && strcasecmp(tok[1],"FILE")==0) {
               loadPopulationFile_impl(tok[2],mem->popData,taiIFD->srcFileName);
               mem->popData->population = FileBased;
            } else 
               ta_error(1,"Invalid input POPULATION");
            break;
        case _DOSE_THRESHOLDS: {
				int j;
				PDoseOverThresh pdot=NULL;
				if(mem->dot!= NULL) ta_error(1,"DOSE_THRESHOLDS has already been specified");
				if( ntokens == 1) ta_error(1,"At least one DOSE_THRESHOLD is required if DOSE_THRESHOLDS is specified");
				pdot = (PDoseOverThresh)calloc(1,sizeof(DoseOverThresh));
				pdot->numThresh=ntokens-1;
				pdot->thresholds=(float*)calloc(pdot->numThresh,sizeof(float));
				pdot->threshIDs=(char**)calloc(pdot->numThresh,sizeof(char*));
				pdot->totOver = (int *)calloc(pdot->numThresh,sizeof(int));
				pdot->nodesOver = (int *)calloc(pdot->numThresh,sizeof(int));
				pdot->numOver = (int **)calloc(pdot->numThresh,sizeof(int*));
				pdot->numOverByTime = (int **)calloc(pdot->numThresh,sizeof(int*));
				for(j=0;j<pdot->numThresh;j++) {
					float f;
					char *tmp=tok[j+1];
					if(TAgetfloat(tmp,&f)) {
						pdot->thresholds[j]=f;
						pdot->threshIDs[j]=(char *)calloc(strlen(tmp)+1,sizeof(char));
						strcpy(pdot->threshIDs[j],tmp);
					} else {
						ta_error(1,"Invalid input value in DOSE_THRESHOLD");
					}
					pdot->numOver[j] = (int *)calloc(mem->net->info->numNodes,sizeof(int));
					pdot->numOverByTime[j] = (int *)calloc(mem->net->info->numSteps,sizeof(int));
				}
				mem->dot=pdot;
			}
			break;

        case _DOSE_BINDATA: {
          int j;
		  PDoseBinData pdb=NULL;
          int numValues=ntokens-2;
          int hasZero=0;
          int type=0;
          float *values=(float*)calloc(numValues+1,sizeof(float));
          if(mem->doseBins!= NULL) ta_error(1,"DOSE_BINDATA has already been specified");
          if( ntokens == 1) ta_error(1,"No data specified for DOSE_BINDATA");
          if(strcasecmp(tok[1],"DOSE")) type=1;
          else if(strcasecmp(tok[1],"RESPONOSE")) type=2;
          else ta_error(1,"Invalid data type specified for DOSE_BINDATA");
          
          for(j=0;j<numValues;j++) {
            float f;
            if(TAgetfloat(tok[j+2],&f)) {
              if(f < 0 || (type==2&&f > 1.0)) {
            	if(type==2)
            		ta_error(1,"Invalid input value in DOSE_BINDATA: out of range [0,1]");
            	else
            		ta_error(1,"Invalid input value in DOSE_BINDATA: value less than zero");
              }
              if(f==0) {
                hasZero=1;
              }
              values[j]=f;
            } else {
              ta_error(1,"Invalid input value in DOSE_BINDATA");
            }
          }
          if(!hasZero) {
            values[numValues]=0;
            numValues++;
          }
          qsort(values,numValues,sizeof(float),x_sortFloatAsc);
          
          pdb = (PDoseBinData)calloc(1,sizeof(DoseBinData));
          pdb->numBins=numValues;
          if(type==2){
        	  pdb->responseBins=values;
              pdb->doseBins=(float*)calloc(numValues,sizeof(float));
          } else {
              pdb->doseBins=values;
          }
          pdb->data=(int*)calloc(numValues,sizeof(int));
          mem->doseBins=pdb;
          break;
        }
        case _TSONAME:
//            if( ntokens != 2 ) ta_error(1,"Invalid input TSONAME");
//            strcpy(io->tsoname,tok[1]);
            break;
        case _TAONAME:
            if( ntokens != 2 ) ta_error(1,"Invalid input TAONAME");
            strcpy(mem->io->taoname,tok[1]);
            break;
        case _SAPNAME:
			ta_error(0,"SAPNAME is no longer supported");
            break;
        case _SAPVARS:
			ta_error(0,"SAPVARS are no longer supported");
            break;
        case _THRESHOLD:
            if( ntokens != 2 ) ta_error(1,"Invalid input THRESHOLD");
            if ( TAgetfloat(tok[1],&x) ) mem->concentrationThreshold=x;
            else ta_error(1,"Invalid input THRESHOLD");
            break;
        default:
			if(strcmp(tok[0],"USAGERATE")==0) {
				ta_error(1,"USAGERATE has been deprecated - use POPULATION DEMAND UsageRateValue");
			} else {
				ta_error(1,"Invalid TAI keyword");
			}
        }
    }   /* End of while */
 
    if(mem->ingestionData->volumeMode==VMMean && mem->ingestionData->meanVolume<=0) {
      ta_error(1,"Invalid input: VolumeMode is set to MEAN, and there is an invalid mean volume specified (INGESTIONRATE) of %f\n",mem->ingestionData->meanVolume);
      }
	if(mem->dr->speciesIndex==-1) {
		ta_error(0,"SPECIES was not specified, assuming index 0");
		mem->dr->speciesIndex=0;
		mem->dr->speciesName=(char *)calloc(8,sizeof(char));
		strcpy(mem->dr->speciesName,"Unknown");
	}
    /* Allocate memory depending on assessment parameters */
    mem->dr->maxsteps = mem->net->info->numSteps + mem->dr->nL + mem->dr->nF;       /* Max number   of time steps for disease response */
    initU(mem->u,mem->dr->maxsteps);	/* Basic dose-respone   state variables */
    initXA(mem->xa,mem->dr->maxsteps);                 /*  Spatial averages */
    initTA(mem->ta,mem->net->info->numNodes);                  /* Temporal averages */
    if(mem->doseBins && mem->doseBins->responseBins) {
      PDoseBinData pdb=mem->doseBins;
      for(i=0;i<pdb->numBins;i++) {
        pdb->doseBins[i]=ResponseDose(pdb->responseBins[i],mem->dr);
      }
    }
}


void loadPopulationFile(char *fn,PopulationDataPtr pd)
{
   loadPopulationFile_impl(fn, pd, NULL);
}

static int x_sortFloatAsc(const void *a, const void *b)
{
	float f=*((float *)a) - *((float *)b);
	return (f<0?-1:(f>0?1:0));
}
