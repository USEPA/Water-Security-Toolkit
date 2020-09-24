/*

	erd2scada
	
	program reads information from an ERD and sends to a SCADA database
	according to the CANARY xml configuration file.

*/



#include "erd2scada.h"
#include <erd.h>
#include <unistd.h>
#include <mysql.h>


int incrementDATETIME(MYSQL_TIME *myTS, float stepSize);
int runERD2SCADA(PERD myERD,pSCADAinfo mySCADAinfo, int runFast);


int main(int argc, char** argv) {
	
	if(argc < 3 || argc > 4) {
		fprintf(stderr,"\n:: Usage: %s <config-file.xml> <erd-directory/> [fast]\n\n",argv[0]);
		return 1;
	}
	
	char* xmlConfigFile = argv[1];
	char* erdDirectory = argv[2];
	int runFast = 0;
	
	if(argc == 4 && (strcmp(argv[3],"fast") == 0))
		runFast = 1;
	
	// get SCADA info from canary xml config file
	pSCADAinfo mySCADAinfo = initializeSCADAinfo(xmlConfigFile);
	
	// get ERD data from the database
	PERD myERD;
	ERD_open(&myERD, erdDirectory);
	
	// run
	runERD2SCADA(myERD, mySCADAinfo, runFast);
	
	fprintf(stdout,"Done.\n");
	
	return 0;
}







int runERD2SCADA(PERD myERD,pSCADAinfo mySCADAinfo, int runFast) {
	
	int i = 0;
	int timeStep = 0;
	int tagIndex = 0;
	int h = 0;  			// hyd group to send to scada.  default to zero.	
	int erdIndex = 0;	
	unsigned long totalWritten = 0;
	float 	myInsertValue;
	int errorCode = 0;
	int controlLinkIndex = 0;
	
	PHydData myHydResults;
	PLinkInfo tlinks = *myERD->links;
	PNodeInfo tnodes = *myERD->nodes;
	
	signalTypes mySignal;
	
	MYSQL *conn;
	MYSQL_TIME myTS;
	MYSQL_BIND bind[1];
	MYSQL_STMT *stmt;
	
	my_ulonglong affectedRows = 0;
	
	
	// server specific stuff - consider pulling from canary file
	char *server = "localhost";
  	char *user = "scada_user";
	char *password = "scada_pass";
	char *database = "scada_test";
	
	char *queryString;
	
	queryString = calloc(MAXQUERY,sizeof(char));
	
	bind[0].buffer_type = MYSQL_TYPE_DATETIME;
	bind[0].buffer = (char*)&myTS;
	bind[0].is_null = 0;
	bind[0].length = 0;
	
	
	// set the start datetime...  maybe pull this from the xml file as well?
	// for now, this is Unix Epoch.
	myTS.year = 1970;
	myTS.month = 01;
	myTS.day = 01;
	myTS.hour = 00;
	myTS.minute = 00;
	myTS.second = 00;
	
	
	// initialize the mysql connection
	conn = mysql_init(NULL);
	
	/* Connect to database */
	if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(0);
	}
	
	if(ERD_getHydGroupCount(myERD) < 1) {
		fprintf(stderr,": Error, no hydraulic groups present\n");
		return 1;
	}
	
	// fetch hydraulic results from ERD and store in a data structure
	errorCode = ERD_getHydResults(h,myERD,&myHydResults);
	if(errorCode)
		fprintf(stdout,": error %i retrieving hyd results\n",errorCode);
	
	// for every time step in the stored ERD...
	for(timeStep = 0; timeStep < myERD->network->numSteps; timeStep++) {
	
		// let's poll the SCADA information structure, just like a real system would poll the devices.
		// if a device measures something we want, let's retreive that value from the ERD at the current
		// timeStep, convert node/link indices to scada tags, and send measurements to scada.
		
		for(tagIndex = 0; tagIndex < mySCADAinfo->numTags; tagIndex++) {
			mySignal = mySCADAinfo->tags[tagIndex]->signalType;
			
			if(mySignal == FLOW || mySignal == PRESSURE || mySignal == VALVESTAT) {
				// get erd index for this device from the canary file...
				// we don't care if it's a link or a node.  we'll figure that out later.
				// e.g., flow=link, pressure=node.
				
				erdIndex = -1;
				
				if(mySignal == PRESSURE) {
					// get ERD node index by ID name
					for(i=0; i<myERD->network->numNodes; i++) {
						if(strcmp(tnodes[i].id, mySCADAinfo->tags[tagIndex]->epanetID) == 0) {
							erdIndex = i;
						}
					}
				}
				
				if(mySignal == FLOW || mySignal == VALVESTAT) {
					// get ERD link index by ID name
					for(i=0; i<myERD->network->numLinks; i++) {
						if(strcmp(tlinks[i].id, mySCADAinfo->tags[tagIndex]->epanetID) == 0) {
							erdIndex = i;
						}
					}
				}
				
				// sanity check
				if(erdIndex < 0) {
					fprintf(stderr,":: ERROR: could not find EPANET ID specified\n");
					exit(0);
				}
				
				
				if(mySignal == FLOW) {
					myInsertValue = myHydResults->flow[timeStep][erdIndex];
				}
				else if(mySignal == PRESSURE) {
					myInsertValue = myHydResults->pressure[timeStep][erdIndex];
				}
				else if(mySignal == VALVESTAT) {
					controlLinkIndex = ERD_getERDcontrolLinkIndex(myERD, erdIndex);
					myInsertValue = (float)myHydResults->linkStatus[timeStep][controlLinkIndex];
				}
				
				fprintf(stdout,":: signal %s: %f\n",mySCADAinfo->tags[tagIndex]->scadaID,myInsertValue);
				
				// prepare the query string with "?" character to be replaced with timedate.
				sprintf(queryString,"INSERT INTO scada_events (scada_tag,scada_value,time) VALUES ('%s','%f',?)",mySCADAinfo->tags[tagIndex]->scadaID,myInsertValue);
				// prepare the mysql statement for this insert.
				stmt = mysql_stmt_init(conn);
				if(!stmt) {
					fprintf(stderr,"error, mysql_stmt_init(), out of memory\n");
					exit(0);
				}
				
				if(mysql_stmt_prepare(stmt, queryString, strlen(queryString))) {
					fprintf(stderr,"\n mysql_stmt_prepare(), INSERT failed");
					fprintf(stderr, "\n %s\n", mysql_stmt_error(stmt));
					exit(0);
				}
				
				
				
				// bind the timedate to the mysql statement
				mysql_stmt_bind_param(stmt, bind);
				
				// finally, execute the statement.  this inserts the data into the sql db.
				mysql_stmt_execute(stmt);
				
				// let's fetch the number of inserts, just for sanity's sake
				affectedRows = mysql_stmt_affected_rows(stmt);
				if((unsigned long)affectedRows < 1)
					fprintf(stderr,": error, could not write to sql...\n");
				// and accumulate for reporting purposes.
				totalWritten += (unsigned long)affectedRows;
			
			
			
			}
		}
		
	// let's show and tell what we've just done...
	fprintf(stdout,": sent %lu measurements at timestep %i\n",totalWritten,timeStep);
	totalWritten = 0;
	
	// increment the current time by the step size.
	incrementDATETIME(&myTS, myERD->network->stepSize);
	
	// and wait for the actual time step to elapse...
	if(!runFast)
		sleep((unsigned int) (myERD->network->stepSize * 3600));
	
	} 
	
	/* Release memory used to store results and close connection */
	mysql_close(conn);
	
	return 0;
}





int incrementDATETIME(MYSQL_TIME *myTS, float stepSize) {
	
	
	int stepLen = (int)(3600 * stepSize);
	
	time_t *ttDate = malloc(sizeof(time_t));
	
	
	// stick to UTC time here, so as not to confuse time zones / DST
	struct tm *a_tm_struct = gmtime(ttDate);
	
	a_tm_struct->tm_year = myTS->year - 1900;
	a_tm_struct->tm_mon = myTS->month - 1;
	a_tm_struct->tm_mday = myTS->day;
	a_tm_struct->tm_hour = myTS->hour;
	a_tm_struct->tm_min = myTS->minute;
	a_tm_struct->tm_sec = myTS->second + stepLen;
	*ttDate = timegm(a_tm_struct);
	
	myTS->year = a_tm_struct->tm_year + 1900;
	myTS->month = a_tm_struct->tm_mon + 1;
	myTS->day = a_tm_struct->tm_mday;
	myTS->hour = a_tm_struct->tm_hour;
	myTS->minute = a_tm_struct->tm_min;
	myTS->second = a_tm_struct->tm_sec;
	
	free(ttDate);
	return 0;
}













