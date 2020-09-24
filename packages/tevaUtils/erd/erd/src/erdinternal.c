#include "erdinternal.h"
#include "lzma_enc.h"
#include "lzma_dec.h"
#include <time.h>

int checkFlags(PERD db);

PHydraulicsSim getHydSimIdx(PQualitySim qualSim, PERD db)
{
	return db->hydSim[qualSim->hydSimIdx];
}

void getPrologueFilename(PERD db, char *prologueFilename, int withDir)
{
	char *prologueExt = ".erd";

	if(withDir) {
		strcpy(prologueFilename, db->directory);
		strcat(prologueFilename, "/");
		strcat(prologueFilename, db->baseName);
	}
	else {
		strcpy(prologueFilename, db->baseName);
	}
	strcat(prologueFilename, prologueExt);
}

void getIndexFilename(PERD db, char *indexFilename, int withDir)
{
	char *indexExt = ".index.erd";
	
	if(withDir) {
		strcpy(indexFilename, db->directory);
		strcat(indexFilename, "/");
		strcat(indexFilename, db->baseName);
	}
	else {
		strcpy(indexFilename, db->baseName);
	}
	strcat(indexFilename, indexExt);
}

void getHydFilename(PERD db, int idx, char *hydFilename, int withDir)
{
	char *hydExt = ".hyd.erd";
	char buffer[5]; 
	
	if(withDir) {
		strcpy(hydFilename, db->directory);
		strcat(hydFilename, "/");
		strcat(hydFilename, db->baseName);
	}
	else {
		strcpy(hydFilename, db->baseName);
	}
	strcat(hydFilename, "-");
	sprintf(buffer, "%d", idx);
	strcat(hydFilename, buffer);
	strcat(hydFilename, hydExt);
}

void getQualFilename(PERD db, int idx, char *qualFilename, int withDir)
{
	char *qualExt = ".qual.erd";
	char buffer[5];
	
	if(withDir) {
		strcpy(qualFilename, db->directory);
		strcat(qualFilename, "/");
		strcat(qualFilename, db->baseName);
	}
	else {
		strcpy(qualFilename, db->baseName);
	}
	strcat(qualFilename, "-");
	sprintf(buffer, "%d", idx);
	strcat(qualFilename, buffer);
	strcat(qualFilename, qualExt);
}


char *getHydDataFilename(PERD db, PHydraulicsSim hydSim)
{
	return db->hydFilenames[hydSim->fileIdx];
}

char *getQualDataFilename(PERD db, PQualitySim qualSim)
{
	return db->qualFilenames[qualSim->fileIdx];
}

PERD initializeDatabase(const char *erdName)
{
	PERD db;
	int i;
	
	MEMCHECK(db = (PERD)calloc(1, sizeof(ERD)), "db in initalizeDatabase");

	for(i = 0; i < HEADER_SIZE; i++)
		db->header[i] = 0;
	db->header[0] = 0xff;
	db->header[1] = ERD_VERSION;
	db->header[2] = lzma;
	db->header[3] = 64;
	db->header[4] = (int)undefinedApp;
	db->header[5] = ALL_ON;

	db->hydStream = NULL;
	db->qualStreams = NULL;
	db->indexStream = NULL;
	db->prologueStream = NULL;
	db->status = undefined;
	db->hydFileOpened = -1;
	db->hydSimInMemory = NULL;
	db->qualSimInMemory = NULL;
	db->hydFileCount = 0;
	db->qualFileCount = 0;
	db->qualSimCount = 0;
	db->hydSimCount = 0;
	getBaseNameAndDirectory(db,(char*)erdName);
	db->hydFilenames = NULL;
	db->qualFilenames = NULL;
	MEMCHECK(db->hydSim = (PHydraulicsSim *)calloc(1, sizeof(PHydraulicsSim)), "db->hydSim in initializedatabase");
	MEMCHECK(db->qualSim = (PQualitySim *)calloc(1, sizeof(PQualitySim)), "db->qualSim in initializedatabase");
	db->network = NULL;
	db->nodes = NULL;
	db->links = NULL;
	db->readFunction = NULL;
	db->writeFunction = NULL;

	return db;
}
void getBaseNameAndDirectory(PERD db, char *fn)
{
	char *si,*ts;
	int len;
	si = strrchr(fn,'\\');
	if(si == NULL) si = strrchr(fn,'/');
	if(si == NULL) {
		MEMCHECK(db->directory = (char *)calloc(2,sizeof(char)),"db->directory in getBaseNameAndDirectory");
		db->directory[0]='.';
		db->directory[1]='\0';
		si=&fn[0];
	} else {
		len=(int)(si-fn);
		MEMCHECK(db->directory = (char *)calloc(len+1,sizeof(char)),"tso->directory in getBaseNameAndDirectory");
		memcpy(db->directory,fn,len);
		db->directory[len] = '\0';
		si++;
	}
	ts = strrchr(si,'.');
	if(ts != 0)
		len = (int)(ts-si);
	else
		len = (int)strlen(si);
	db->baseName = (char *)calloc(len+1,sizeof(char));
	memcpy(db->baseName,si,len);
}

/* This function sets the pointers to application specific
read/write functions in the database, based on an application
type flag.  Used to switch between applications that use the 
database - such as TEVA and DPX. */
int setAppRWFunctions(PERD db, enum OutputFrom application) 
{
	if(application == teva) {
		db->readFunction = &readTEVAIndexData;
		db->writeFunction = &writeTEVAIndexData;
		db->freeFunction = &freeTEVAIndexData;
	}
	else if(application == epanetdpx) {
		db->readFunction = &readDPXIndexData;
		db->writeFunction = &writeDPXIndexData;
		db->freeFunction = &freeDPXIndexData;
	}
	else
		return TRUE;  // Unrecognized application
	return FALSE;
}

/* allocate memory for node and link structures, fill with default values */
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
		tlinks[i].diameter = FLT_MAX;
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

void releaseNetInfo(PNetInfo net)
{
	int i;
	
	// Free hydraulics and quality data
	if(net->hydResults != NULL) {
		freeHyd(net->numSteps, net->hydResults);
	}
	if(net->qualResults != NULL) {
		freeQual(net->numSpecies, net->numLinks, net->numNodes, net->qualResults, net->species);
	}
	for(i = 0; i < net->numSpecies; i++) {
		free(net->species[i]);
	}
	free(net->species);
	free(net->controlLinks);
	free(net);
	net = NULL;
}

int currentHyd(PHydraulicsSim hsp, PERD db)
{
	return db->hydSimInMemory == hsp;
}

int currentQual(PQualitySim qsp, PERD db)
{
	return db->qualSimInMemory == qsp;
}


int readPrologue(PERD db)
{
	int i, j,
		length = -1,
		errorCode = 0,
		numNodes, numLinks, numTanks, numJunctions, numSpecies;
	char *fn;
	PNetInfo net;
	PNodeInfo nodes = NULL;
	PLinkInfo links = NULL;

	if(fread(&db->header, sizeof(char), HEADER_SIZE, db->prologueStream) < HEADER_SIZE) 
		return 732;
	if(fread(&db->hydSimCount, sizeof(int), 1, db->prologueStream) != 1) 
		return 732;
	if(fread(&db->qualSimCount, sizeof(int), 1, db->prologueStream) != 1) 
		return 732;
	if(fread(&numNodes, sizeof(int), 1, db->prologueStream) < 1)
		return 732;
	if(fread(&numLinks, sizeof(int), 1, db->prologueStream) < 1)
		return 732;
	if(fread(&numTanks, sizeof(int), 1, db->prologueStream) < 1)
		return 732;
	if(fread(&numJunctions, sizeof(int), 1, db->prologueStream) < 1)
		return 732;
	if(fread(&numSpecies, sizeof(int), 1, db->prologueStream) < 1)
		return 732;
	// check flags vs. what's stored.
	if(!checkFlags(db)) {
		return 747;
	}
	net = db->network = initializeNetInfo(numSpecies);
	net->numNodes = numNodes;
	net->numLinks = numLinks;
	net->numTanks = numTanks;
	net->numJunctions = numJunctions;
	net->numSpecies = numSpecies;
	
	if(fread(&net->numSteps, sizeof(int), 1, db->prologueStream) < 1)
		errorCode = 732;
	if(fread(&net->stepSize, sizeof(float), 1, db->prologueStream) < 1)
		errorCode = 732;
	if(fread(&net->fltMax, sizeof(float), 1, db->prologueStream) < 1)
		errorCode = 732;
	if(fread(&net->qualCode, sizeof(int), 1, db->prologueStream) < 1)
		errorCode = 732;
	if(fread(&net->simDuration, sizeof(int), 1, db->prologueStream) < 1)
		errorCode = 732;
	if(fread(&net->reportStart, sizeof(int), 1, db->prologueStream) < 1)
		errorCode = 732;
	if(fread(&net->reportStep, sizeof(int), 1, db->prologueStream) < 1)
		errorCode = 732;
	if(fread(&net->simStart, sizeof(int), 1, db->prologueStream) < 1)
		errorCode = 732;
	
	// read control link information
	if(fread(&net->numControlLinks, sizeof(int), 1, db->prologueStream) < 1)
		errorCode = 732;
	// allocate space for all the control link indices
	MEMCHECK(net->controlLinks = (int *)calloc(net->numControlLinks, sizeof(int)), "net->controlLinks in readPrologue");
	for(i=0; i<net->numControlLinks; i++) {
		if(fread(&net->controlLinks[i], sizeof(int), 1, db->prologueStream) < 1)
			errorCode = 732;
	}
	
	// Allocate space for the node and link data
	initializeNetworkData(&nodes, &links, net);
	for( i = 0; i < net->numNodes; i++ ) {
		MEMCHECK(nodes[i].nz = (int *)calloc(net->numSpecies, sizeof(int)), "nodes[i].nz in readPrologue");
	}
	for( i = 0; i < net->numLinks; i++ ) {
		MEMCHECK(links[i].nz = (int *)calloc(net->numSpecies, sizeof(int)), "nodes[i].nz in readPrologue");
	}

	for(i = 0; i < net->numTanks; i++) {
        if(fread(&j, sizeof(int), 1, db->prologueStream) != 1) 
			errorCode = 732;
		else {
			nodes[j].type = tank;
		}
    }
	if(errorCode) return errorCode;

    for(i = 0; i< net->numNodes; i++) {
		memset(nodes[i].id, '\0', MAX_ID_LENGTH + 1);
        if(fread(&nodes[i].id, sizeof(char), MAX_ID_LENGTH, db->prologueStream) != MAX_ID_LENGTH)
			errorCode = 732;
    }
    for(i = 0; i< net->numLinks; i++) {
		memset(links[i].id, '\0', MAX_ID_LENGTH + 1);
		if(fread(&links[i].id, sizeof(char), MAX_ID_LENGTH, db->prologueStream) != MAX_ID_LENGTH)
			errorCode = 732;
    }
	for(i = 0; i < net->numSpecies; i++) {
		if(fread(&net->species[i]->index, sizeof(int), 1, db->prologueStream) != 1)
			errorCode = 732;
		if(fread(&net->species[i]->type, sizeof(enum SpecieTypes), 1, db->prologueStream) != 1)
			errorCode = 732;
		memset(net->species[i]->id, '\0', MAX_ID_LENGTH + 1);
		if(fread(&net->species[i]->id, sizeof(char), MAX_ID_LENGTH, db->prologueStream) != MAX_ID_LENGTH)
			errorCode = 732;
	}
    for(i = 0; i < net->numNodes; i++) {
        if(fread(&nodes[i].x, sizeof(float), 1, db->prologueStream) != 1) 
			errorCode = 732;
    }
    for(i = 0; i < net->numNodes; i++) {
        if(fread(&nodes[i].y, sizeof(float), 1, db->prologueStream) != 1) 
			errorCode = 732;
    }
    for(i = 0; i < net->numLinks; i++) {
        if(fread(&links[i].from, sizeof(int), 1, db->prologueStream) != 1) 
			errorCode = 732;
    }
    for(i = 0;i < net->numLinks; i++) {
        if(fread(&links[i].to, sizeof(int), 1, db->prologueStream) != 1) 
			errorCode = 732;
    }
    for(i = 0;i < net->numLinks; i++) {
		if(fread(&links[i].length, sizeof(float), 1, db->prologueStream) != 1) 
			errorCode = 732;
    }
	if(db->header[1] >=13) {
		for(i = 0;i < net->numLinks; i++) {
			if(fread(&links[i].diameter, sizeof(float), 1, db->prologueStream) != 1) 
				errorCode = 732;
		}
	} else {
		for(i = 0;i < net->numLinks; i++) {
			links[i].diameter=FLT_MAX;
		}
	}
    for(i = 0; i < net->numLinks; i++) {
        if(fread(&links[i].nv, sizeof(int), 1, db->prologueStream) != 1)
			errorCode = 732;
		if(links[i].nv) {
			MEMCHECK(links[i].vx = (float *)calloc(links[i].nv, sizeof(float)), "link[i].vx in readPrologue");
			MEMCHECK(links[i].vy = (float *)calloc(links[i].nv, sizeof(float)), "link[i].vy in readPrologue");
			if(fread(links[i].vx, sizeof(float), links[i].nv, db->prologueStream) < (size_t)links[i].nv)
				errorCode = 732;
			if(fread(links[i].vy, sizeof(float), links[i].nv, db->prologueStream) < (size_t)links[i].nv) 
				errorCode = 732;
		}
	}
    if(errorCode) return errorCode;
    
	length = -1;
	while(length != 0 && !feof(db->prologueStream)) {
		fread(&length, sizeof(int), 1, db->prologueStream);
		if(!feof(db->prologueStream) && length > 0) {
			MEMCHECK(fn = (char *)calloc(length+1, sizeof(char)), "fn in readPrologue");
			if(fread(fn, sizeof(char), length, db->prologueStream) != length)
				errorCode = 732;
			if(strstr(fn, "hyd") != NULL) {
				if(db->hydFilenames == NULL)
					MEMCHECK(db->hydFilenames = (char **)calloc(1, sizeof(char *)), "db->hydFilenames in ERD_readPrologue");
				else
					MEMCHECK(db->hydFilenames = (char **)realloc(db->hydFilenames, (db->hydFileCount + 1) * sizeof(char *)), "db->hydFilenames in ERD_readPrologue");
				MEMCHECK(db->hydFilenames[db->hydFileCount] = (char *)calloc(length+1, sizeof(char)), "db->hydFilenames[db->hydFileCount] in ERD_readPrologue");
				strncpy(db->hydFilenames[db->hydFileCount++], fn, length);
			}
			else if(strstr(fn, "qual") != NULL) {
				if(db->qualFilenames == NULL) 
					MEMCHECK(db->qualFilenames = (char **)calloc(1, sizeof(char *)), "db->qualFilenames in ERD_readPrologue");
				else
					MEMCHECK(db->qualFilenames = (char **)realloc(db->qualFilenames, (db->qualFileCount + 1) * sizeof(char *)), "db->qualFilenames in ERD_readPrologue");
				MEMCHECK(db->qualFilenames[db->qualFileCount] = (char *)calloc(length+1, sizeof(char)), "db->qualFilenames[db->qualFileCount] in ERD_readPrologue");
				strncpy(db->qualFilenames[db->qualFileCount++], fn, length);
			}
			free(fn);
		}
	}

	db->qualStreams = (FILE **)calloc(db->qualFileCount,sizeof(FILE*));
	db->nodes = nodes;
	db->links = links;

	// this needs to be done AFTER species info is read in.
	net->qualResults=ERD_UTIL_initQual(net,db->readFlags);
	net->hydResults=ERD_UTIL_initHyd(net,db->readFlags);

	return errorCode;
}
void appendErrStr(char *es, char *val) {
	if((es)[0] != '\0')
		strcat(es,", ");
	strcat(es,val);
}
/**
 * Checks to make sure that the data that will be read (as specified in
 * db->readFlags) is actually in the ERD db).
 * returns 1 on success, 0 if there is data that was requested, but not stored.
 */
int checkFlags(PERD db) {
	int badFlags=0;
	if((db->readFlags&READ_DEMANDS) && !ISSET_DEMAND(db->header[5]))
		badFlags |= READ_DEMANDS;
	if((db->readFlags&READ_PRESSURE) && !ISSET_PRESSURE(db->header[5]))
		badFlags |= READ_PRESSURE;
	if((db->readFlags&READ_LINKVEL) && !ISSET_VELOCITY(db->header[5]))
		badFlags |= READ_LINKVEL;
	if((db->readFlags&READ_LINKFLOW) && !ISSET_FLOW(db->header[5]))
		badFlags |= READ_LINKFLOW;
	if((db->readFlags&READ_DEMANDPROFILES) && !ISSET_DEMANDPROFILE(db->header[5]))
		badFlags |= READ_DEMANDPROFILES;
	if(badFlags != 0) {
		char errStr[512];
		errStr[0]='\0';
		if(badFlags & READ_DEMANDS) appendErrStr(errStr,"demands");
		if(badFlags & READ_PRESSURE) appendErrStr(errStr,"pressure");
		if(badFlags & READ_LINKVEL) appendErrStr(errStr,"link velocity");
		if(badFlags & READ_LINKFLOW) appendErrStr(errStr,"link flow");
		if(badFlags & READ_DEMANDPROFILES) appendErrStr(errStr,"demand profiles");
		fprintf(stderr,"Data requested that is not stored:\n  %s\n",errStr);
		if(db->readFlags & READ_IGNORE_ERROR) {
			fprintf(stderr,"*** The above is only a warning since the READ_IGNORE_ERROR flag was used\n\n");
		}
	}
	if(db->readFlags & READ_IGNORE_ERROR)
		badFlags=0;
	return badFlags == 0;
}
int writePrologue(PERD db)
{
	PNetInfo net = db->network;
	PNodeInfo nodes = db->nodes;
	PLinkInfo links = db->links;
	int i;
	int length = 0;
	
	rewind(db->prologueStream);
	
	if(fwrite(db->header, sizeof(db->header), 1, db->prologueStream) != 1) return 742;
	fflush(db->prologueStream);

	if(fwrite(&db->hydSimCount, sizeof(int), 1, db->prologueStream) != 1) return 742;
	if(fwrite(&db->qualSimCount, sizeof(int), 1, db->prologueStream) != 1) return 742;
	if(fwrite(&net->numNodes, sizeof(int), 1, db->prologueStream) != 1) return 742;
	if(fwrite(&net->numLinks, sizeof(int), 1, db->prologueStream) != 1) return 742;
	if(fwrite(&net->numTanks, sizeof(int), 1, db->prologueStream) != 1) return 742;
	if(fwrite(&net->numJunctions, sizeof(int), 1, db->prologueStream) != 1) return 742;
	if(fwrite(&net->numSpecies, sizeof(int), 1, db->prologueStream) != 1) return 742;
	if(fwrite(&net->numSteps, sizeof(int), 1, db->prologueStream) != 1) return 742;
	if(fwrite(&net->stepSize, sizeof(float), 1, db->prologueStream) != 1) return 742;
	if(fwrite(&net->fltMax, sizeof(float), 1, db->prologueStream) != 1) return 742;
	if(fwrite(&net->qualCode, sizeof(int), 1, db->prologueStream) != 1) return 742;
	if(fwrite(&net->simDuration, sizeof(int), 1, db->prologueStream) != 1) return 742;
	if(fwrite(&net->reportStart, sizeof(int), 1, db->prologueStream) != 1) return 742;
	if(fwrite(&net->reportStep, sizeof(int), 1, db->prologueStream) != 1) return 742;
	if(fwrite(&net->simStart, sizeof(int), 1, db->prologueStream) != 1) return 742;
	
	// write control link information
	if(fwrite(&net->numControlLinks, sizeof(int), 1, db->prologueStream) != 1) return 742;
	for(i=0; i<net->numControlLinks; i++) {
		if(fwrite(&net->controlLinks[i], sizeof(int), 1, db->prologueStream) != 1) return 742;
	}
	
	for(i = 0; i < net->numNodes; i++) {
		if(nodes[i].type == tank)
			if(fwrite(&i, sizeof(int), 1, db->prologueStream) != 1) return 742;
	}
	for(i = 0; i < net->numNodes; i++) {
		if(fwrite(nodes[i].id, sizeof(char), MAX_ID_LENGTH, db->prologueStream) != MAX_ID_LENGTH) return 742;
	}
	for(i = 0; i < net->numLinks; i++) {
		if(fwrite(links[i].id, sizeof(char), MAX_ID_LENGTH, db->prologueStream) != MAX_ID_LENGTH) return 742;
	}
	for(i = 0; i < net->numSpecies; i++) {
		if(fwrite(&net->species[i]->index, sizeof(int), 1, db->prologueStream) != 1) return 742;
		if(fwrite(&net->species[i]->type, sizeof(enum SpecieTypes), 1, db->prologueStream) != 1) return 742;
		if(fwrite(net->species[i]->id, sizeof(char), MAX_ID_LENGTH, db->prologueStream) != MAX_ID_LENGTH) return 742;
	}
	for(i = 0; i < net->numNodes; i++) 
		if(fwrite(&nodes[i].x, sizeof(float), 1, db->prologueStream) != 1) return 742;
	for(i = 0; i < net->numNodes; i++) 
		if(fwrite(&nodes[i].y, sizeof(float), 1, db->prologueStream) != 1) return 742;
	for(i = 0; i < net->numLinks; i++)
		if(fwrite(&links[i].from, sizeof(int), 1, db->prologueStream) != 1) return 742;
	for(i = 0; i < net->numLinks; i++)
		if(fwrite(&links[i].to, sizeof(int), 1, db->prologueStream) != 1) return 742;
	for(i = 0; i < net->numLinks; i++)
		if(fwrite(&links[i].length, sizeof(float), 1, db->prologueStream) != 1) return 742;
	for(i = 0; i < net->numLinks; i++)
		if(fwrite(&links[i].diameter, sizeof(float), 1, db->prologueStream) != 1) return 742;
	for(i = 0; i < net->numLinks; i++) {
		if(fwrite(&links[i].nv, sizeof(int), 1, db->prologueStream) != 1) return 742;
		if(links[i].nv) {
			if(fwrite(links[i].vx, sizeof(float), links[i].nv, db->prologueStream) != links[i].nv) return 742;
			if(fwrite(links[i].vy, sizeof(float), links[i].nv, db->prologueStream) != links[i].nv) return 742;
		}
	}

	fflush(db->prologueStream);
	
	for(i = 0; i < db->hydFileCount; i++) {
		length = (int)strlen(db->hydFilenames[i]);
		if(fwrite(&length, sizeof(int), 1, db->prologueStream) != 1)
			return 742;
		if(fwrite(db->hydFilenames[i], sizeof(char), length, db->prologueStream) != length)
			return 742;
	}
	fflush(db->prologueStream);	
	for(i = 0; i < db->qualFileCount; i++) {
		length = (int)strlen(db->qualFilenames[i]);
		if(fwrite(&length, sizeof(int), 1, db->prologueStream) != 1)
			return 742;
		if(fwrite(db->qualFilenames[i], sizeof(char), length, db->prologueStream) != length)
			return 742;
	}
	fflush(db->prologueStream);

	return FALSE;
}

int getNLHydData(PHydraulicsSim hydSim, PERD db)
{
	int i;
	char *file, *fn;
	int hydFileIdx;
	__file_pos_t offset;
	PHydData hyd = db->network->hydResults;

	// Open a new file if needed, else continue with current stream
	hydFileIdx = hydSim->fileIdx;
	if( db->hydFileOpened != hydFileIdx ) 
	{
		// Hydraulic simulation is stored in different file
		// Close existing open hydraulics data stream
		if(db->hydStream != NULL) {
			fclose(db->hydStream);
			db->hydStream = NULL;
		}

		// Construct the requested hydraulics file name
		fn = getHydDataFilename(db, hydSim);
		MEMCHECK(file = (char *)calloc(strlen(db->directory) + strlen(fn) + 1 + 1, sizeof(char)), "file in getNLHydData");
		file = strncpy(file, db->directory, strlen(db->directory));
		file = strncat(file, "/", 1);
		file = strncat(file, fn, strlen(fn));

		// Open hydraulics file stream 
		db->hydStream = fopen(file, "rb");
		free(file);
	}
	if(db->hydStream == NULL) 
		return 714;
	offset = hydSim->offset;
	ERD_UTIL_positionFile(db->hydStream, offset);
	db->hydFileOpened = hydFileIdx;

	// Control link information
	if(db->network->numControlLinks > 0) {
		for(i = 0; i < db->network->numSteps; i++) {
			if(fread(hyd->linkStatus[i], sizeof(int), db->network->numControlLinks, db->hydStream) != db->network->numControlLinks)
				return TRUE;
		}
	}

	// Node demands
	if(ISSET_DEMAND(db->header[5])) {
		if(db->header[2]==lzma) {
			if(db->readFlags & READ_DEMANDS) {
				lzma_decompress_float_2d(hyd->demand,db->network->numSteps,db->network->numNodes,db->hydStream);
			} else {
				int cSize;
				fread(&cSize,sizeof(int),1,db->hydStream);
				fseek(db->hydStream,(long)cSize,SEEK_CUR);
			}
		} else {
			if(db->readFlags & READ_DEMANDS) {
				for(i = 0; i < db->network->numSteps; i++) {
					if(fread(hyd->demand[i], sizeof(float), db->network->numNodes, db->hydStream) != db->network->numNodes)
						return TRUE;
				}
			} else {
				fseek(db->hydStream,(long)(db->network->numSteps*db->network->numNodes*sizeof(float)),SEEK_CUR);
			}
		}
	}

	// Node pressures
	if(ISSET_PRESSURE(db->header[5])) {
		if(db->header[2]==lzma) {
			if(db->readFlags & READ_PRESSURE) {
				lzma_decompress_float_2d(hyd->pressure,db->network->numSteps,db->network->numNodes,db->hydStream);
			} else {
				int cSize;
				fread(&cSize,sizeof(int),1,db->hydStream);
				fseek(db->hydStream,(long)cSize,SEEK_CUR);
			}
		} else {
			if(db->readFlags & READ_PRESSURE) {
				for(i = 0; i < db->network->numSteps; i++) {
					if(fread(hyd->pressure[i], sizeof(float), db->network->numNodes, db->hydStream) != db->network->numNodes)
						return TRUE;
				}
			} else {
				fseek(db->hydStream,(long)(db->network->numSteps*db->network->numLinks*sizeof(float)),SEEK_CUR);
			}
		}
	}

	// Link flows
	if(ISSET_FLOW(db->header[5])) {
		if(db->header[2]==lzma) {
			if(db->readFlags & READ_LINKFLOW) {
				lzma_decompress_float_2d(hyd->flow,db->network->numSteps,db->network->numLinks,db->hydStream);
			} else {
				int cSize;
				fread(&cSize,sizeof(int),1,db->hydStream);
				fseek(db->hydStream,(long)cSize,SEEK_CUR);
			}
		} else {
			if(db->readFlags & READ_LINKFLOW) {
				for(i = 0; i < db->network->numSteps; i++) {
					if(fread(hyd->flow[i], sizeof(float), db->network->numLinks, db->hydStream) != db->network->numLinks)
						return TRUE;
				}
			} else {
				fseek(db->hydStream,(long)(db->network->numSteps*db->network->numLinks*sizeof(float)),SEEK_CUR);
			}
		}
	}

	// Link velocity
	if(ISSET_VELOCITY(db->header[5])) {
		if(db->header[2]==lzma) {
			if(db->readFlags & READ_LINKVEL) {
				lzma_decompress_float_2d(hyd->velocity,db->network->numSteps,db->network->numLinks,db->hydStream);
			} else {
				int cSize;
				fread(&cSize,sizeof(int),1,db->hydStream);
				fseek(db->hydStream,(long)cSize,SEEK_CUR);
			}
		} else {
			if(db->readFlags & READ_LINKVEL) {
				for(i = 0; i < db->network->numSteps; i++) {
					if(fread(hyd->velocity[i], sizeof(float), db->network->numLinks, db->hydStream) != db->network->numLinks)
						return TRUE;
				}
			} else {
				fseek(db->hydStream,(long)(db->network->numSteps*db->network->numLinks*sizeof(float)),SEEK_CUR);
			}
		}
	}
    if(ISSET_DEMANDPROFILE(db->header[5])) {
		int length;
		PNodeInfo nodes=db->nodes;
    	for(i = 0; i < db->network->numNodes; i++) {
			size_t nread=fread(&length, sizeof(int), 1, db->hydStream);
			if(nread != 1) {
				return TRUE;
			}
			if(db->readFlags & READ_DEMANDPROFILES) {
				nodes[i].demandProfileLength = length;
   				if(length > 0) {
					MEMCHECK(nodes[i].demandProfile = (float *)calloc(length, sizeof(float)), "node[i].demandProfile in getNLHydData");
					if(db->header[2]==lzma) {
						lzma_decompress_float_1d(nodes[i].demandProfile,length,db->hydStream);
					} else {
						if(fread(nodes[i].demandProfile, sizeof(float), length, db->hydStream) != length) 
							return TRUE;
					}
   				}
			} else {
				if(length > 0) {
					if(db->header[2]==lzma) {
						int cSize;
						fread(&cSize,sizeof(int),1,db->hydStream);
						fseek(db->hydStream,(long)cSize,SEEK_CUR);
					} else {
						fseek(db->hydStream,length*sizeof(float),SEEK_CUR);
					}
				}
			}
    	}
    }

	return FALSE;
}
int ensureQualityFileOpened(PQualitySim qualSim, PERD db)
{
	char *file, *fn;
	if(db->qualStreams != NULL && db->qualStreams[qualSim->fileIdx] != NULL)
		return 1;

	// Construct the requested quality file name
	fn = getQualDataFilename(db, qualSim);
	MEMCHECK(file = (char *)calloc(strlen(db->directory) + strlen(fn) + 1 + 1, sizeof(char)), "file in ensureQualityFileOpened");
	file = strncpy(file, db->directory, strlen(db->directory));
	file = strncat(file, "/", 1);
	file = strncat(file, fn, strlen(fn));

	// Open quality file stream 
	db->qualStreams[qualSim->fileIdx] = fopen(file, "rb");
	free(file);
	return db->qualStreams[qualSim->fileIdx]!=NULL;
}
int getNLQualData(PQualitySim qualSim, PERD db)
{
	__file_pos_t offset;
	int errorCode = FALSE;

	if(!ensureQualityFileOpened(qualSim,db)) {
		return 714;
	}
	offset = qualSim->offset;
	ERD_UTIL_positionFile(db->qualStreams[qualSim->fileIdx], offset);

	// Read quality data
	errorCode = getDecompressedQualData(db,db->qualStreams[qualSim->fileIdx]);
	
	return errorCode;
}

int writeIndex(PERD db)
{
	PQualitySim *qualsim = db->qualSim;
	PHydraulicsSim *hydsim = db->hydSim;
	FILE *stream = db->indexStream;
	int i, errorCode = FALSE;
	unsigned char idxType;

	idxType=IDX_QUAL;
	// Write QualitySim data
	for(i = 0; i < db->qualSimCount; i++)
	{
		if(fwrite(&idxType, sizeof(unsigned char), 1, stream) != 1)
			return 741;
		if(fwrite(&qualsim[i]->fileIdx, sizeof(int), 1, stream) != 1)
			return 741;
		if(fwrite(&qualsim[i]->hydSimIdx, sizeof(int), 1, stream) != 1)
			return 741;
		if(fwrite(&qualsim[i]->length, sizeof(int), 1, stream) != 1)
			return 741;
		if(fwrite(&qualsim[i]->offset, sizeof(__file_pos_t), 1, stream) != 1)
			return 741;
		if(db->writeFunction(qualsim[i]->appData, stream))
			return 741;
	}
	idxType=IDX_HYD;
	// Write HydraulicsSim data
	for(i = 0; i < db->hydSimCount; i++)
	{
		if(fwrite(&idxType, sizeof(unsigned char), 1, stream) != 1)
			return 741;
		if(fwrite(&hydsim[i]->fileIdx, sizeof(int), 1, stream) != 1)
			return 741;
		if(fwrite(&hydsim[i]->length, sizeof(int), 1, stream) != 1)
			return 741;
		if(fwrite(&hydsim[i]->offset, sizeof(__file_pos_t), 1, stream) != 1)
			return 741;
	}

	fflush(stream);

	return errorCode;
}

int readIndex(PERD db)
{	
	int errorCode = 0, j;
	int h=0;
	int q=0;

	// Allocate memory for the database HydraulicsSim and QualitySim structures
	MEMCHECK(db->qualSim = (PQualitySim *)realloc(db->qualSim, db->qualSimCount * sizeof(PQualitySim)), "db->qualSim in readIndex");
	for(j = 0; j < db->qualSimCount; j++) {
		MEMCHECK(db->qualSim[j] = (PQualitySim)calloc(1, sizeof(QualitySim)), "db->qualSim[] in readIndex");
	}
	MEMCHECK(db->hydSim = (PHydraulicsSim *)realloc(db->hydSim, db->hydSimCount * sizeof(PHydraulicsSim)), "db->hydSim in readIndex");
	for(j = 0; j < db->hydSimCount; j++) {
		MEMCHECK(db->hydSim[j] = (PHydraulicsSim)calloc(1, sizeof(HydraulicsSim)), "db->hydSim[] in readIndex");
	}

	while(!feof(db->indexStream)) {
		unsigned char indexType;
		int nread=(int)fread(&indexType,sizeof(unsigned char),1,db->indexStream);
		if(nread != 1) {
			if(!feof(db->indexStream)) {
				return 731;
			}
		} else {
			if(indexType ==IDX_QUAL) {
				if(fread(&db->qualSim[q]->fileIdx, sizeof(int), 1, db->indexStream) != 1)
					return 731;
				if(fread(&db->qualSim[q]->hydSimIdx, sizeof(int), 1, db->indexStream) != 1)
					return 731;
				if(fread(&db->qualSim[q]->length, sizeof(int), 1, db->indexStream) != 1)
					return 731;
				if(fread(&db->qualSim[q]->offset, sizeof(__file_pos_t), 1, db->indexStream) != 1)
					return 731;
				if(db->readFunction(db,db->qualSim[q], db->indexStream))
					return 731;
				q++;
			} else if (indexType==IDX_HYD) {
				if(fread(&db->hydSim[h]->fileIdx, sizeof(int), 1, db->indexStream) != 1)
					return 731;
				if(fread(&db->hydSim[h]->length, sizeof(int), 1, db->indexStream) != 1)
					return 731;
				if(fread(&db->hydSim[h]->offset, sizeof(__file_pos_t), 1, db->indexStream) != 1)
					return 731;
				h++;
			} else {
				return 731;
			}
		}
	}

	// The QualitySim structures are sorted by hydraulics simulation index
	// This probably makes going through them serially a bit faster, cause
	// the hydraulics solution for a group should only be read once.
	sortByGroup(db->qualSim, db->qualSimCount);

	return errorCode;
}

void sortByGroup(PQualitySim *list, int size)
{
	int i, j;
	
	for(i = 0; i < (size - 2); i++) {
		int min = i;
		PQualitySim temp;
		for(j = (i + 1); j < size; j++) {
			if(list[j]->hydSimIdx < list[min]->hydSimIdx)
				min = j;
		}
		temp = list[i];
		list[i] = list[min];
		list[min] = temp;
	}
}

int getDecompressedQualData(PERD db, FILE *qStream)
{
	int resID;
	if(fread(&resID, sizeof(int), 1, qStream) != 1)
		return TRUE;
	if(feof(qStream))
		return TRUE;
	if(IS_COMP_RLE(db)) 
		return readAndDecompressRLE(qStream, db);
	else if(IS_COMP_LZMA(db)) 
		return readAndDecompressLZMA(qStream, db);
	else
		return 746;
}


/*
 * Input: hydraulic simulation index, quality index within hydraulic group, Database pointer
 * Output: Index of qualIdxInGroup'th water quality simulation associated with a hydraulics group
 * Returns < 0 if bad qualIdxInGroup
 * Purpose: Retrieve the Index of the water quality simulation associated with a hydraulics group
 * and simulation number within that group.  This returned index can be used to directly access
 * results stored within the database, which are indexed by sequential integers starting with zero.
 */
int getQualSimIdxFor(int hydSimIdx, int qualIdxInGroup, PERD database)
{
	int i, nqual, count = 0;
	
	nqual = ERD_getQualSimCountFor(hydSimIdx, database);
	if( qualIdxInGroup < 0 || qualIdxInGroup >= nqual ) return -1;

	for(i = 0; i < database->qualSimCount; i++) {
		if(database->qualSim[i]->hydSimIdx == hydSimIdx)
		{
			if( count == qualIdxInGroup ) return i;  // Found the right one
			count++; // Increment number of qual simulation found in group
		}
	}
	
	return -1;
}


void freeQual(int numSpecies, int numLinks, int numNodes, PQualData qual, PSpeciesData *species)
{
	int i, j;
	
	for(i = 0; i < numSpecies; i++) {
		if(species[i]->index!=-1) {
			if( species[i]->type == bulk ) {
				free(qual->nodeC[i][0]);
				//for(j = 0; j < numNodes; j++) {
					//free(qual->nodeC[i][j]);
				//}
				free(qual->nodeC[i]);
			} else {
				for(j = 0; j < numLinks; j++) {
					free(qual->linkC[i][j]);
				}
				free(qual->linkC[i]);
			}
		}
	}
	free(qual->nodeC);
	free(qual->linkC);
	free(qual);
}

void freeHyd(int numSteps, PHydData hyd)
{
	int i;
	
	for(i = 0; i < numSteps; i++) {
		if(hyd->demand)     free(hyd->demand[i]);
		if(hyd->pressure)   free(hyd->pressure[i]);
		if(hyd->flow)       free(hyd->flow[i]);
		if(hyd->velocity)   free(hyd->velocity[i]);
		if(hyd->linkStatus) free(hyd->linkStatus[i]);
	}
	if(hyd->demand)     free(hyd->demand);
	if(hyd->pressure)   free(hyd->pressure);
	if(hyd->flow)       free(hyd->flow);
	if(hyd->velocity)   free(hyd->velocity);
	if(hyd->linkStatus) free(hyd->linkStatus);
	free(hyd);		
}

void loadSources(FILE *stream, PSource sources, PQualitySim qualSim) 
{
	int sourceID;
	PTEVAIndexData index;

	fread(&sourceID, sizeof(int), 1, stream);
	index = (PTEVAIndexData)qualSim->appData;
	memcpy(sources, index->source, index->nsources * sizeof(Source));

}

int getDatabaseType(PERD db)
{
	return db->header[4];
}

/*
 * Input: Hydraulics group index, database pointer, hydraulics data pointer
 * Output: If an error occurs, a non-zero error code value
 * Purpose: Retrieve data for a hydraulics group.
 */
int getHydResults(PHydraulicsSim hydSim, PERD database)
{
	if(hydSim == NULL) {
		return 705;
	}

	if(!currentHyd(hydSim, database)) {
		if(getNLHydData(hydSim, database))
			return 733;
	}
	// Update current database hydraulic and quality simulation index
	database->hydSimInMemory = hydSim;

	return 0;
}
/*
 * Input: hydraulic simulation index (group index), Quality simulation index within group,
 * database pointer, water quality data pointer
 * Output: If an error occurs, a non-zero error code value
 * Purpose: Retrieve indexed water quality data
 */
int getQualResults(PQualitySim qualSim, PERD database)
{
	if(qualSim == NULL) {
		return 705;
	}

	if(!currentQual(qualSim, database)) {
		if(getNLQualData(qualSim, database))
			return 734;
	}

	// Update current database hydraulic and quality simulation index
	database->qualSimInMemory = qualSim;

	return 0;
}
