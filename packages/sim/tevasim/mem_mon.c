#include <stdio.h>
#include <memory.h>
#include <windows.h>
#include <process.h>    /* _beginthread, _endthread */
#include <psapi.h>
#include <time.h>

static FILE *logFile=NULL;

int ensureLogFileExists(char *logFileName)
{
	if(logFile==NULL) {
		logFile = fopen(logFileName,"w");
		if(logFile==NULL) {
			fprintf(stderr,"Unable to create log file: %s\n",logFileName);
			fprintf(stderr,"memory logging will be disabled\n");
		}
	}
	return logFile==NULL;
}

void winMemoryLogger(void *v)
{
	clock_t t0=clock();
	clock_t t1;
	size_t maxWss=0,maxPws=0;
	size_t wss=0,pws=0;
	DWORD updateInterval=*((int*)v);
	char *fmtStr;
	free(v);
	if(sizeof(size_t)==8) {
		fmtStr="%ld\t%llu\t%llu\n";
	} else {
		fmtStr="%ld\t%u\t%u\n";
	}
	fprintf(logFile,"time\tWSS\tPeakWSS\n");
	while(1) {

		PROCESS_MEMORY_COUNTERS memstat;
		GetProcessMemoryInfo(GetCurrentProcess(),&memstat,sizeof(PROCESS_MEMORY_COUNTERS));
		pws=memstat.PeakWorkingSetSize;
		wss=memstat.WorkingSetSize;
		if(pws>maxPws) maxPws=pws;
		if(wss>maxWss) maxWss=wss;
		t1=clock();
		fprintf(logFile,fmtStr,(t1-t0),wss,pws);
		fflush(logFile);
		Sleep(updateInterval);
	}
}

void startMemoryMonitorThread(char *logFileName,int updateInterval)
{
	uintptr_t th;
	int *interval=(int *)calloc(1,sizeof(int));
	*interval=updateInterval;
	if(ensureLogFileExists(logFileName)==1) return;
	th=_beginthread(winMemoryLogger,0,interval);
}
