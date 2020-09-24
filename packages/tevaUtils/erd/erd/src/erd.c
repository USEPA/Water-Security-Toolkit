#include "erdinternal.h"
#include "lzma_enc.h"
#include "lzma_dec.h"
#include <time.h>
/* ========================================================================= */
/* ERD API                                                                   */
/* Creating, opening, closing database and setting its characteristics       */
/* ========================================================================= */

/*
 * Input: Database pointer, directory where database will be created, base of database filenames, 
 *        application that created the database
 * Ouptut: If an error occurs, a non-zero error code value
 * Purpose: Create a new database, setting properties based on its source applicaton.  
 * Create initial hydraulics group for output.  
 */
LIBEXPORT(int) ERD_create(PERD *database, const char *erdName, enum OutputFrom application, enum CompressionLevel compLevel)
{
	PERD db;
	char prologueFilename[MAXFNAME],
		indexFilename[MAXFNAME];
	struct stat statbuf;
	
	if(compLevel < rle)
		return 746;
	// Set up the database
	db = *database = initializeDatabase(erdName);
	if(!db) return 702;
	db->header[2]=compLevel;

	// if the output directory doesn't exist, exit with error.
	stat(db->directory, &statbuf);
	if(S_ISDIR(statbuf.st_mode) == 0)
		return 703;

	// Set the status to "created" so that we know this is a new database
	db->status = created;
	
	// Store the prologue and index file names and open the streams since those don't change.
	getPrologueFilename(db, prologueFilename, TRUE);
	db->prologueStream = fopen(prologueFilename, "wb");
	if(db->prologueStream == NULL) 
		return 712;
	
	getIndexFilename(db, indexFilename, TRUE);
	db->indexStream = fopen(indexFilename, "wb");
	if(db->indexStream == NULL) 
		return 711;

	// Set the application type and associated read/write functions
	db->header[4] = application;
	if(setAppRWFunctions(db, application)) return 745;

	return FALSE;
}

LIBEXPORT(int) ERD_isERD(const char *erdName)
{
	FILE *fp=fopen(erdName,"r");
	size_t nread;
	char header[HEADER_SIZE];
	if(fp==NULL) {
		fprintf(stderr,"Unable to open ERD/TSO file: %s\n",erdName);
		exit(9);
	}
	nread=fread(header, sizeof(char), HEADER_SIZE, fp);
	fclose(fp);
	if(nread < HEADER_SIZE)
		return 0;
	return(header[1] >= FIRST_ERD_VERSION);
}
/*
 * Input: Database pointer, directory where database is located
 * Ouptut: If an error occurs, a non-zero error code value
 * Purpose: Open a database for reading, initialize memory, load prologue and index into 
 * memory, sort index data by hydraulics group. 
 */
LIBEXPORT(int) ERD_open(PERD *database, const char *erdName, int flags)
{
	PERD db;
	char prologueFilename[MAXFNAME],
		indexFilename[MAXFNAME];
	enum OutputFrom application;

	
	// Initialize the database structure
	db = *database = initializeDatabase(erdName);
	if(!db) return 702;

	db->readFlags=flags;
	// Set the status to "opened" so we know this was an existing database
	db->status = opened;
	
	// Store the prologue and index file names and open the streams since those don't change.
	getPrologueFilename(db, prologueFilename, TRUE);
	db->prologueStream = fopen(prologueFilename, "rb");
	if(db->prologueStream == NULL) 
		return 712;
	getIndexFilename(db, indexFilename, TRUE);
	db->indexStream = fopen(indexFilename, "rb");
	if(db->indexStream == NULL) 
		return 711;

	// Read the prologue
	if(readPrologue(db))
		return 732;

	// Set the application specific read/write functions
	application = (enum OutputFrom)db->header[4];
	if(setAppRWFunctions(db, application)) return 745;

	// Read simulation index data
	if(readIndex(db))
		return 730;
	if(db->hydSimCount ==0)
		return 748;
	// call this to ensure that demand profiles are loaded (if they are there and requested)
	getHydResults(db->hydSim[0],db);
	return 0;
}
/*
 * Input: Database pointer
 * Ouptut: If an error occurs, a non-zero error code value
 * Purpose: Close the database, deallocate memory.
 */
LIBEXPORT(int) ERD_close(PERD *database)
{
	PERD db = *database;
	PNodeInfo nodes = db->nodes;
	PLinkInfo links = db->links;
	
	int i,
		errorCode = FALSE;
	
	if(db == NULL) return 704;	

	// Write the prologue and index data if new database was created
	if(db->status == created) {
		if( (errorCode = writePrologue(db)) != 0 ) return errorCode;  
		if( (errorCode = writeIndex(db)) != 0 ) return errorCode;
	}

	// Free storage for Hydraulic and Quality simulation index data
	for(i = 0; i < db->qualSimCount; i++) {
		db->freeFunction(&db->qualSim[i]->appData);
		free(db->qualSim[i]);
	}
	free(db->qualSim);
	db->qualSim = NULL;
	for(i = 0; i < db->hydSimCount; i++) {
		free(db->hydSim[i]);
	}
	free(db->hydSim);
	db->hydSim = NULL;

	// Free storage for hydraulic and quality filenames
	for(i = 0; i < db->hydFileCount; i++) {
		free(db->hydFilenames[i]);
	}
	free(db->hydFilenames);
	for(i = 0; i < db->qualFileCount; i++) {
		free(db->qualFilenames[i]);
	}
	free(db->qualFilenames);

	// Close the open streams
	if(db->hydStream != NULL) {
		if(fclose(db->hydStream)) 
			errorCode = 723;
		db->hydStream = NULL;
	}
	for(i=0;i<db->qualFileCount;i++) {
		if(db->qualStreams[i] != NULL) {
			if(fclose(db->qualStreams[i])) 
				errorCode = 724;
			db->qualStreams[i] = NULL;
		}
	}
	free(db->qualStreams);
	if(db->prologueStream != NULL) {
		if(fclose(db->prologueStream)) 
			errorCode = 722;
		db->prologueStream = NULL;
	}
	if(db->indexStream != NULL) {
		if(fclose(db->indexStream)) 
			errorCode = 721;
		db->indexStream = NULL;
	}

	// Free the node data structures
	for(i = 0; i < db->network->numNodes; i++) {
		if(nodes[i].demandProfile != NULL)
			free(nodes[i].demandProfile);
			free(nodes[i].nz);
	}
	free(db->nodes);
	db->nodes = NULL;

	// Free the link data structures
	for(i = 0; i < db->network->numLinks; i++) {
		free(links[i].vx);
		free(links[i].vy);
		free(links[i].nz);
	}
	free(db->links);
	db->links = NULL;

	// Free the network structure
	releaseNetInfo(db->network);

	free(db->directory);
	free(db->baseName);
	// Release the database
	free(db);
	lzma_enc_close();
	lzma_dec_close();
	return errorCode;
}

LIBEXPORT(int) ERD_getVersion(PERD database) {
	return database->header[1];
}
/*
 * Input: Database pointer, link velocity flag, link flow flag, node demand flag, demand profile flag
 * Output: If an error occurs, a non-zero error code value
 * Purpose: Set what hydraulics data are to be stored when ERD_writeHydResults is called.  
 * Flags are True (1) or False (0) and set values in the database prologue's header section.
 */
LIBEXPORT(int) ERD_setHydStorage(PERD db, int velocity, int flow, int demand, int pressure, int profile)
{
	db->header[5] = 0x00;
	if(velocity)
		SET_VELOCITY(db->header[5]);
	if(flow)
		SET_FLOW(db->header[5]);
	if(demand)
		SET_DEMAND(db->header[5]);
	if(pressure)
		SET_PRESSURE(db->header[5]);
	if(profile)
		SET_DEMANDPROFILE(db->header[5]);
	return FALSE;
}

/* ========================================================================= */
/* ERD API                                                                   */
/* Creating and Writing database files                                       */
/* ========================================================================= */

/*
 * Input: Database pointer
 * Output: If an error occurs, a non-zero error code value
 * Purpose: Add a new hydraulics file to a database.
 *          Open a new hydraulics file and set it for output in the ERD structure
 *          Add the filename to the ERD structure for use in writing index data
 */
LIBEXPORT(int) ERD_newHydFile(PERD db)
{
	char filename[MAXFNAME];
	
	// One new hydraulics file; increment File counter
	db->hydFileCount++;

	// Close existing hydraulic streams
	if(db->hydStream != NULL) 
		if(fclose(db->hydStream)) return 723;

	// Get a new hydraulics file name and open/store the stream
	getHydFilename(db, db->hydFileCount, filename, TRUE);
	db->hydStream = fopen(filename, "wb");
	if(db->hydStream == NULL) return 713;

	// The file index of the opened hydraulics file
	db->hydFileOpened = db->hydFileCount - 1;

	// Store the new file name in the database (without path)
	getHydFilename(db, db->hydFileCount, filename, FALSE);
	if(db->hydFilenames == NULL)
		MEMCHECK(db->hydFilenames = (char **)calloc(1, sizeof(char *)), "db->hydFilenames in ERD_newHydFile");
	else
		MEMCHECK(db->hydFilenames = (char **)realloc(db->hydFilenames, (db->hydFileCount) * sizeof(char *)), "db->hydFilenames in ERD_newHydFile");
	MEMCHECK(db->hydFilenames[db->hydFileOpened] = (char *)calloc(strlen(filename) + 1, sizeof(char)), "db->hydFilenames[db->hydFileCount] in ERD_newHydFile");
	strncpy(db->hydFilenames[db->hydFileOpened], filename, strlen(filename));
	
	return FALSE;
}
/*
 * Input: Database pointer
 * Ouptut: If an error occurs, a non-zero error code value
 * Purpose: Add a water quality results file to a hydraulics group in a database.
 *             Open a new water quailty output file and set it for output in the ERD structure
 *             Add the filename to the ERD structure for use in writing index data
 */
LIBEXPORT(int) ERD_newQualFile(PERD db) 
{
	char filename[MAXFNAME];
	
	// One new quality file; increment counter
	db->qualFileCount++;
	db->qualStreams=(FILE **)realloc(db->qualStreams,db->qualFileCount*sizeof(FILE *));

	// Get a new quality file name and open/store the stream
	getQualFilename(db, db->qualFileCount, filename, TRUE);
	db->qualStreams[db->qualFileCount-1] = fopen(filename, "wb");
	if(db->qualStreams[db->qualFileCount-1] == NULL) return 714;

	// Store the new file name in the database (without path)
	getQualFilename(db, db->qualFileCount, filename, FALSE);
	if(db->qualFilenames == NULL)
		MEMCHECK(db->qualFilenames = (char **)calloc(1, sizeof(char *)), "db->qualFilenames in ERD_newQualFile");
	else
		MEMCHECK(db->qualFilenames = (char **)realloc(db->qualFilenames, (db->qualFileCount) * sizeof(char *)), "db->qualFilenames in ERD_newQualFile");
	MEMCHECK(db->qualFilenames[db->qualFileCount - 1] = (char *)calloc(strlen(filename) + 1, sizeof(char)), "db->qualFilenames[db->qualFileCount] in ERD_newQualFile");
	strncpy(db->qualFilenames[db->qualFileCount - 1], filename, strlen(filename));

	return FALSE;
}
/*
 * Input: Database pointer
 * Output: If an error occurs, a non-zero error code value
 * Purpose: Write hydraulics simulation data to hydraulics data stream.  
 *          The database structure's NodeInfo and LinkInfo fields should contain 
 *          the necessary data -- they can be loaded by functions in the ENL API.
 *          Also updates the HydraulicsSim structure that stores
 *          the necessary information about the new simulation.
 */
LIBEXPORT(int) ERD_writeHydResults(PERD database)
{
	int	i, bw = 0, errorCode = 0;
	PNetInfo network = database->network;
	PHydData hyd = network->hydResults;
	PHydraulicsSim hydsim;

	// If we need to create a new hydraulics file, then do so
	// Otherwise write to current stream
	if(database->hydStream == NULL)
		errorCode = ERD_newHydFile(database);
	if(errorCode)
		return errorCode;

	// One new hydraulics simulation; increment counter
	database->hydSimCount++;

	// Allocate space for the new HydraulicsSim structure
	// reallocate space for the pointer, and then allocate space for the data pointed to.
	MEMCHECK(database->hydSim = (PHydraulicsSim *)realloc(database->hydSim, (database->hydSimCount) * sizeof(PHydraulicsSim)), "database->hydSim in ERD_writeHydResults");
	MEMCHECK(database->hydSim[database->hydSimCount - 1] = (PHydraulicsSim)calloc(1, sizeof(HydraulicsSim)), "database->hydSim[] in ERD_writeHydResults");
	hydsim = database->hydSim[database->hydSimCount - 1];
	
	// Before Writing: update the hydraulics simulation data for this group
	hydsim->offset = ERD_UTIL_getFilePosition(database->hydStream);
	hydsim->fileIdx = database->hydFileOpened;

	// write control link information, only for links that are controlled
	for(i = 0; i < network->numSteps; i++) {
		if((int)fwrite(hyd->linkStatus[i], sizeof(int), network->numControlLinks, database->hydStream) != network->numControlLinks)
			return 743;
		bw += sizeof(int)*network->numControlLinks;
	}

	if(ISSET_DEMAND(database->header[5])) {
		if(database->header[2]==lzma) {
			bw += (int)lzma_compress_float_2d(hyd->demand,network->numSteps,network->numNodes,database->hydStream);
		} else {
			for(i = 0; i < network->numSteps; i++) {
				if(fwrite(hyd->demand[i], sizeof(float), network->numNodes, database->hydStream) != network->numNodes)
					return 743;
				bw += sizeof(float)*network->numNodes;
			}
		}
		fflush(database->hydStream);
	}
	
	if(ISSET_PRESSURE(database->header[5])) {
		if(database->header[2]==lzma) {
			bw += lzma_compress_float_2d(hyd->pressure,network->numSteps,network->numNodes,database->hydStream);
		} else {
			for(i = 0; i < network->numSteps; i++) {
				if(fwrite(hyd->pressure[i], sizeof(float), network->numNodes, database->hydStream) != network->numNodes)
					return 743;
				bw += sizeof(float)*network->numNodes;
			}
		}
		fflush(database->hydStream);
	}
	
	if(ISSET_FLOW(database->header[5])) {
		if(database->header[2]==lzma) {
			bw += lzma_compress_float_2d(hyd->flow,network->numSteps,network->numLinks,database->hydStream);
		} else {
			for(i = 0; i < network->numSteps; i++) {
				if(fwrite(hyd->flow[i], sizeof(float), network->numLinks, database->hydStream) != network->numLinks)
					return 743;
				bw += sizeof(float)*network->numLinks;
			}
		}
		fflush(database->hydStream);
	}
	if(ISSET_VELOCITY(database->header[5])) {
		if(database->header[2]==lzma) {
			bw += lzma_compress_float_2d(hyd->velocity,network->numSteps,network->numLinks,database->hydStream);
		} else {
			for(i = 0; i < network->numSteps; i++) {
				if(fwrite(hyd->velocity[i], sizeof(float), network->numLinks, database->hydStream) != network->numLinks)
					return 743;
				bw += sizeof(float)*network->numLinks;
			}
		}
		fflush(database->hydStream);
	}
	if(ISSET_DEMANDPROFILE(database->header[5])) {
		int length;
		PNodeInfo nodes=database->nodes;
		for(i = 0; i < database->network->numNodes; i++) {
			length = nodes[i].demandProfileLength;
			if(fwrite(&length, sizeof(int), 1, database->hydStream) != 1)
				return 743;
			bw+=4;
			if(length>0) {
				if(database->header[2]==lzma) {
					bw += lzma_compress_float_1d(nodes[i].demandProfile,length,database->hydStream);
				} else {
					if(fwrite(nodes[i].demandProfile, sizeof(float), length, database->hydStream) != length)
						return 743;
					bw+=length*sizeof(float);
				}
			}
		}
	 	fflush(database->hydStream);
	}

	
	// After Writing: Update the hydraulics simulation data for this group
	hydsim->written = TRUE;
	hydsim->length = bw;

	// clear hydraulic data so we don't accumulate later on.
	for(i = 0; i < network->numSteps; i++) {
		memset(hyd->demand[i], 0, sizeof(float)*network->numNodes);
		memset(hyd->pressure[i], 0, sizeof(float)*network->numNodes);
		memset(hyd->flow[i], 0, sizeof(float)*network->numLinks);
		memset(hyd->velocity[i], 0, sizeof(float)*network->numLinks);
	}
	
	return FALSE;
}
/*
 * Input: Database pointer, application-specific index data pointer
 * Output: If an error occurs, a non-zero error code value
 * Purpose: Write water quality results to a database.  The database structure's NodeInfo field
 * should contain the necessary data -- it can be loaded by functions in the ENL API.
 */
LIBEXPORT(int) ERD_writeQualResults(PERD db, void *appdata)
{
	int errorCode = 0;
	unsigned int bw = 0;
	PQualitySim qualsim;
	FILE *qStream=NULL;

// Make room for the new QualitySim structure
	// Reallocate space for the pointer, then allocate space for the data pointed to
	MEMCHECK(db->qualSim = (PQualitySim *)realloc(db->qualSim, (db->qualSimCount + 1) * sizeof(PQualitySim)), "db->qualSim in ERD_writeQualResults");
	MEMCHECK(qualsim = db->qualSim[db->qualSimCount] = (PQualitySim)calloc(1, sizeof(QualitySim)), "qualsim in ERD_writeQualResults");

	// If we need to create a new quality file, then do so
	// Otherwise write to current stream
	if(db->qualStreams==NULL) {
		ERD_newQualFile(db);
	}
	qStream=db->qualStreams[db->qualFileCount - 1];
	if(errorCode)
		return errorCode;
	
	// Before Writing: update the quality simulation data for this group
	qualsim->offset = ERD_UTIL_getFilePosition(qStream);
	qualsim->fileIdx = db->qualFileCount - 1;
	qualsim->hydSimIdx = db->hydSimCount - 1;  // Assumed last hyd simulation written; could do better
	qualsim->appData = appdata;

	// Start writing the new quality simulation record
	if(fwrite(&(db->qualSimCount), sizeof(int), 1, qStream) != 1)
		return 744;
	bw += sizeof(int);
	if(IS_COMP_RLE(db))
		bw += compressAllAndStoreRLE(db,qStream);
	else if(IS_COMP_LZMA(db))
		bw += lzma_store_qual(db,qStream);
	else
		return 746;
	// After Writing: Update the quality simulation data for this group
	qualsim->written = TRUE;
	qualsim->length = bw;
	// Update the database counters
	db->qualSimCount++;
	
	return FALSE;
}
LIBEXPORT(int) ERD_clearQualityData(PERD db) 
{
	int s,
		nsteps = db->network->numSteps,
		nnodes = db->network->numNodes,
		nlinks = db->network->numLinks;
	PSpeciesData *species = db->network->species;
	float ***nC = db->network->qualResults->nodeC;
	float ***lC = db->network->qualResults->linkC;
	//clear the node and link data structures so we don't accumulate later on.
	for(s = 0; s < db->network->numSpecies; s++) {
		if(species[s]->index!=-1) {
			if( species[s]->type == bulk ) {
				int n;
				for(n = 0; n < nnodes; n++) {
					memset(nC[s][n], 0, sizeof(float)*nsteps);
				}
			} else {
				int n;
				for(n = 0; n < nlinks; n++) {
					memset(lC[s][n], 0, sizeof(float)*nsteps);
				}
			}
		}
	}
	return FALSE;
}

/* ========================================================================= */
/* ERD API                                                                   */
/* Retrieving information from the database                                  */
/* ========================================================================= */
/*
 * Input: Database pointer
 * Output: Count of water quality results
 * Purpose: Retrieve the total number of water quality results stored in a database.
 * The value can be used in a for loop with ERD_getResults.
 */
LIBEXPORT(int) ERD_getQualSimCount(PERD database)
{
	return database->qualSimCount;
}

/*
 * Input: Database pointer
 * Output: Count of hydraulics results
 * Purpose: Retrieve the number of hydraulics results (hydraulics groups) stored in
 * a database.
 */
LIBEXPORT(int) ERD_getHydSimCount(PERD database)
{
	return database->hydSimCount;
}

/*
 * Input: Database pointer
 * Output: Count of water quality results associated with a hydraulics group
 * Purpose: Retrieve the number of water quality results associated with a hydraulics group
 * stored in a database.  This value can be used in a for loop with ERD_getQualResults.
 */
LIBEXPORT(int) ERD_getQualSimCountFor(int hydSimIdx, PERD database)
{
	int count = 0;
	int i;
	
	for(i = 0; i < database->qualSimCount; i++) {
		if(database->qualSim[i]->hydSimIdx == hydSimIdx)
			count++;
	}
	
	return count;
}

/*
 * Input: Database pointer
 * Output: Network data structure pointer
 * Purpose: Retrieve data about the network stored in a database.
 */
LIBEXPORT(PNetInfo) ERD_getNetworkData(PERD database)
{
	return database->network;
}
/*
 * Input: Results index, database pointer, node data pointer, link data pointer
 * Output: If an error occurs, a non-zero error code value
 * Purpose: Retrieve simulation results in NodeInfo and LinkInfo data structures.
 */
LIBEXPORT(int) ERD_getResults(PQualitySim qualSim, PERD database)
{
	int err;
	PHydraulicsSim hydSim;

	hydSim = getHydSimIdx(qualSim, database);
	err=getHydResults(hydSim,database);
	if(err != 0)
		return err;
	err=getQualResults(qualSim, database);
	return err;
}

/*
 * Input: Result ID, database pointer, node data pointer, source data pointer, data buffer, buffer length
 * Output: If an error occurs, a non-zero error code value
 * Purpose: Retrieves compressed water quality data directly from results data file.
 */
LIBEXPORT(int) ERD_getRawSimulationResults(PQualitySim qualSim, PERD database, PSource sources, char *buffer, int length)
{
	if(qualSim == NULL) {
		return 705;
	}
	ensureQualityFileOpened(qualSim, database);

	if(ERD_UTIL_positionFile(database->qualStreams[qualSim->fileIdx],qualSim->offset)) {
	  return DB_UNABLE_TO_POSITION;
	}
	if(!currentHyd(database->hydSim[qualSim->hydSimIdx], database)) {
		if(getNLHydData(database->hydSim[qualSim->hydSimIdx], database))
			return 733;
	}
	loadSources(database->qualStreams[qualSim->fileIdx], sources, qualSim);
	if((unsigned int)length < qualSim->length) {
		return 734; 
	}
	fread(buffer, 1, qualSim->length, database->qualStreams[qualSim->fileIdx]);
	
	return FALSE;
}

/*
 * Input: Database pointer, ERD element index of a link (base 0)
 * Purpose: test for presence of controlled link in ERD data structure.
 * Returns: internal ERD controlLink index for given epanet index.  on error, -1
 */
LIBEXPORT(int) ERD_getERDcontrolLinkIndex(PERD db, int epanetIndex)
{
	int i;
	
	for(i=0; i<db->network->numControlLinks; i++) {
		if(db->network->controlLinks[i] == epanetIndex)
			return i;
	}
	return -1;
}

/*
 * Input: Database pointer, results index
 * Output: pointer to application-specific data that identifies result
 * Purpose: See Output
 */

LIBEXPORT(void*) ERD_getApplicationData(PERD db, int qualSimIdx)
{
	return db->qualSim[qualSimIdx]->appData;
}

/* ========================================================================= */
/* ERD API                                                                   */
/* Error Handling - see also ERDERROR macro definition                       */
/* ========================================================================= */
/*
 * Input: Error code integer value
 * Output: Character array containing error code text
 * Purpose: Return an error code in printable form
 */
LIBEXPORT(char*) ERD_getErrorMessage(int errorCode)
{
	char *value;
	
	switch(errorCode)
	{
		case 701:
			value = "Database not found in directory";
			break;
		case 702: 
			value = "Incorrect database format";
			break;
		case 703:
			value = "Bad directory name";
			break;
		case 704:
			value = "Database already closed";
			break;
		case 705:
			value = "Results not found for ID";
			break;
		case 706:
			value = "Temp file error";
			break;
		case 707:
			value = "No results found in database";
			break;
		case 711:
			value = "Error opening qual index file";
			break;
		case 712:
			value = "Error opening prologue file";
			break;
		case 713:
			value = "Error opening hydraulics results file";
			break;
		case 714:
			value = "Error opening water quality results file";
			break;
		case 721:
			value = "Error closing qual index file";
			break;
		case 722:
			value = "Error closing prologue file";
			break;
		case 723:
			value = "Error closing hydraulics results file";
			break;
		case 724:
			value = "Error closing water quality results file";
			break;
		case 726:
			value = "Error reading/writing application-specific index data";
			break;
		case 731:
			value = "Error reading qual index file";
			break;
		case 732:
			value = "Error reading prologue file";
			break;
		case 733:
			value = "Error reading hydraulics data file";
			break;
		case 734:
			value = "Error reading water quality data file";
			break;
		case 735:
			value = "Error retrieving network data";
			break;
		case 741:
			value = "Error writing qual index file";
			break;
		case 742:
			value = "Error writing prologue file";
			break;
		case 743:
			value = "Error writing hydraulics results file";
			break;
		case 744:
			value = "Error writing quality data file";
			break;
		case 745:
			value = "Invalid application type";
			break;
		case 746:
			value = "Invalid compression method";
			break;
		case 747:
			value = "Data requested that is not stored.";
			break;
		case 748:
			value = "No hydraulic data stored.";
			break;
		default:
			value = "This error value does not exist.\n";
			break;
	}

	return value;
}
/*
 * Input: Error code integer value
 * Output: None
 * Purpose: Prints the ERD error message string and exits.
 */
LIBEXPORT(void) ERD_Error(int errorCode)
{
	char *errorMessage;

	errorMessage = ERD_getErrorMessage(errorCode);
	fprintf(stderr, "ERD run-time error.\n"); 
	fprintf(stderr, "%s\n", errorMessage);
	exit(1);
}
LIBEXPORT(int) ERD_getCompressionLevel(PERD db)
{
	int level=db->header[2];
	return level;
}

LIBEXPORT(char *) ERD_GetCompressionDesc(PERD db)
{
	char *names[]={"None","Non-zero","RLE Non-zero","RLE","LZMA"};
	return names[db->header[2]];
}

LIBEXPORT(int) ERD_getSpeciesIndex(PERD db, const char *specie)
{
	int i;
	for(i=0;i<db->network->numSpecies;i++) {
		if(strcmp(db->network->species[i]->id,specie)==0) return i;
	}
	return -1;
}

LIBEXPORT(PHydData) ERD_UTIL_initHyd(PNetInfo network, int flags)
{
	int i;
	PHydData hyd;

	MEMCHECK(hyd = (PHydData)calloc(1, sizeof(HydData)), "hyd in initHyd");
	
	MEMCHECK(hyd->linkStatus = (int **)calloc(network->numSteps, sizeof(int*)), "hyd->linkStatus in initHyd");
	for(i=0; i<network->numSteps; i++) {
		MEMCHECK(hyd->linkStatus[i] = (int *)calloc(network->numControlLinks, sizeof(int)), "hyd->linkStatus[i] in initHyd");
	}
	if(flags & READ_LINKVEL) {
		MEMCHECK(hyd->velocity = (float **)calloc(network->numSteps, sizeof(float *)), "hyd->velocity in initHyd");
		for(i = 0; i < network->numSteps; i++) {
			MEMCHECK(hyd->velocity[i] = (float *)calloc(network->numLinks, sizeof(float)), "hyd->velocity[i] in initHyd");
		}
	}
	if(flags & READ_LINKFLOW) {
		MEMCHECK(hyd->flow = (float **)calloc(network->numSteps, sizeof(float *)), "hyd->flow in initHyd");
		for(i = 0; i < network->numSteps; i++) {
			MEMCHECK(hyd->flow[i] = (float *)calloc(network->numLinks, sizeof(float)), "hyd->flow[i] in initHyd");
		}
	}
	if(flags & READ_DEMANDS) {
		MEMCHECK(hyd->demand = (float **)calloc(network->numSteps, sizeof(float *)), "hyd->demand in initHyd");
		for(i = 0; i < network->numSteps; i++) {
			MEMCHECK(hyd->demand[i] = (float *)calloc(network->numNodes, sizeof(float)), "hyd->demand[i] in initHyd");
		}
	}
	MEMCHECK(hyd->pressure = (float **)calloc(network->numSteps, sizeof(float *)), "hyd->pressure in initHyd");
	for(i = 0; i < network->numSteps; i++) {
		MEMCHECK(hyd->pressure[i] = (float *)calloc(network->numNodes, sizeof(float)), "hyd->pressure[i] in initHyd");
	}
	return hyd;
}

LIBEXPORT(PQualData) ERD_UTIL_initQual(PNetInfo network, int flags)
{
	int s, n;
	int numNodes=network->numNodes;
	int numLinks=network->numLinks;
	int numSpecies=network->numSpecies;
	int numSteps=network->numSteps;
	PSpeciesData *species=network->species;
	PQualData qual;

	//Allocate QualData
	MEMCHECK(qual = (PQualData)calloc(1, sizeof(QualData)), "qual in getNetworkData");
	if(flags & READ_QUALITY) {
		MEMCHECK(qual->nodeC = (float ***)calloc(numSpecies, sizeof(float **)), "qual->nodeC in ENL_getNetworkData");
		MEMCHECK(qual->linkC = (float ***)calloc(numSpecies, sizeof(float **)), "qual->linkC in ENL_getNetworkData");
		for(s = 0; s < numSpecies; s++) {
			if(species[s]->index != -1) { // only allocate mem if we are going to save the species
				if( species[s]->type == bulk ) {
					MEMCHECK(qual->nodeC[s] = (float **)calloc(numNodes, sizeof(float *)), "qual->nodeC[s] in ENL_getNetworkData");
					MEMCHECK(qual->nodeC[s][0] = (float *)calloc(numSteps*numNodes, sizeof(float)), "qual->nodeC[s][0] in ENL_getNetworkData");
					for(n = 0; n < numNodes; n++) {
						//MEMCHECK(qual->nodeC[s][n] = (float *)calloc(numSteps, sizeof(float)), "qual->nodeC[s][n] in ENL_getNetworkData");
						qual->nodeC[s][n] = qual->nodeC[s][0] + n*numSteps;
					}
				} else {
					MEMCHECK(qual->linkC[s] = (float **)calloc(numLinks, sizeof(float *)), "qual->linkC[s] in ENL_getNetworkData");
					for(n = 0; n < numLinks; n++) {
						MEMCHECK(qual->linkC[s][n] = (float *)calloc(numSteps, sizeof(float)), "qual->linkC[s][n] in ENL_getNetworkData");
					}
				}
			}
		}
	}
	return qual;
}

LIBEXPORT(int) ERD_UTIL_positionFile(FILE *fp,__file_pos_t offs)
{
    fpos_t pos;
	memset(&pos,0,sizeof(fpos_t));
#if defined(__linux__)
	pos.__pos = (int64_t)offs;
#else
#if defined(WIN32) || defined(__APPLE__)
	pos = (fpos_t)offs;
#endif
#endif
	return fsetpos(fp,&pos);
}
LIBEXPORT(__file_pos_t) ERD_UTIL_getFilePosition(FILE *fp) {
    fpos_t pos;
	__file_pos_t offs;

	memset(&pos,0,sizeof(fpos_t));
	fgetpos(fp,&pos);
#if defined(__linux__)
	offs=pos.__pos;
#else
#if defined(WIN32) || defined(__APPLE__)
	offs=pos;
#endif
#endif
	return offs;
}
