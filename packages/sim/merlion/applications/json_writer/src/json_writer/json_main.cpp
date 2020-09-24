#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <epanet2.h>

bool CheckForError(int nError, char *sMessage);
char *PrintENErrorMsg(int x);

int main(int argc, char *argv[])
{
	const char *newLine  = "";//"\n";
	if (argc == 1) {
		printf("{\"error\":\"Error: No arguments passed!\"}");
		return 1;
		argv[1] = "/Users/smcgee/Repos/wst_trunk/examples/tutorial/Net3.inp";
		newLine  = "\n";
	}
	//
	char *inp_file = argv[1];
	char *unix_rpt_file = "/dev/null";
	char *win_rpt_file = "nul";
        char *rpt_file = NULL;
        FILE* test_file = NULL;
        if ( (test_file = fopen(unix_rpt_file, "w")) != NULL )
        {
           rpt_file = unix_rpt_file;
           fclose(test_file);
        }
        else if ( (test_file = fopen(win_rpt_file, "w")) != NULL )
        {
           rpt_file = win_rpt_file;
           fclose(test_file);
        }
        else
        {
           printf("{\"error\":\"Error: unable to open scratch EPANET report file.\"}");
           return 1;
        }
              
	char *bin_file = "";
	int nError = ENopen(inp_file, rpt_file, bin_file);
	if (nError > 0) {
		char emsg[512];
		ENgeterror(nError, emsg, 512);
		printf("{\"error\":\"%s\"}", emsg);
		return 1;
	}
	//
	long time_duration;
	nError = ENgettimeparam(EN_DURATION, &time_duration);
	if (CheckForError(nError,"ENgettimeparam(EN_DURATION, &time_duration)")) return nError;
	long time_hydstep;
	nError = ENgettimeparam(EN_HYDSTEP, &time_hydstep);
	if (CheckForError(nError,"ENgettimeparam(EN_HYDSTEP, &time_hydstep)")) return nError;
	long time_qualstep;
	nError = ENgettimeparam(EN_QUALSTEP, &time_qualstep);
	if (CheckForError(nError,"ENgettimeparam(EN_QUALSTEP, &time_qualstep)")) return nError;
	long time_reportstep;
	nError = ENgettimeparam(EN_REPORTSTEP, &time_reportstep);
	if (CheckForError(nError,"ENgettimeparam(EN_REPORTSTEP, &time_reportstep)")) return nError;
	long time_reportstart;
	nError = ENgettimeparam(EN_REPORTSTART, &time_reportstart);
	if (CheckForError(nError,"ENgettimeparam(EN_REPORTSTART, &time_reportstart)")) return nError;
	long time_statistic;
	nError = ENgettimeparam(EN_STATISTIC, &time_statistic);
	if (CheckForError(nError,"ENgettimeparam(EN_STATISTIC, &time_statistic)")) return nError;
	long time_periods;
	nError = ENgettimeparam(EN_PERIODS, &time_periods);
	if (CheckForError(nError,"ENgettimeparam(EN_PERIODS, &time_periods)")) return nError;
	//
	printf("{%s", newLine);
	printf("\"TimeData\":");
	printf("{");
	printf("\"Duration\":%ld",time_duration);
	printf(",");
	printf("\"HydraulicStep\":%ld",time_hydstep);
	printf(",");
	printf("\"WaterQualityStep\":%ld",time_qualstep);
	printf(",");
	printf("\"ReportStep\":%ld",time_reportstep);
	printf(",");
	printf("\"ReportStart\":%ld",time_reportstart);
	printf(",");
	printf("\"Statistic\":%ld",time_statistic);
	printf(",");
	printf("\"Periods\":%ld",time_periods);
	printf("}");
	printf(",");
	int n_nodes = -1;
	nError = ENgetcount(EN_NODECOUNT, &n_nodes);
	if (CheckForError(nError,"ENgetcount(EN_NODECOUNT, &n_nodes)")) return nError;
	//
	if (true) {
		printf("\"SimList\":[");
		//printf("\"ALL\",");
		//printf("\"NZD\",");
		for (int i = 0; i < n_nodes; i++) {
			char sName[64];
			nError = ENgetnodeid(i+1, sName);
			if (CheckForError(nError,"ENgetnodeid(i+1, sName)")) return nError;
			if (i > 0) printf(",");
			printf("\"%s\"",sName);
		}
		printf("],");
	}
	printf("\"Nodes\":{");
	bool bFirstNode = true;
	for (int i = 0; i < n_nodes; i++) {
		char sName[64];
		int nType;
		//
		// get the name of the node
		nError = ENgetnodeid(i+1, sName);
		if (CheckForError(nError,"ENgetnodeid(i+1, sName)")) return nError;
		// get the type code of the node
		nError = ENgetnodetype(i+1, &nType);
		if (CheckForError(nError,"ENgetnodetype(i+1, &nType)")) return nError;
		//
		const char *sType = "";
		switch (nType) {
			case 0:
				sType = "Junction";
				break;
			case 1:
				sType = "Reservoir";
				break;
			case 2:
				sType = "Tank";
				break;
			default:
				sType = "Error";
		}
		if (!bFirstNode) printf(",");
		bFirstNode = false;
		printf("\"%s\":{\"Type\":\"%s\"}%s", sName, sType, newLine);
	}
	printf("},%s", newLine);
	printf("\"Links\":[%s", newLine);
	//
	int n_links = -1;
	nError = ENgetcount(EN_LINKCOUNT, &n_links);
	if (CheckForError(nError,"ENgetcount(EN_LINKCOUNT, &n_links)")) return nError;
	bool bFirstLink = true;
	for (int i=0; i < n_links; i++) {
		char sName[64];
		char sFrom[64];
		char sTo  [64];
		int nType, nFrom, nTo;
		//
		// get the index of the nodes this links to (FROM & TO)
		nError = ENgetlinknodes(i+1, &nFrom, &nTo);
		if (CheckForError(nError,"ENgetlinknodes(i+1, &nFrom, &nTo)")) return nError;
		//
		// get the id (name or label) corresponding to the "FROM" index
		nError = ENgetnodeid(nFrom, sFrom);
		if (CheckForError(nError,"ENgetnodeid(nFrom, sFrom)")) return nError;
		//
		// get the id (name or label) corresponding to the "TO" index
		nError = ENgetnodeid(nTo, sTo);
		if (CheckForError(nError,"ENgetnodeid(nTo, sTo)")) return nError;
		//
		// get link id
		nError = ENgetlinkid(i+1, sName);
		if (CheckForError(nError,"ENgetlinkid(i+1, sName)")) return nError;
		//
		// get link type
		nError = ENgetlinktype(i+1, &nType);
		if (CheckForError(nError,"ENgetlinktype(i+1, &nType)")) return nError;
		//
		const char *sType = "";
		switch (nType) {
			case 0:
			case 1:
				sType = "Pipe";
				break;
			case 2:
				sType = "Pump";
				break;
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				sType = "Valve";
				break;
			default:
				sType = "Error";
		}
		if (!bFirstLink) printf(",");
		bFirstLink = false;
		printf("{\"Type\":\"%s\",\"ID\":\"%s\",\"Node1\":\"%s\",\"Node2\":\"%s\"}%s", sType, sName, sFrom, sTo, newLine);
	}
	printf("]%s", newLine);
	//
	printf("}%s", newLine);
	//
	return 0;
}

bool CheckForError(int nError, char *sMessage) {
	if (nError > 0) {
		printf("\n\nError #%d: %s\n\n", nError, sMessage);
		return true;
	}
	return false;
}
