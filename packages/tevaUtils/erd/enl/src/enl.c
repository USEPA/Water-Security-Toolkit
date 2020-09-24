#include "enl.h"
#include "epanet2.h"

typedef struct PatternInfo {
	char *id;
	int numValues;
	float *mult;
} PatternInfo, *PatternInfoPtr;
typedef struct Patterns {
	int num;
	PatternInfoPtr *patterns;
} Patterns, *PatternsPtr;
static PatternInfoPtr getPattern(int pattID, PatternsPtr patterns);

// Caution; these functions are duplicated from erdinternals.c
static void initializeNetworkData(PNodeInfo *nodes, PLinkInfo *links, PNetInfo net);
//PHydData initHyd(PNetInfo network);
//PQualData initQual(int numNodes, int numLinks, int numSpecies, int numSteps, PSpeciesData *species);
static PNetInfo initializeNetInfo(int speciesCount);

static void getNodeXY(PNodeInfo nodes, PLinkInfo links, PNetInfo net, const char *epanetfname);
static void enlError(int exitCode,const char *errmsg, ...);
static int  getTokens(char *s, char **tok);
static int findMatch(char *line, char *keyword[]);
static int match(char *str, char *substr);
static int getFloat(char *s, float *y);
static int getInt(char *s, int *y);
static int getLong(char *s, long *y);
void expandGen(int nnodes, PNodeInfo nodes, int sourcetype, int speciesindex, float sourcestrength, long starttime, 
               long stoptime, int numsources, int is, char **nodetok, PSource sources, 
               FILE *simin);
               
static void MEMCHECK(void *x, char *msg)
{
	if(x == NULL)
		enlError(1, "Allocating memory: %s", msg);
}

LIBEXPORT(int) ENL_getHydResults(int time, long timeStep, PERD database)
{	
	// be careful with this one.  each hydraulic measurement is weighted so we can
	// accumulate an average value for the reporting time step.

	PNetInfo network = database->network;
	PHydData hyd = network->hydResults;
	int n, l;
	float nv, lv, w, linkStatus;
	
	w = (float)timeStep / (float)network->reportStep;
	for(n = 0; n < network->numNodes; n++) {
		ENCHECK(ENgetnodevalue(n + 1, EN_DEMAND, &nv));
		hyd->demand[time][n] += w * nv;
		ENCHECK(ENgetnodevalue(n+1, EN_PRESSURE, &nv));
		hyd->pressure[time][n] += w * nv;
	}
	
	for(l = 0; l < network->numLinks; l++) {
		ENCHECK(ENgetlinkvalue(l + 1, EN_FLOW, &lv));
		hyd->flow[time][l] += w * lv;
		ENCHECK(ENgetlinkvalue(l + 1, EN_VELOCITY, &lv));
		hyd->velocity[time][l] += w * lv;
	}
	
	// get link status values.  
	// this may be overwritten if the analysis timestep is shorter than the reporting timestep.
	// final value will be the latest value of link status (status at the polling interval boundary)
	for(l=0; l<network->numControlLinks; l++) {
		ENCHECK(ENgetlinkvalue(network->controlLinks[l] + 1, EN_STATUS, &linkStatus));
		hyd->linkStatus[time][l] = (int)linkStatus;
	}
		
	return FALSE;
}

// get quality calculations from msx (or epanet) and copy values into node data structure.
//
LIBEXPORT(int) ENL_getQualResults(int time, long timeStep, PERD database)
{
	PNetInfo network = database->network;
	PQualData qual = network->qualResults;
	PSpeciesData *species = database->network->species;

	float ***nC = qual->nodeC;
	float ***lC = qual->linkC;

	int n, s;
	float nv, w;
	double dnv;
	
	// calculate weight for accumulating the average concentration over reporting timestep.
	w = (float)timeStep / (float)network->reportStep;

	if(network->qualCode == MULTISPECIES) {
		for(s = 0; s < network->numSpecies; s++) {
			if(species[s]->index != -1) {
				if( species[s]->type == bulk ) {
					for(n = 0; n < network->numNodes; n++) {
						//fprintf(stdout,"s:%i\tn:%i\tt:%i\n",s,n,time);
						MSXCHECK(MSXgetqual(MSX_NODE, n + 1, species[s]->index+1, &dnv));
						nC[s][n][time] += w * (float)dnv;
					}
				} else {
					for(n = 0; n < network->numLinks; n++) {
						//fprintf(stdout,"s:%i\tn:%i\tt:%i\n",s,n,time);
						MSXCHECK(MSXgetqual(MSX_LINK, n + 1, species[s]->index+1, &dnv));
						lC[s][n][time] += w * (float)dnv;
					}
				}
			}
		}
	}
	else {
		for(n = 0; n < network->numNodes; n++) {
			ENCHECK(ENgetnodevalue(n + 1, EN_QUALITY, &nv));
			nC[0][n][time] += w * nv;
		}
	}

	return FALSE;
}

LIBEXPORT(int) ENL_getNetworkData(PERD database, const char *inputFile, const char *msxInputFile,char *msx_species)
{
	char units[16];
	PNetInfo net;
	int i, n, ns, len, type;
	PNodeInfo nodes = NULL;
	PLinkInfo links = NULL;
	PHydData hyd = NULL;
	PQualData qual = NULL;
	int numControls = 0;
	int controlType, controlLinkIndex, controlNodeIndex;
	float controlSetting, controlLevel;
	double aTol, rTol;	
	
	if(msxInputFile != NULL) {
		int cnt=0;
		MSXCHECK(MSXgetcount(MSX_SPECIES, &ns));
		net = database->network = initializeNetInfo(ns);

		// Get and store the species data: index, type, and ID
		for(i = 0; i < ns; i++) {
			MSXCHECK(MSXgetspecies(i+1, &type, units, &aTol, &rTol));
			if( type == MSX_BULK ) {
				net->species[i]->type = bulk;
			} else {
				net->species[i]->type = wall;
			}
			MSXCHECK(MSXgetIDlen(MSX_SPECIES, i+1, &len));
			MSXCHECK(MSXgetID(MSX_SPECIES, i+1, net->species[i]->id, len));
			net->species[i]->index = -1;
		}

		if(msx_species==NULL) {
			// if nothing was specified, then save all
			for(i = 0; i < ns; i++) {
				net->species[i]->index=i;
			}
		} else {
			int num=0;
			char *tok=strtok(msx_species,",");
			while(tok!=NULL) {
				int idx=ERD_getSpeciesIndex(database,tok);
				if(idx==-1) {
					enlError(1,"Species %s specified to save, but not found",tok);
				}
				net->species[idx]->index=idx;
				tok=strtok(NULL,",");
				num++;
			}
			if(num==0) {
				enlError(1,"No species specified to be saved.");
			}
		}
		net->qualCode = 4;
	} else {
		net = database->network = initializeNetInfo(1);
		strncpy(net->species[0]->id, "species", 7);
		net->species[0]->index = 0;
		net->species[0]->type = bulk;
		ENCHECK(ENgetqualtype(&net->qualCode, &i));
	}
	ENCHECK(ENgettimeparam(EN_DURATION, &net->simDuration));
	ENCHECK(ENgettimeparam(EN_REPORTSTART, &net->reportStart));
	ENCHECK(ENgettimeparam(EN_REPORTSTEP, &net->reportStep));
	ENCHECK(ENgettimeparam(EN_STARTTIME, &net->simStart));
	ENCHECK(ENgetcount(EN_NODECOUNT, &net->numNodes));
	ENCHECK(ENgetcount(EN_LINKCOUNT, &net->numLinks));
	ENCHECK(ENgetcount(EN_TANKCOUNT, &net->numTanks));
	net->numJunctions = net->numNodes - net->numTanks;
	net->numSteps = 1 + (net->simDuration - net->reportStart) / net->reportStep;
	net->stepSize = (float)net->reportStep / 3600.0f;
	net->fltMax = FLT_MAX;
	
	// initially allocate only controlLinks[0]
	MEMCHECK(net->controlLinks = (int*)calloc(1,sizeof(int)),"net->controlLinks in ENL_getNetworkData");
	
	// find number of controlled links, and get their epanet indices.
	ENCHECK(ENgetcount(EN_CONTROLCOUNT, &numControls));
	// for each control statement, find the link index
	for(i=1; i<=numControls; i++) {
		ENCHECK(ENgetcontrol(i, &controlType, &controlLinkIndex, &controlSetting, &controlNodeIndex, &controlLevel));
		// decrement the epanet index by 1 since erd index = base 0.
		controlLinkIndex--;
		// check if the current erd link is already listed as controlled
		if(ERD_getERDcontrolLinkIndex(database, controlLinkIndex) < 0) {
			// add the current controlled link to the array, and increment link count.
			net->controlLinks = (int*)realloc(net->controlLinks, (net->numControlLinks + 1)*sizeof(int));
			net->controlLinks[net->numControlLinks] = controlLinkIndex;
			net->numControlLinks++;
		}
	}
	
	// Allocate HydData
	net->hydResults = ERD_UTIL_initHyd(net,READ_ALL);

	// Allocate QualData
	net->qualResults = ERD_UTIL_initQual(net,READ_ALL);

	// Allocate NodeInfo and LinkInfo
	initializeNetworkData(&nodes, &links, net);
	for(n = 0; n < net->numNodes; n++) {
		MEMCHECK(nodes[n].nz = (int *)calloc(net->numSpecies, sizeof(int)), "nodes[i].nz in ENL_getNetworkData");
	}
	for(n = 0; n < net->numLinks; n++) {
		MEMCHECK(links[n].nz = (int *)calloc(net->numSpecies, sizeof(int)), "links[i].nz in ENL_getNetworkData");
	}
	database->nodes = nodes;
	database->links = links;

	for(i = 0; i < net->numNodes; i++) {
 		int type;
		ENCHECK(ENgetnodetype(i + 1, &type));
		if(type == EN_TANK || type == EN_RESERVOIR)
			nodes[i].type = tank;
		ENCHECK(ENgetnodeid(i + 1, nodes[i].id));
	}
	for(i = 0; i < net->numLinks; i++) {
		int from, to;
		float length,diameter;
		ENCHECK(ENgetlinknodes(i + 1, &from, &to));
		ENCHECK(ENgetlinkvalue(i + 1, EN_LENGTH, &length));
		ENCHECK(ENgetlinkvalue(i + 1, EN_DIAMETER, &diameter));
		ENCHECK(ENgetlinkid(i + 1, links[i].id));
		links[i].from = from;
		links[i].to = to;
		links[i].length = length;
		links[i].diameter = diameter;
	}
        
	getNodeXY(nodes, links, net, inputFile);
        	
	return FALSE;
}

float **getDemandProfiles()
{
	float **dp=NULL;
	
	return dp;
}

static void getNodeXY(PNodeInfo nodes, PLinkInfo links, PNetInfo net, const char *epanetfname)
{
    char  line[MAXLINE+1],     /* Line from input data file       */
          wline[MAXLINE+1];    /* Working copy of input line      */
    int   sect,                /* Data sections                   */
          ntokens = 0,         /* # tokens in input line          */
          nodeindex,           /* node index                      */
          linkindex,           /* link index                      */
          nv;
    FILE *EPANETIN;            /* File pointer to epanet input    */
    char *tok[MAXTOKS];        /* Array of token strings          */
    float x,y;                 /* coordinates                     */

    char *secttxt[]
     = {"[TITL", "[JUNC", "[RESE",
        "[TANK", "[PIPE", "[PUMP",
        "[VALV", "[CONT", "[RULE",
        "[DEMA", "[SOUR", "[EMIT",
        "[PATT", "[CURV", "[QUAL",
        "[STAT", "[ROUG", "[ENER",
        "[REAC", "[MIXI", "[REPO",
        "[TIME", "[OPTI", "[COOR",
        "[VERT", "[LABE", "[BACK",
        "[TAGS", "[END",  NULL};

    /* Open epanet input file */
    EPANETIN = fopen( epanetfname, "rt" );
    if (EPANETIN == NULL) enlError(1,"Opening Epanet input file");

    sect = -1;
    /* Read each line from input file. */
    while (fgets(line,MAXLINE,EPANETIN) != NULL)
    {

        /* Make copy of line and scan for tokens */
        strcpy(wline,line);
        ntokens = getTokens(wline,tok);

        /* Skip blank lines and comments */
        if (ntokens == 0) continue;
        if (*tok[0] == ';') continue;

        /* Check if max. length exceeded */
        if (strlen(line) >= MAXLINE) enlError(1,"Epanet input file error.");

        /* Check if at start of a new input section */
        if (*tok[0] == '[')
        {
            sect = findMatch(tok[0],secttxt);
            if (sect >= 0)
            {
                if (sect == _END) break;
                continue;
            }
            else enlError(1,"Epanet input file error");
        }

        /* Otherwise process input if in Coords. section */
        else if (sect == _COORDS)
        {
            if ( ntokens != 3 ) enlError(1,"Epanet input file error");
            ENCHECK( ENgetnodeindex(tok[0],&nodeindex) );
            if(nodeindex > net->numNodes) enlError(1,"Epanet input file error");
            if(getFloat(tok[1],&x)) {
                nodes[nodeindex-1].x = x;
            }
            else enlError(1,"Epanet input file error");
            if(getFloat(tok[2],&y)) {
                nodes[nodeindex-1].y = y;
            }
            else enlError(1,"Epanet input file error");
        }

        /* Otherwise process input if in Vertices section */
        else if (sect == _VERTICES)
        {
            if ( ntokens != 3 ) enlError(1,"Epanet input file error");
            ENCHECK( ENgetlinkindex(tok[0],&linkindex) );
            if ( linkindex > net->numLinks ) enlError(1,"Epanet input file error");
            nv=++links[linkindex-1].nv;
            MEMCHECK(links[linkindex-1].vx=(float *)realloc(links[linkindex-1].vx,nv*sizeof(float)),"links.vx in ENTU_GetNodeXY");
            MEMCHECK(links[linkindex-1].vy=(float *)realloc(links[linkindex-1].vy,nv*sizeof(float)),"links.vy in ENTU_GETNodeXY");
            if ( getFloat(tok[1],&x) ) {
                links[linkindex-1].vx[nv-1] = x;
            }
            else enlError(1,"Epanet input file error");
            if(getFloat(tok[2],&y)) {
                links[linkindex-1].vy[nv-1] = y;
            }
            else enlError(1,"Epanet input file error");
        }

    }
	fclose(EPANETIN);
}

static void enlError(int exitCode,const char *errmsg, ...)
{
	va_list ap;

	fprintf(stderr,"\n\n********************************************************************************\n\n");
	va_start(ap,errmsg);
	vfprintf(stderr,errmsg,ap);
	va_end(ap);
	fprintf(stderr,"\n\n********************************************************************************\n\n");

	exit(exitCode);
}

static void enlWarning(const char *errmsg, ...)
{
	va_list ap;

	fprintf(stderr,"\n\n********************************************************************************\n\n");
	va_start(ap,errmsg);
	vfprintf(stderr,errmsg,ap);
	va_end(ap);
	fprintf(stderr,"\n\n********************************************************************************\n\n");
}

static int getTokens(char *s, char **tok)
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

static int findMatch(char *line, char *keyword[])
{
    int i = 0;
    while (keyword[i] != NULL)
    {
        if (match(line,keyword[i])) return(i);
        i++;
    }
    return(-1);
}

static int match(char *str, char *substr)
{
    int i,j;

    /*** Updated 9/7/00 ***/
    /* Fail if substring is empty */
    if (!substr[0]) return(0);

    /* Skip leading blanks of str. */
    for (i=0; str[i]; i++) {
        if (str[i] != ' ') break;
    }

    /* Check if substr matches remainder of str. */
    for (i=i,j=0; substr[j]; i++,j++) {
        if (!str[i] || UCHAR(str[i]) != UCHAR(substr[j]))
            return(0);
    }
    return(1);
}

static int getFloat(char *s, float *y)
{
    char *endptr;
    *y = (float) strtod(s,&endptr);
    if (*endptr > 0) return(0);
    return(1);
}

static int getInt(char *s, int *y)
{
    char *endptr;
    *y = (int)strtol(s,&endptr,10);
    if (*endptr > 0) return(0);
    return(1);
}


static int getLong(char *s, long *y)
{
    char *endptr;
    *y = strtol(s,&endptr,10);
    if (*endptr > 0) return(0);
    return(1);
}

LIBEXPORT(int) epanetError(int errorCode)
{
	char errorMessage[MAXERRMSG];

	ENgeterror(errorCode, errorMessage, MAXERRMSG);
	if(errorCode > 100) {
		fprintf(stderr, "EPANET run-time error.\n");
		fprintf(stderr, "%s\n", errorMessage);
		exit(1);
	}
	else {
		return errorCode;
	}
}

LIBEXPORT(int) epanetmsxError(int errorCode)
{
	char errorMessage[MAXERRMSG];
	int msxFirst = 500;
        
	if(errorCode > 100) {
		if(errorCode < msxFirst)
			ENgeterror(errorCode, errorMessage, MAXERRMSG);
		else
			MSXgeterror(errorCode, errorMessage, MAXERRMSG);
			fprintf(stderr, "EPANET-MSX run-time error.\n"); 
			fprintf(stderr, "%s\n", errorMessage);
			exit(1);
	}
	else {
		return(errorCode);
	}
}




/* CAUTION: The following functions are duplicated in erdinternal.c */
static void initializeNetworkData(PNodeInfo *nodes, PLinkInfo *links, PNetInfo net)
{
	int i;
	PNodeInfo tnodes;
	PLinkInfo tlinks;
	
	MEMCHECK(tnodes = *nodes = (PNodeInfo)calloc(net->numNodes, sizeof(NodeInfo)), "*nodes in initializeNetworkData");
	for(i = 0; i < net->numNodes; i++) {
		tnodes[i].type = junction;
		tnodes[i].x = FLT_MAX;
		tnodes[i].y = FLT_MAX;
	}
	
	MEMCHECK(tlinks = *links = (PLinkInfo)calloc(net->numLinks, sizeof(LinkInfo)), "*links in initializeNetworkData");
	for(i = 0; i < net->numLinks; i++) {
		tlinks[i].length = FLT_MAX;
		tlinks[i].nv = 0;
		tlinks[i].vx = NULL;
		tlinks[i].vy = NULL;
	}
	
}

static PNetInfo initializeNetInfo(int speciesCount)
{
	PNetInfo net;
	int i;
	
	MEMCHECK(net = (PNetInfo)calloc(1, sizeof(NetInfo)), "net in initializeNetInfo");
	net->numSpecies = speciesCount;
	MEMCHECK(net->species = (PSpeciesData *)calloc(speciesCount, sizeof(PSpeciesData)), "net->species in initializeNetInfo");
	for(i = 0; i < speciesCount; i++) {
		MEMCHECK(net->species[i] = (PSpeciesData)calloc(1, sizeof(SpeciesData)), "net->species[i] in initializeNetInfo");
	}
	return net;
}

/*
PHydData initHyd(PNetInfo network)
{
	int i;
	PHydData hyd;
	
	
	MEMCHECK(hyd = (PHydData)calloc(1, sizeof(HydData)), "hyd in initHyd");
	
	MEMCHECK(hyd->linkStatus = (int **)calloc(network->numSteps, sizeof(int*)), "hyd->linkStatus in initHyd");
	for(i=0; i<network->numSteps; i++) {
		MEMCHECK(hyd->linkStatus[i] = (int *)calloc(network->numControlLinks, sizeof(int)), "hyd->linkStatus[i] in initHyd");
	}
	MEMCHECK(hyd->velocity = (float **)calloc(network->numSteps, sizeof(float *)), "hyd->velocity in initHyd");
	for(i = 0; i < network->numSteps; i++) {
		MEMCHECK(hyd->velocity[i] = (float *)calloc(network->numLinks, sizeof(float)), "hyd->velocity[i] in initHyd");
	}
	MEMCHECK(hyd->flow = (float **)calloc(network->numSteps, sizeof(float *)), "hyd->flow in initHyd");
	for(i = 0; i < network->numSteps; i++) {
		MEMCHECK(hyd->flow[i] = (float *)calloc(network->numLinks, sizeof(float)), "hyd->flow[i] in initHyd");
	}
	MEMCHECK(hyd->demand = (float **)calloc(network->numSteps, sizeof(float *)), "hyd->demand in initHyd");
	for(i = 0; i < network->numSteps; i++) {
		MEMCHECK(hyd->demand[i] = (float *)calloc(network->numNodes, sizeof(float)), "hyd->demand[i] in initHyd");
	}
	MEMCHECK(hyd->pressure = (float **)calloc(network->numSteps, sizeof(float *)), "hyd->pressure in initHyd");
	for(i = 0; i < network->numSteps; i++) {
		MEMCHECK(hyd->pressure[i] = (float *)calloc(network->numNodes, sizeof(float)), "hyd->pressure[i] in initHyd");
	}
	return hyd;
}

PQualData initQual(int numNodes, int numLinks, int numSpecies, int numSteps, PSpeciesData *species)
{
	int s, n;
	PQualData qual;

	//Allocate QualData
	MEMCHECK(qual = (PQualData)calloc(1, sizeof(QualData)), "qual in getNetworkData");
	MEMCHECK(qual->nodeC = (float ***)calloc(numSpecies, sizeof(float **)), "qual->nodeC in ENL_getNetworkData");
	MEMCHECK(qual->linkC = (float ***)calloc(numSpecies, sizeof(float **)), "qual->linkC in ENL_getNetworkData");
	for(s = 0; s < numSpecies; s++) {
		if( species[s]->type == bulk ) 
		{
			MEMCHECK(qual->nodeC[s] = (float **)calloc(numNodes, sizeof(float *)), "qual->nodeC[s] in ENL_getNetworkData");
			for(n = 0; n < numNodes; n++) {
				MEMCHECK(qual->nodeC[s][n] = (float *)calloc(numSteps, sizeof(float)), "qual->nodeC[s][n] in ENL_getNetworkData");
			}
		}
		else
		{
			MEMCHECK(qual->linkC[s] = (float **)calloc(numLinks, sizeof(float *)), "qual->linkC[s] in ENL_getNetworkData");
			for(n = 0; n < numLinks; n++) {
				MEMCHECK(qual->linkC[s][n] = (float *)calloc(numSteps, sizeof(float)), "qual->linkC[s][n] in ENL_getNetworkData");
			}
		}
	}
	return qual;
}
*/
static int gcd(int a, int b)
{
	if(b==0) return a;
	return gcd(b,a % b);
}

static int lcm(int a, int b)
{
	return (a*b)/gcd(a,b);
}

LIBEXPORT(void) ENL_createDemandProfiles(PERD db)
{
	PNetInfo net = db->network;
	PNodeInfo nodes = db->nodes;
	int n;
	
	PatternsPtr patterns = (PatternsPtr)calloc(1, sizeof(Patterns));
	for(n = 0; n < net->numNodes; n++) {
		int enIdx = n + 1, d;
		PatternInfoPtr *allPatts;
		float *demands;
		float *combinedDemands;
		PNodeInfo node = &nodes[n];
		int numDemands;
		int combinedLength = 0;
		ERDCHECK(ENgetnumdemands(enIdx, &numDemands));
		allPatts = (PatternInfoPtr *)calloc(numDemands, sizeof(PatternInfoPtr));
		demands = (float *)calloc(numDemands, sizeof(float));
		
		for(d = 0; d < numDemands; d++) {
			ERDCHECK(ENgetbasedemand(enIdx, d, &demands[d]));
			if(demands[d] != 0) {
				int pattID;
				ERDCHECK(ENgetdemandpattern(enIdx, d, &pattID));
				allPatts[d] = getPattern(pattID, patterns);
				if(combinedLength == 0) 
					combinedLength = allPatts[d]->numValues;
				else 
					combinedLength = lcm(combinedLength, allPatts[d]->numValues);
			}
		}
		combinedDemands = (float*)calloc(combinedLength, sizeof(float));
		for(d = 0 ;d < numDemands; d++) {
			float baseDemand = demands[d];
			if(baseDemand != 0) {
				PatternInfoPtr patt = allPatts[d];
				float *mult = patt->mult;
				int i = 0;
				while(i < combinedLength) {
					int j;
					for(j = 0; j < patt->numValues; j++, i++) {
						combinedDemands[i] += baseDemand * mult[j];
					}
				}
			}
		}
		node->demandProfile = combinedDemands;
		node->demandProfileLength = combinedLength;
		free(allPatts);
		free(demands);
	}
	for(n = 0; n < patterns->num; n++) {
		if(patterns->patterns[n] != NULL) {
			free(patterns->patterns[n]->mult);
			free(patterns->patterns[n]);
		}
	}
	free(patterns->patterns);
	free(patterns);
}

static PatternInfoPtr getPattern(int pattID, PatternsPtr patterns)
{
	int id = pattID; // epanet indices are 1-based
	if(id >= patterns->num) {
		int i;
		// increase the patterns array...
		patterns->patterns = (PatternInfoPtr*)realloc(patterns->patterns, (id + 1) * sizeof(PatternInfoPtr));
		for(i = patterns->num; i < id + 1; i++) {
			patterns->patterns[i] = NULL;
		}
		patterns->num = id+1;
	}
	if(patterns->patterns[id] == NULL) {
		int plen, pi;
		PatternInfoPtr p = (PatternInfoPtr)calloc(1, sizeof(PatternInfo));
		patterns->patterns[id] = p;
		if(pattID > 0) {
			ERDCHECK(ENgetpatternlen(pattID, &plen));
		} else {
			plen=1;
		}
		p->numValues = plen;
		p->mult = (float*)calloc(plen, sizeof(float));
		if(pattID > 0) {
			for(pi = 0; pi < plen; pi++) {
				ERDCHECK(ENgetpatternvalue(pattID, pi + 1, &p->mult[pi]));
			}
		} else {
			p->mult[0]=1;
		}
	}
	return patterns->patterns[id];
}

LIBEXPORT(void) ENL_writeTSI(PNetInfo net, PNodeInfo nodes, PSource sources, FILE *simgen, FILE *simin)
/*
**--------------------------------------------------------------
**  Input:   simin = pointer to simulation input stream
**           simgen = pointer to simulation generator stream
**  Output:
**           Returns error code, or 0 if successful
**  Purpose: Processes simulation generator file to create
**           threat simulation input (TSI) file.
**--------------------------------------------------------------
*/
{
    char line[MAXLINE+1],     /* Line from input data file       */
        wline[MAXLINE+1];     /* Working copy of input line      */
    int nnodes=net->numNodes,   /* Number of nodes                 */
        sourcetype = 0,       /* source type                     */
        speciesindex,         /* source species index            */
        ntokens = 0,          /* # tokens in input line          */
        numsources;           /* number of sources               */
    float sourcestrength;     /* source strength                 */
    long starttime,           /* start time                      */
        stoptime;             /* stop time                       */
    char *tok[MAXTOKS];       /* Array of token strings          */

    /* Read a line from simulation generator input file. */
    while (fgets(line,MAXLINE,simgen) != NULL)
    {

        /* Make copy of line and scan for tokens */
        strcpy(wline,line);
        ntokens = getTokens(wline,tok);

        /* Skip blank lines and comments */
        if (ntokens == 0) continue;
        if (*tok[0] == ';') continue;

        /* Check if max. length exceeded */
        if (strlen(line) >= MAXLINE) enlError(1,"TSG file error");

        /* Otherwise process input line */
		if(net->qualCode==MULTISPECIES) {
			numsources = ntokens - 5;                                                  /* number of sources         */
			if (numsources > MAXSOURCES || numsources < 1) enlError(1,"TSG file error");
			if ( match(tok[numsources],"CONCEN") ) {sourcetype=EN_CONCEN;}
			else if ( match(tok[numsources],"SETPOINT") ) {sourcetype=EN_SETPOINT;}
			else if ( match(tok[numsources],"FLOWPACED") ) {sourcetype=EN_FLOWPACED;}
			else if ( match(tok[numsources],"MASS") ) {sourcetype=EN_MASS;}
			else enlError(1,"TSG file error");
			if ( MSXgetindex(MSX_SPECIES,tok[numsources+1],&speciesindex) ) enlError(1,"TSG file error: bad species ID");  /* source species ID */
			speciesindex-=1;  // locally this is 0-based
			if ( !getFloat(tok[numsources+2],&sourcestrength) ) enlError(1,"TSG file error");/* source strength            */
			if ( !getLong(tok[numsources+3],&starttime) )  enlError(1,"TSG file error");     /* source start time          */
			if ( !getLong(tok[numsources+4],&stoptime) ) enlError(1,"TSG file error");       /* source stop time           */
		} else {
			numsources = ntokens - 4;                                                  /* number of sources         */
			if (numsources > MAXSOURCES || numsources < 1) enlError(1,"TSG file error");
			if ( match(tok[numsources],"CONCEN") ) {sourcetype=EN_CONCEN;}
			else if ( match(tok[numsources],"SETPOINT") ) {sourcetype=EN_SETPOINT;}
			else if ( match(tok[numsources],"FLOWPACED") ) {sourcetype=EN_FLOWPACED;}
			else if ( match(tok[numsources],"MASS") ) {sourcetype=EN_MASS;}
			else enlError(1,"TSG file error");
			speciesindex=0;
			if ( !getFloat(tok[numsources+1],&sourcestrength) ) enlError(1,"TSG file error");/* source strength            */
			if ( !getLong(tok[numsources+2],&starttime) )  enlError(1,"TSG file error");     /* source start time          */
			if ( !getLong(tok[numsources+3],&stoptime) ) enlError(1,"TSG file error");       /* source stop time           */
		}
        expandGen(nnodes, nodes, sourcetype, speciesindex, sourcestrength, starttime, stoptime, numsources, 0, tok, sources, simin);

    }   /* End of while */

    rewind(simin);
}   /* End of WriteTSI  */

void expandGen(int nnodes, PNodeInfo nodes, int sourcetype, int speciesindex, float sourcestrength, long starttime, 
               long stoptime, int numsources, int is, char **nodetok, PSource sources, 
               FILE *simin)
/*
**--------------------------------------------------------------
**  Purpose: Recursive processing of TSG file source node tokens
**--------------------------------------------------------------
*/
{
    int i, itok, nodetype;
//    float demand;

	if (is < numsources-1) {  /* Working down the stack of node tokens */

        if ( match(nodetok[is],"ALL") ) {
            for (i=1; i<=nnodes; i++) {
                ENCHECK(ENgetnodeid(i,sources[is].sourceNodeID) );
                ENCHECK(ENgetnodetype(i,&nodetype) );
                if (nodetype == EN_JUNCTION) {
                    expandGen(nnodes, nodes, sourcetype, speciesindex, sourcestrength, starttime, stoptime, numsources, is+1, nodetok, sources, simin);
                }
            }
        }
        else if ( match(nodetok[is],"NZD") ) {
            for (i=1; i<=nnodes; i++) {
				int j;
				int hasDemand=0;
                ENCHECK(ENgetnodeid(i,sources[is].sourceNodeID) );
                ENCHECK(ENgetnodetype(i,&nodetype) );
				for(j=0;!hasDemand&&j<nodes[i-1].demandProfileLength; j++) {
					if(nodes[i-1].demandProfile[j] != 0)
						hasDemand=1;
				}
                if (nodetype == EN_JUNCTION && hasDemand) {
                    expandGen(nnodes, nodes, sourcetype, speciesindex, sourcestrength, starttime, stoptime, numsources, is+1, nodetok, sources, simin);
                }
            }
        }
        else {
            ENCHECK(ENgetnodeindex(nodetok[is],&i));
            strcpy(sources[is].sourceNodeID,nodetok[is]);
            expandGen(nnodes, nodes, sourcetype, speciesindex, sourcestrength, starttime, stoptime, numsources, is+1, nodetok, sources, simin);
        }
    }

    else {  /* Last node token */

        if ( match(nodetok[is],"ALL") ) {
            for (i=1; i<=nnodes; i++) {
                ENCHECK(ENgetnodeid(i,sources[is].sourceNodeID) );
                ENCHECK(ENgetnodetype(i,&nodetype) );
                if (nodetype == EN_JUNCTION) {
                    for (itok=0; itok < numsources; itok++) {
                        fprintf(simin," %-15s %-15d %-15d %-15e %-15d %-15d",sources[itok].sourceNodeID,sourcetype,speciesindex,sourcestrength,starttime,stoptime);
                    }
                    fprintf(simin,"\n");
                }
            }
        }
        else if ( match(nodetok[is],"NZD") ) {
            for (i=1; i<=nnodes; i++) {
				int j;
				int hasDemand=0;
                ENCHECK(ENgetnodeid(i,sources[is].sourceNodeID) );
                ENCHECK(ENgetnodetype(i,&nodetype) );
				for(j=0;!hasDemand&&j<nodes[i-1].demandProfileLength; j++) {
					if(nodes[i-1].demandProfile[j] != 0)
						hasDemand=1;
				}
                if (nodetype == EN_JUNCTION && hasDemand) {
                    for (itok=0; itok < numsources; itok++) {
                        fprintf(simin," %-15s %-15d %-15d %-15e %-15d %-15d",sources[itok].sourceNodeID,sourcetype,speciesindex,sourcestrength,starttime,stoptime);
                    }
                    fprintf(simin,"\n");
                }
            }
        }
        else {
            ENCHECK(ENgetnodeindex(nodetok[is],&i));
            strcpy(sources[is].sourceNodeID,nodetok[is]);
            for (itok=0; itok < numsources; itok++) {
                fprintf(simin," %-15s %-15d %-15d %-15e %-15d %-15d",sources[itok].sourceNodeID,sourcetype,speciesindex,sourcestrength,starttime,stoptime);
            }
            fprintf(simin,"\n");
        }
    }
}  /* End of ENTUExpandGen */

LIBEXPORT(int)  ENL_saveInitQual(PNetInfo net, double ***initQual)
{
	int i,j;
	double **iq;
	MEMCHECK(iq = (double**)calloc(net->numSpecies,sizeof(double*)),"*initQual in ENL_saveInitQual");
	for(i=0;i<net->numSpecies;i++) {
		if(net->species[i]->type==bulk) {
			MEMCHECK(iq[i]=(double*)calloc(net->numNodes,sizeof(double)),"initQual[i] in ENL_saveInitQual");
			for(j=0;j<net->numNodes;j++) {
				MSXgetinitqual(MSX_NODE,j+1,i+1,&iq[i][j]);
			}
		} else {
			MEMCHECK(iq[i]=(double*)calloc(net->numLinks,sizeof(double)),"initQual[i] in ENL_saveInitQual");
			for(j=0;j<net->numLinks;j++) {
				MSXgetinitqual(MSX_LINK,j+1,i+1,&iq[i][j]);
			}
		}
	}
	*initQual=iq;
	return 0;
}

LIBEXPORT(int)  ENL_restoreInitQual(PNetInfo net, double **initQual)
{
	int i,j;
	for(i=0;i<net->numSpecies;i++) {
		if(net->species[i]->type==bulk) {
			for(j=0;j<net->numNodes;j++) {
				MSXsetinitqual(MSX_NODE,j+1,i+1,initQual[i][j]);
			}
		} else {
			for(j=0;j<net->numLinks;j++) {
				MSXsetinitqual(MSX_LINK,j+1,i+1,initQual[i][j]);
			}
		}
	}
	return 0;
}
LIBEXPORT(int)  ENL_releaseInitQual(PNetInfo net, double ***initQual)
{
	int i;
	double **iq;
	iq = *initQual;
	for(i=0;i<net->numSpecies;i++) {
		free(iq[i]);
	}
	free(iq);
	*initQual=NULL;
	return 0;
}

LIBEXPORT(int) ENL_setSource(PSourceData source, PNetInfo net, FILE *simin,int isMSX)
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
    int   i,is;
    char  line[MAXLINE+1],     /* Line from input data file       */
          wline[MAXLINE+1];    /* Working copy of input line      */
    int   ntokens = 0,         /* # tokens in input line          */
          tokspersource = 6,   /* # tokens to describe one source */
          nreadtoks;           /* # of read tokens                */
    char  *tok[MAXTOKS];       /* Array of token strings          */

    /* Read a line from input file. */
    while (fgets(line,MAXLINE,simin) != NULL)
	{
    	PSource sources=source->source;
        /* Make copy of line and scan for tokens */
        strcpy(wline,line);
        ntokens = getTokens(wline,tok);

        /* Skip blank lines and comments */
        if (ntokens == 0) continue;
        if (*tok[0] == ';') continue;

        /* Check if max. length exceeded */
        if (strlen(line) >= MAXLINE) enlError(1,"TSI file error");

        /* Check for proper number of tokens */
        if ( ntokens%tokspersource != 0 ) enlError(1,"TSI file error");

        /* Otherwise process input line */
        if ( (source->nsources = ntokens/tokspersource) > MAXSOURCES) enlError(1,"TSI file error");          /* Max # of quality sources   */
        for (is = 0; is < source->nsources; is++) {
            nreadtoks = is*tokspersource;
            if ( strlen(tok[nreadtoks]) > 64 ) enlError(1,"TSI file error: Node ID too long");
            ENCHECK( ENgetnodeindex(tok[nreadtoks],&sources[is].sourceNodeIndex) );                    /* Node index of source       */
            strcpy(sources[is].sourceNodeID,tok[nreadtoks]);                                           /* source node ID             */
            if ( !getInt(tok[nreadtoks+1],&sources[is].sourceType) ) enlError(1,"TSI file error"); /* Source type                */
            if ( sources[is].sourceType < EN_CONCEN || sources[is].sourceType > EN_FLOWPACED ) enlError(1,"TSI file error");
            if ( !getInt(tok[nreadtoks+2],&sources[is].speciesIndex) ) enlError(1,"TSI file error"); /* Species Index                */
            if ( sources[is].speciesIndex < 0 || sources[is].speciesIndex > net->numSpecies ) enlError(1,"TSI file error");
            if ( !getFloat(tok[nreadtoks+3],&sources[is].sourceStrength) ) enlError(1,"TSI file error");  /* source strength            */
            if ( !getLong(tok[nreadtoks+4],&sources[is].sourceStart) ) enlError(1,"TSI file error");      /* start time                 */
            if ( sources[is].sourceStart < 0 || sources[is].sourceStart > net->simDuration ) enlError(1,"TSI file error: SourceStart");
            if ( !getLong(tok[nreadtoks+5],&sources[is].sourceStop) ) enlError(1,"TSI file error");       /* stop time                  */
            if ( sources[is].sourceStop < sources[is].sourceStart || sources[is].sourceStop > net->simDuration ) enlError(1,"TSI file error: SourceStop");
        }
        for (i = 1; i <= net->numNodes; i++ ) { /* Override ALL *.inp source and WQ information */
	        ENCHECK( ENsetnodevalue(i,EN_SOURCETYPE,EN_CONCEN) );  /* CONCEN type                */
		    ENCHECK( ENsetnodevalue(i,EN_SOURCEQUAL,0.0) );        /* Zero all sources           */
			ENCHECK( ENsetnodevalue(i,EN_SOURCEPAT,0) );           /* No pattern                 */
			ENCHECK( ENsetnodevalue(i,EN_INITQUAL,0) );            /* Initial quality = 0        */
		}
		if(isMSX) {
	        for (i = 0; i < net->numNodes; i++ ) { /* Override ALL *.inp source and WQ information */
		        for (is = 0; is < net->numSpecies; is++) {
					MSXCHECK( MSXsetinitqual(net->species[is]->type==bulk?MSX_NODE:MSX_LINK,i+1,is+1,0));
				}
			}
		}
    return(1);
    }   /* End of while */
  return(0);
}     /* End of ENTU_SetSource  */

