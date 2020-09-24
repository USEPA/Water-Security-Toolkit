#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


#include "epanet2.h"
#include "btxtypes.h"
#include "btxfuns.h"
#include "epanetbtx.h"
#include "mempool.h"

// This is a complete HACK to get this to compile under windows, and
// should be removed to restore DLL builds. [JDS; 15 Dec 14]
#undef DLLEXPORT
#define DLLEXPORT


BTXproject BTX;

int DLLEXPORT  BTXopen(char *hydfname)
/*
-------------------------------------------------------------------------
   Input:   hydfname = name of binary hydraulics results file.              
   Output:  none 
   Returns: error code (0 for no error)                             
   Purpose: associates a hydraulic solution file
-------------------------------------------------------------------------
*/
{

	int flowflag, magic, n, version;	
	int errcode = 0;
	int simdur;
	long time = 0;
	BTX.HydrauInforFlag = 0;	
    BTX.HydFile = fopen(hydfname, "rb");	
    if (!BTX.HydFile) return ERR_OPEN_HYD_FILE;


	ENgetflowunits(&flowflag);
	
	if (flowflag >= EN_LPS)     //International unit
		BTX.LUcf = MperF;
	else 
		BTX.LUcf = 1.0;


	errcode = ENgetcount(EN_NODECOUNT, &(BTX.Network.nnodes));
	if (errcode) return (errcode);	
	errcode = ENgetcount(EN_LINKCOUNT, &(BTX.Network.nlinks));  
	if (errcode) return (errcode);	
	errcode = ENgetcount(EN_TANKCOUNT, &(BTX.Network.ntanks));  
	if (errcode) return (errcode);	
	errcode = ENgettimeparam(EN_DURATION, &(BTX.Network.dur));
	if (errcode) return (errcode);

// check that file is really a hydraulics file for current project

    fread(&magic, sizeof(int), 1, BTX.HydFile);
    fread(&version, sizeof(int), 1, BTX.HydFile);
    fread(&n, sizeof(int), 1, BTX.HydFile);
    if ( n != BTX.Network.nnodes ) return ERR_READ_HYD_FILE;
    fread(&n, sizeof(int), 1, BTX.HydFile);
    if ( n != BTX.Network.nlinks ) return ERR_READ_HYD_FILE;
     fread(&n, sizeof(int), 1, BTX.HydFile);
    if ( n != BTX.Network.ntanks ) return ERR_READ_HYD_FILE;   
	fseek(BTX.HydFile, 2*sizeof(int), SEEK_CUR);
    fread(&simdur, sizeof(int), 1, BTX.HydFile);
	if (simdur != BTX.Network.dur) return ERR_READ_HYD_FILE;

	BTX.HydOffset = (int)ftell(BTX.HydFile);

	BTX.Network.nsteps = 0;
	do
	{
		BTX.Network.nsteps++;
		if( fread(&time, sizeof(int), 1, BTX.HydFile) < 1 ) return ERR_READ_HYD_FILE;
		fseek(BTX.HydFile, (2*BTX.Network.nnodes+3*BTX.Network.nlinks)*sizeof(float)+sizeof(int), SEEK_CUR);	
	}while(time < BTX.Network.dur);

	return (0);
}


int DLLEXPORT  BTXinit(float impactstep, float erasecri)
/*
--------------------------------------------------------------------------------------------------------------
   Input:   impactstep = time step (in hour) for impact coefficient aggregation at inputs  
			easecri    = threshold for particle impact coefficient to delete backtracking particles with 
						 small impact on output water quality
   Output:  none 
   Returns: error code (0 for no error)                             
   Purpose: alllocates memory for data structure and read hydraulic data 
---------------------------------------------------------------------------------------------------------------
*/
{
	int nnodes, nlinks, ntanks, nsteps, errcode, nodetype, i;
    float nodevalue;
	errcode = 0;

	nlinks = BTX.Network.nlinks;
	nnodes = BTX.Network.nnodes;
	ntanks = BTX.Network.ntanks;
	nsteps = BTX.Network.nsteps;
	
	BTX.Erasecri = erasecri;
	BTX.Network.nimpactsteps =  (int)ceil((BTX.Network.dur/(3600*impactstep)));
	BTX.Network.impactstep = impactstep;

	BTX.Tank = (struct Stank *)calloc((ntanks+1),sizeof(struct Snode));
	if (BTX.Tank == NULL) return ERR_MEMORY;
	ntanks = 0;
	for(i=1; i<=nnodes; i++)
	{
		errcode = ENgetnodetype(i, &nodetype);
		if (errcode) return (errcode);
		if(nodetype == EN_TANK)
		{
			++ntanks;
			BTX.Tank[ntanks-1].nodeindex = i;
			errcode = ENgetnodevalue(i, EN_MIXMODEL, &nodevalue);
			if (errcode) return errcode;
			BTX.Tank[ntanks-1].mixtype = (int)nodevalue;
			errcode = ENgetnodevalue(i, EN_MIXZONEVOL, &nodevalue);
			if (errcode) return errcode;			
			BTX.Tank[ntanks-1].mixvolume = nodevalue/(BTX.LUcf*BTX.LUcf*BTX.LUcf);
			errcode = ENgetnodevalue(i, EN_INITVOLUME, &nodevalue);
			if (errcode) return errcode;
			BTX.Tank[ntanks-1].initvolume = nodevalue/(BTX.LUcf*BTX.LUcf*BTX.LUcf);
			errcode = ENgetnodevalue(i, EN_TANK_KBULK, &nodevalue);
			if (errcode) return errcode;
			BTX.Tank[ntanks-1].decay_coeff = nodevalue;
		}
	}		
	BTX.Network.ntanks = ntanks;
	
	BTX.Node = (struct Snode *)calloc((nnodes+1),sizeof(struct Snode));
	if (BTX.Node == NULL) return ERR_MEMORY;
	BTX.TankImpactCoeff = (float *)calloc((ntanks+1),sizeof(float));
	if (BTX.TankImpactCoeff == NULL) return ERR_MEMORY;
	BTX.TankMainImpactCoeff = (float *)calloc((ntanks+1),sizeof(float));
	if (BTX.TankMainImpactCoeff == NULL) return ERR_MEMORY;
	BTX.TimeofTankImpact = (float *)calloc((ntanks+1),sizeof(float));
	if (BTX.TimeofTankImpact == NULL) return ERR_MEMORY;

	BTX.SaveTime = (long *)calloc(nsteps,sizeof(long));
	if (BTX.SaveTime == NULL) return ERR_MEMORY;
	BTX.SaveStep = (long *)calloc(nsteps,sizeof(long));
	if (BTX.SaveStep == NULL) return ERR_MEMORY;

	BTX.TankVolume = (float **)calloc(ntanks,sizeof(float *));
	if (BTX.TankVolume == NULL) return ERR_MEMORY;
	for(i=0; i<ntanks; i++)
	{
		BTX.TankVolume[i] = (float *)calloc(nsteps,sizeof(float));
		if (BTX.TankVolume[i] == NULL)
			return ERR_MEMORY;
	}

	BTX.LinkFlow = (float **)calloc(nsteps,sizeof(float *));
	if (BTX.LinkFlow == NULL) return ERR_MEMORY;
	for(i = 0; i <= nsteps-1; i++)
	{
		BTX.LinkFlow[i] = (float *)calloc(nlinks,sizeof(float));
		if (BTX.LinkFlow[i] == NULL) return ERR_MEMORY;
	}

	BTX.PathFlag = (int *)calloc(nlinks,sizeof(int));
	if (BTX.PathFlag == NULL) return ERR_MEMORY;
	for ( i = 0; i < nlinks; i++)
		BTX.PathFlag[i] = 0;

	if (AllocInit() == NULL) return ERR_MEMORY;
    BTX.FreeParticle = NULL;
	AllocReset();

	for ( i = 1; i < BTX.Network.nnodes+1; i++)
	{
		BTX.Node[i].inputflag = 0;
		BTX.Node[i].impact = NULL;
		BTX.Node[i].sourcelength = 0;
		BTX.Node[i].sourcestrength = NULL;
	}

	errcode = getADJ();	//ADJ contains the node neighborhood information 
	if (errcode) return (errcode);

	BTX.FirstPath = NULL;
	BTX.LastPath = NULL;
	errcode = readhydraulics();

	return (errcode);
}


int DLLEXPORT  BTXsetinput(char * nodeid, int type)
/*
---------------------------------------------------------------
   Input:   nodeid = node id
            type   = source type (EN_CONCEN, EN_SETPOINT, EN_MASS or EN_FLOWPACED)
   Output:  none 
   Returns: error code                              
   Purpose: set water quality input locations
---------------------------------------------------------------
*/
{
	int nindex, errcode;
	errcode = ENgetnodeindex(nodeid, &nindex);
	if(errcode)  return(ERR_INVALID_NODE);
	if (type != EN_SETPOINT && type != EN_FLOWPACED && type != EN_MASS && type != EN_CONCEN)
		return (ERR_INVALID_SOURCETYPE);
	if (BTX.Node[nindex].inputflag != 1)
	{
		BTX.Node[nindex].inputflag = 1;
		BTX.Node[nindex].impact = (float *)calloc((BTX.Network.nimpactsteps),sizeof(float)); //index starts from 0
	}
	BTX.Node[nindex].sourcetype = type;

	return (0);
}


int DLLEXPORT BTXsetinputstrength(char * nodeid, float *strength, int length, float patternstep)
/*
-------------------------------------------------------------------------------------------------
   Input:   nodeid   = node id
			strength = source strength vector (strength[0], strength[1], ..., strength[length-1])
			length   = length of source strength vector
			patterstep = time step (in hour) of source strength 
   Output:  none 
   Returns: error code                              
   Purpose: set water quality source strength
-------------------------------------------------------------------------------------------------
*/
{
	int nindex, errcode, i;
	errcode = ENgetnodeindex(nodeid, &nindex);
	if (errcode) return (ERR_INVALID_NODE);
	if (BTX.Node[nindex].inputflag != 1) return (ERR_INVALID_INPUT);
	
	if (BTX.Node[nindex].sourcestrength != NULL)
	{
		free(BTX.Node[nindex].sourcestrength);
	}
	BTX.Node[nindex].sourcestrength = (float *)calloc(length,sizeof(float));
	if (BTX.Node[nindex].sourcestrength == NULL) return (ERR_MEMORY);
	for (i = 0; i < length; i++)
		BTX.Node[nindex].sourcestrength[i] = strength[i];
	BTX.Node[nindex].sourcelength = length;
	BTX.Node[nindex].patternstep = patternstep;
	return (0);
}

int DLLEXPORT  BTXsim(char * outputid, float outtime)
/*
--------------------------------------------------------------------------------------------------------------
   Input:   outputid   = node id   
			easecri    = water quality sampling time (in hour)
   Output:  none 
   Returns: error code (0 for no error)                             
   Purpose: backtracking water quality modeling at outputid and outtime 
---------------------------------------------------------------------------------------------------------------
*/
{
	int nlinks = BTX.Network.nlinks, ntanks = BTX.Network.ntanks, errcode = 0;
	int i, outnodeindex;

	if ((outtime*3600.0 > BTX.Network.dur)||outtime < 0.0) return (ERR_INVALID_TIME);
	if (BTX.HydrauInforFlag == 0) return (104);

	if (errcode) return (errcode);
	errcode = ENgetnodeindex(outputid, &outnodeindex);		
	if (errcode) return (ERR_INVALID_NODE);
	for (i = 1; i < BTX.Network.nnodes+1; i++)
	{
		if (BTX.Node[i].inputflag == 1)
			memset(BTX.Node[i].impact, 0, (BTX.Network.nimpactsteps)*sizeof(float));
	}

	if (outtime == 0.0) return (0);

	AllocReset();
	BTX.FreeParticle = NULL;
	errcode = prepareBT(outnodeindex, outtime); /*preparation work before particle backtrack*/
	if (errcode) return (errcode);

	errcode = backtrack();
	if (errcode == ERR_BACKTRACK_TIME)  /*move active particles to complete list*/
	{
		clearactivelist();	
	}
		
	return (errcode);
}


int DLLEXPORT BTXout(float * result)
/*
--------------------------------------------------------------------------------------------------------------
   Input:   none
   Output:  result = water quality result  
   Returns: 0                             
   Purpose: water quality modeling at outputid and outtime (this function is called after BTXsim) 
---------------------------------------------------------------------------------------------------------------
*/
{
	int position, n=0;
	float abstime;
	int nindex, i;

	*result =  0.0; 
	for ( nindex = 1; nindex <= BTX.Network.nnodes; nindex++)
	{
		if ((BTX.Node[nindex].inputflag == 1) && (BTX.Node[nindex].sourcelength > 0))
		{
			for ( i = 0; i < BTX.Network.nimpactsteps; i++)
			{
				if (BTX.Node[nindex].impact[i] > 0.0)
				{
					abstime = i*BTX.Network.impactstep;
					abstime = abstime - ((int)(abstime/(BTX.Node[nindex].sourcelength*BTX.Node[nindex].patternstep)))*BTX.Node[nindex].sourcelength*BTX.Node[nindex].patternstep;
					position = (int)(abstime/BTX.Node[nindex].patternstep);
					* result += BTX.Node[nindex].impact[i]*BTX.Node[nindex].sourcestrength[position];
				}	
			}
		}
	}
	return (0);
}

int DLLEXPORT BTXgetimpact(char * nodeid, float * impact, int size)
/*
-------------------------------------------------------------------------------------------------
   Input:   nodeid = node id
			size   = length of impact vector
   Output:  impact = impact coefficient vector (impact[0], impact[1], ..., impact[size-1])
   Returns: error code                              
   Purpose: get impact coefficients of input node on output water quality 
--------------------------------------------------------------------------------------------------
*/
{
	int i, errcode = 0, nindex, actsize;

	errcode = ENgetnodeindex(nodeid, &nindex);
	if (errcode) return ERR_INVALID_NODE;

	if (BTX.Node[nindex].inputflag != 1) return (ERR_INVALID_INPUT);
	if (size > BTX.Network.nimpactsteps) 
		actsize = BTX.Network.nimpactsteps;
	else 
		actsize = size;

	for ( i = 0; i < actsize; i++)
		impact[i] = BTX.Node[nindex].impact[i];
	for ( i = actsize; i < size; i++)
		impact[i] = 0.0;

	return (errcode);
}


int DLLEXPORT BTXgetpathnumber(int * npath)
/*
-------------------------------------------------------------------------------------------------
   Input:   none
   Output:  npath = number of flow paths
   Returns: 0                              
   Purpose: get number of flow paths 
--------------------------------------------------------------------------------------------------
*/
{

	*npath = BTX.PathNumber;
	return (0);
}

int DLLEXPORT BTXgetpathinfor(int pathindex, int * endnodeindex, float * timedelay, float * impactcoeff)
/*
-------------------------------------------------------------------------------------------------
   Input:   pathindex = flow path index
   Output:  endnodeindex = index of node where flow path ends
			timedelay    = time delay of the flow path in hours
			impactcoeff  = water quality impact coefficient of the flow path
   Returns: error code                              
   Purpose: get flow path information 
--------------------------------------------------------------------------------------------------
*/
{
	int i;
	struct Spath * path;
	if ( pathindex <= 0 || pathindex > BTX.PathNumber) return (ERR_INVALID_PATH);
	path = BTX.FirstPath;
	for ( i = 1; i < pathindex; i++)
		path = path->next;
	
	*endnodeindex = path->endnodeindex;
	*timedelay = path->timedelay;
	*impactcoeff = path->impactcoefficient;

	return (0);
}

int DLLEXPORT BTXpipeinpath(char * linkid, int *flag)
/*
-------------------------------------------------------------------------------------------------
   Input:   linkindex = link index
   Output:  flag = 1 if the link is in flow paths, otherwise 0
   Returns: error code                              
   Purpose: check if a link is in the flow path
--------------------------------------------------------------------------------------------------
*/
{
	int linkindex, errcode = 0;
	errcode = ENgetlinkindex(linkid, &linkindex);
	if (errcode) 
		return ERR_INVALID_LINK;
	*flag = BTX.PathFlag[linkindex-1];
	return 0;
}

int DLLEXPORT BTXclose()
/*
-------------------------------------------------------------------------------------------------
   Input:   none
   Output:  none
   Returns: 0                         
   Purpose: free memory used by BTX
--------------------------------------------------------------------------------------------------
*/
{
	int i, nnodes = BTX.Network.nnodes, nsteps = BTX.Network.nsteps, ntanks = BTX.Network.ntanks;
	Padj temp1 = NULL, temp2 = NULL;
	
	for (i = 1; i < nnodes+1; i++)
	{
		if(BTX.Node[i].impact != NULL)
			free(BTX.Node[i].impact);
		if(BTX.Node[i].sourcestrength != NULL)
			free(BTX.Node[i].sourcestrength);
	}
	AllocFreePool();

	if ( BTX.SaveTime != NULL)
	{
		free(BTX.SaveTime);
		BTX.SaveTime = NULL;
	}
	if (BTX.SaveStep != NULL)
	{
		free(BTX.SaveStep);
		BTX.SaveStep = NULL;
	}
	if (BTX.ADJ != NULL)
	{
		for(i = 1;i <= nnodes; i++)
		{		
			temp1 = BTX.ADJ[i-1];
			while(temp1 != NULL)
			{
				temp2 = temp1->next;
				free(temp1);
				temp1=temp2;
			}
		}
		free(BTX.ADJ);
		BTX.ADJ = NULL;
	}
	if (BTX.LinkFlow != NULL)
	{
		for(i = 1;i <= nsteps; i++)
			free(BTX.LinkFlow[i-1]);
		free(BTX.LinkFlow);
		BTX.LinkFlow = NULL;
	}
	if (BTX.Tank != NULL)
	{
		free(BTX.Tank);
		BTX.Tank = NULL;
	}
	if (BTX.Node != NULL)
	{
		free(BTX.Node);
		BTX.Node = NULL;
	}
	if (BTX.TankVolume != NULL)	
	{
		for(i = 1; i <= ntanks; i++)
		free(BTX.TankVolume[i-1]);
		free(BTX.TankVolume);
		BTX.TankVolume = NULL;
	}
	if (BTX.TankImpactCoeff != NULL)
	{
		free(BTX.TankImpactCoeff);
		BTX.TankImpactCoeff = NULL;
	}
	if (BTX.TankMainImpactCoeff != NULL)
	{
		free(BTX.TankMainImpactCoeff);
		BTX.TankMainImpactCoeff = NULL;
	}
	if (BTX.TimeofTankImpact != NULL)
	{
		free(BTX.TimeofTankImpact);
		BTX.TimeofTankImpact = NULL;
	}
	if (BTX.PathFlag != NULL)
	{
		free(BTX.PathFlag);
		BTX.PathFlag = NULL;
	}

	freepath();
	return (0);
}


