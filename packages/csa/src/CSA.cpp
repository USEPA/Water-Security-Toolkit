// CSA.cpp : Defines the entry point for the DLL application.
//
/*Annamaria E. De Sanctis*/
/*orig. code 2006, August 2009- ver. 0.9, vers. 1 mod.September 2009*/

//#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc/malloc.h>
#include <string.h>
#include <math.h> 
extern "C" {
#include "epanet2.h"
#include "epanetbtx.h"
#include "csa.h"
}
#ifdef _MANAGED
#pragma managed(push, off)
#endif

/*
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    return TRUE;
}
*/

int  gframe=0, gVS, gnnodes;
int ** gupdatematrix;
double gimpepsilon;
float gsourcestep;
char **gsensorid;
float **gsensormeasurement;
int gnsensors;
float gsamplestep;
int *gsafe, *gunsafe;

//__declspec(dllexport) int CSAopen(double epsilon, float sourcestep, int historical_frame)
int CSAopen(double epsilon, float sourcestep, int historical_frame)
/*
Purpose:	Opens the CSA toolkit system. Allocates memory data for CSA modeling. 
			Allocates memory for contamination source status matrix S  for all network nodes and for the length of simulation
			(S is initialized to unknown status).
Input:		epsilon: threshold for the significant flow path (total impact coefficient  I_{ij}^{\tau}(t)>= epsilon)
			historical-frame: historical analysis time frame (number of hours)
			sourcestep: time step tau to integrate impact coefficients (in the unit of seconds)
output:		none
Returns:	error code (0 for no error)
*/
{	
	int errcode,i;
	long timevalue; // to get the simulation duration in seconds

	gimpepsilon= epsilon;//threshold for the tot. impact coefficient
	gsourcestep = sourcestep;
	gframe= int(historical_frame*3600/gsourcestep); //lenght of the historical analysis time frame
	errcode = ENgetcount(EN_NODECOUNT, &gnnodes);
	errcode = ENgettimeparam(EN_DURATION, &timevalue ); // get the length of the simulation duration
	gVS=(int)(timevalue/gsourcestep)+1;//gVS=int(timevalue/3600)+1;
	gsafe = (int *)calloc(gVS, sizeof(int));
	gunsafe = (int *)calloc(gVS, sizeof(int));
	for ( i = 0; i <= gVS-1; i++)
	{
		gsafe[i] = 0;
		gunsafe[i] = 0;
	}
	gupdatematrix = (int **)calloc(gnnodes+1, sizeof(int *));
	for ( i = 1; i <= gnnodes; i++)
	{
		gupdatematrix[i] = (int *)calloc(gVS, sizeof(int));
		memset(gupdatematrix[i], 0, gVS*sizeof(int));	//contamination source status matrix S is inizialized to unknown status
	}
	return(errcode);
}  	
		
//__declspec(dllexport) int CSAhydraulics(int sourcetype)
int CSAhydraulics(int sourcetype)
/*
Purpose:	Runs hydraulic simulation and opens the backtracking toolkit, reads the hydraulic results, and initializes data for backtracking modeling.
			Initializes every network node as possible source
Input:		sourcetype: water quality type(EN_MASS or EN_FLOWPACED as defined in EPANET2)	
output:		none
Returns:	error code (0 for no error)
*/
{ 
	int errcode, m;
	char ID[50];
	float thresh= 0.000001f; //thresh=0.000001 threshold for particle impact coefficient to delete backtracking particles with small impact on output water quality
    float impactstep;
	
	errcode = ENsolveH();
	errcode = ENsavehydfile("hydfile");
	errcode = BTXopen("hydfile");
	impactstep = gsourcestep/3600;
	errcode = BTXinit(impactstep, thresh);//1.0 hour for time discrete step, 0.000001 per cent as particle erase criteria
	//Every node is a possible source
	if (sourcetype!=EN_MASS && sourcetype!= EN_FLOWPACED)
		printf("error source type!\n");
	for(m=1; m<=gnnodes; m++)
	{
		errcode = ENgetnodeid(m,ID);
		errcode = BTXsetinput(ID,sourcetype);
	}	
	return(errcode);
}
	
//__declspec(dllexport) int CSAaddsensor(int nsensors, char *sensorfile)//int CSAaddsensor(int index)
int CSAaddsensor(int nsensors, char *sensorfile)//int CSAaddsensor(int index)
/*
Purpose:	Adds all the sensor locations selected among the network nodes.
Input:		nsensors: total number of sensor locations;
			sensorfile: text file with the sensors IDs
Returns: 	error code (0 for no error)
*/
{ 
	int i, errcode;
	FILE *sensoridfile;
	//char **sensorid;
	
	gnsensors = nsensors;
	gsensorid = (char **)calloc(gnsensors+1, sizeof(char *));
	for( i = 1; i <= gnsensors; i++)
		gsensorid[i] = (char *)calloc(20, sizeof(char));
	//<<read sensor IDs>>
	//errcode  = fopen_s(&sensoridfile, sensorfile, "r");
	sensoridfile  = fopen(sensorfile, "r");
	if(!sensoridfile) return 700;

	for ( i =1; i <= gnsensors; i++)
	{
		fscanf(sensoridfile, "%s", gsensorid[i]);
		printf("%s\n", gsensorid[i]);
	}
	fclose(sensoridfile);
	return(errcode);
}

//__declspec(dllexport) int CSAsetbinary(float samplestep, char * measurementfile)//int CSAsetbinary(int index, char Mstatus, float samplestep, int period)
int CSAsetbinary(float samplestep, char * measurementfile)//int CSAsetbinary(int index, char Mstatus, float samplestep, int period)
/*
Purpose:	Sets new monitoring station status (positive/negative) and associated sampling times. This function is used to collect all samples to be 
			analyzed during the current analysis time interval of duration sourcestep >= samplestep
Input:		samplestep: sampling time interval in unit of seconds;
			measurementfile: text file containing the sensor measurements from all the sensor locations set in CSAaddsensor
Returns:	error code (0 for no error)
Notes:		This is called before CSAupdate.
*/
{	int i,j, nsamplestep, errcode;
	float measure;
	FILE * sensoridfile;
	
	gsamplestep = samplestep;
	nsamplestep=(gVS-1)*(int)floor(gsourcestep/gsamplestep);//nsamplestep = gVS-1;//
	//<<read sensor measurements>>
	gsensormeasurement = (float **)calloc(nsamplestep+1, sizeof(float *));
	for ( i = 1; i <= nsamplestep; i++)
	gsensormeasurement[i] = (float *)calloc(gnsensors+1, sizeof(float));
	//errcode  = fopen_s(&sensoridfile, measurementfile, "r");
	sensoridfile = fopen(measurementfile, "r");
	if(!sensoridfile) return 700;
	for ( i =1; i <= nsamplestep; i++)
	{
		for (j=1;j<=gnsensors;j++)
		{	
		fscanf(sensoridfile, "%f", &gsensormeasurement[i][j]);
		measure = gsensormeasurement[i][j];
		if (measure< 0)// trunk to zero if negative
		{measure=0;
		}
		gsensormeasurement[i][j] = measure;
		printf("%f\t", gsensormeasurement[i][j]);	
		}
		printf("\n");
	}
	fclose(sensoridfile);
	return(errcode);
}

//__declspec(dllexport) int CSAupdate()//int CSAupdate(int i)
int CSAupdate()//int CSAupdate(int i)
/*
Purpose:	Runs the particle backtracking simulation from each sensor location at a particular output time (based on the sampling time). 
			Applies the update CSA rule for all the network nodes and for an historical analysis time frame equal to historical-frame using the 
          information from all the sensors at the current analysis time. 
			Finally at each analysis time computes the Contamination Status Matrix
Input:		none
Output:		Returns a text file Smatrix.txt with all the Contamination Source matrices S computed during the simulation duration;
			each matrix corresponds to a particular analysis time
Returns:	Returns an error code if there are errors, otherwise returns 0.
Notes:		This function is called after CSAbinary, because it reads measurements from all the sensor locations.
*/
{
	int errcode=0, i,j,k,m, nsamplestep, nsourcestep;
	float simtime;
	float results, thresh=0.000;
	float *impact;
	//char ** gsensorid;
	char ID[50];
	//int ** updatematrix;
	int red=0, green=0, white=0, sum=0;
	////FILE * Pfile;
	////char *outputfile= "candidates.txt";
	FILE * UMfile;
	char *storefile= "Smatrix.txt";

	impact = (float *)calloc(gVS, sizeof(int));
	for ( k = 0; k <=(gVS-1) ; k++) // k <241//inizialize impact;
	{
		impact[k] = 0;
	}
	////errcode  = fopen_s(&Pfile, outputfile, "w+");
	//errcode  = fopen_s(&UMfile, storefile, "w+");
	UMfile = fopen(storefile, "w+");
	if(!UMfile) return 700;

	nsamplestep=(gVS-1)*(int)floor(gsourcestep/gsamplestep);//nsamplestep=gVS-1;
	for (i=1;i<=nsamplestep;i++)
	{
		simtime = (i*gsamplestep/3600)+0.000f;
		nsourcestep =(int)floor(i*gsamplestep/gsourcestep)+1; //floor(simtime/gsourcestep)+1; //nsourcestep = floor(simtime/sourcestep);
		for (j = 1; j<=gnsensors; j++)
		{
			results=gsensormeasurement[i][j];
			errcode = BTXsim(gsensorid[j], simtime);
			if (errcode>0) printf("%d\n", errcode);
			for(m=1; m<=gnnodes; m++)
			{
				errcode = ENgetnodeid(m,ID);
				errcode = BTXgetimpact(ID,impact,nsourcestep);// gets impact vector for node m at time simtime
				for (k=((nsourcestep-gframe>=0)?nsourcestep-gframe:0); k < nsourcestep; k++)//for (k=0; k < nsourcestep; k++)//
				{
					if(impact[k]>gimpepsilon && gupdatematrix[m][k] != -1)// -1=Green, gimpepsilon=1.0e-10
					{
						if (results > thresh)
						{
							gupdatematrix[m][k]=1;
						}
						else
							gupdatematrix[m][k] = -1;
						}
				}//k
			}//m
		}//j
		red=0;
		green=0;
		white=0;
		sum=0;
		fprintf(UMfile,"%.2f\n",simtime);
		for(m = 1; m<=gnnodes; m++)
		{
			for(k=((nsourcestep-gframe>=0)?nsourcestep-gframe:0); k < nsourcestep; k++)//for(k=0; k < nsourcestep; k++)//
			{
				fprintf(UMfile,"%d\t", gupdatematrix[m][k]);
				if(gupdatematrix[m][k]==1)
				{red++;}
				else if (gupdatematrix[m][k]== -1)
				{green++;}
				else
				{white++;}
			}//k
			fprintf(UMfile,"\n");
			//free memory for data outside the backtracking window
			if((nsourcestep-gframe)>=0)
			{
				for (k=0; k < nsourcestep-gframe; k++)
				{
					gupdatematrix[m][k]=NULL;
				}
			}
		}//m
		gunsafe[i] = red;
		gsafe[i] = green;
		sum=red+green+white;
		////fprintf(Pfile, "%6.2f\t%d\t%d\t%d\t%d\n",simtime, red, green, white, sum); // CSA output
	}//i
	////fclose(Pfile);
	fclose(UMfile);
	return(errcode);
}


//__declspec(dllexport) int CSAcandidates()//int CSAcandidates(int * unsafe, int * safe)
int CSAcandidates()//int CSAcandidates(int * unsafe, int * safe)
{ 
	int errcode, i, nsamplestep;
	FILE * Pfile;
	float simtime;
	char *outputfile= "candidates.txt";

	//errcode  = fopen_s(&Pfile, outputfile, "w+");
	Pfile = fopen(outputfile, "w+");
	if(!Pfile) return 700;

	nsamplestep=(gVS-1)*(int)floor(gsourcestep/gsamplestep);//floor((gVS-1)/gsamplestep);//nsamplestep=gVS-1;
	//safe = (int *)calloc(gnnodes+1, sizeof(int *));
	for ( i = 0; i <= nsamplestep; i++)
	{
		simtime = (i*gsamplestep/3600)+0.000f;
		fprintf(Pfile, "%6.2f\t%d\t%d\n",simtime, gunsafe[i], gsafe[i]); // CSA output
	}
	fclose(Pfile);
	return(errcode);
}


//__declspec(dllexport) int CSAclose()
int CSAclose()
{
	int errcode;
	//free memory used by CSA;
	free(gsafe);
	free(gunsafe);
	free(gupdatematrix);
	free(gsensorid);
	free(gsensormeasurement);
	errcode = BTXclose();
	return(errcode);
}
//Annamaria E. De Sanctis - orig. code 2006, vers. 1 mod.September 2009


#ifdef _MANAGED
#pragma managed(pop)
#endif
