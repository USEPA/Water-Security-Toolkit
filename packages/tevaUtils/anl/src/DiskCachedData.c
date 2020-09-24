#include "DiskCachedData.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "loggingUtils.h"

static __file_pos_t x_getFilePosition(FILE *fp);
static void x_positionFile(FILE *fp,__file_pos_t offs);
int sortCachedDataAsc(const void *a, const void *b);

int sortCachedDataAsc(const void *a, const void *b)
{
	float f=(*((CachedDataPtr*)a))->val - (*((CachedDataPtr*)b))->val;
	return (f<0?-1:(f>0?1:0));
}

void x_positionFile(FILE *fp,__file_pos_t offs)
{
    fpos_t pos;
        int rv;
        memset(&pos,0,sizeof(fpos_t));
#ifdef __linux__
        pos.__pos = (int64_t)offs;
#else
#ifdef WIN32
        pos = (fpos_t)offs;
#endif
#endif
        rv=fsetpos(fp,&pos);
        if(rv!=0)
                printf("%d: %s\n",errno,strerror(errno));
}
__file_pos_t x_getFilePosition(FILE *fp)
{
    __file_pos_t fpos;
    fpos_t pos;
    memset(&pos,0,sizeof(fpos_t));
    fgetpos(fp,&pos);
#ifdef __linux__
    fpos= pos.__pos;
#else
#ifdef WIN32
    fpos = pos;
#endif
#endif
    return fpos;
}

static void *x_calloc(JNIEnv * env,size_t num, size_t s,char *id) {
	void *p=calloc(num,s);
	if(p==NULL) {
		char t_msg[256];
		sprintf(t_msg,"Unable to allocate %d bytes for %s.  errno=%d\nerror=%s",
			s,id,errno,strerror(errno));
		ANL_UTIL_LogSevere(env,"teva.analysis.server",t_msg);
	}
	return p;
}


LIBEXPORT(DiskCachedDataPtr) initDiskCachedData(JNIEnv * env,char *fn, int nelems,int numScenarios)
{
	int i;
	DiskCachedDataPtr p = (DiskCachedDataPtr)x_calloc(env,1,sizeof(DiskCachedData),"p (initDiskCachedData)");
	strcpy(p->fn,fn);
	p->fp=fopen(fn,"w+b");
	p->num=0;
	p->size=nelems;
	p->data=(CachedDataPtr*)x_calloc(env,numScenarios,sizeof(CachedDataPtr),"dcd->data");
	for(i=0;i<numScenarios;i++) {
	  p->data[i] = (CachedDataPtr)x_calloc(env,1,sizeof(CachedData),"dcd->data[dcd->num]");
	}
	return p;
}

LIBEXPORT(void) freeDiskCachedData(JNIEnv *env,DiskCachedDataPtr *dcd, int numScenarios)
{
	char logMsg[1024];
	struct stat stats;
	int i;

	fstat(fileno((*dcd)->fp),&stats);
	fclose((*dcd)->fp);

	sprintf(logMsg,"Cache file: %s size: %ld\n",(*dcd)->fn,stats.st_size);
	ANL_UTIL_LogInfo(env,"teva.analysis.server",logMsg);
	if(unlink((*dcd)->fn)==-1) {
		sprintf(logMsg,"Unable to remove cache file: %s: (%d) %s\n",(*dcd)->fn,errno,strerror(errno));
		ANL_UTIL_LogSevere(env,"teva.analysis.server",logMsg);
	}
	for(i=0;i<numScenarios;i++) {
		free((*dcd)->data[i]->injDef);
		free((*dcd)->data[i]->simID);
		free((*dcd)->data[i]);
	}
	free((*dcd)->data);
	free(*dcd);
	*dcd=NULL;
}

LIBEXPORT(void) cacheData(JNIEnv *env, DiskCachedDataPtr dcd, float val, char *simID, char *injDef, void *data, int size,char *type)
{
	char logMsg[1024];
	sprintf(logMsg,"%s dcd: 0x%08x",type,dcd);
	ANL_UTIL_LogFine(env,"teva.analysis.server",logMsg);
	sprintf(logMsg,"%s dcd->data: 0x%08x dcd->num: %5d",type,dcd->data,dcd->num);
	ANL_UTIL_LogFine(env,"teva.analysis.server",logMsg);
	sprintf(logMsg,"%s dcd->data[dcd->num]",type,dcd->data[dcd->num]);
	ANL_UTIL_LogFine(env,"teva.analysis.server",logMsg);

	dcd->data[dcd->num]->val = val;
	dcd->data[dcd->num]->simID = (char *)calloc(strlen(simID)+1,sizeof(char));
	strcpy(dcd->data[dcd->num]->simID,simID);
	dcd->data[dcd->num]->injDef = (char *)calloc(strlen(injDef)+1,sizeof(char));
	strcpy(dcd->data[dcd->num]->injDef,injDef);
	dcd->data[dcd->num]->fileOffset = x_getFilePosition(dcd->fp);
	dcd->data[dcd->num]->fileOffset = ftell(dcd->fp);
	dcd->num++;
	fwrite(data,size,1,dcd->fp);
	fflush(dcd->fp);

	sprintf(logMsg,"wrote %ld bytes so far to the cache file",x_getFilePosition(dcd->fp));
	ANL_UTIL_LogFine(env,"teva.analysis.server",logMsg);
}
LIBEXPORT(void) getCachedData(DiskCachedDataPtr dcd, int i,void *data, int size)
{
        __file_pos_t curFilePos = x_getFilePosition(dcd->fp);
        x_positionFile(dcd->fp,dcd->data[i]->fileOffset);
        fread(data,size,1,dcd->fp);
        x_positionFile(dcd->fp,curFilePos);
}


LIBEXPORT(void) SortCachedData(DiskCachedDataPtr dcd)
{
	qsort(dcd->data,dcd->num,sizeof(DiskCachedDataPtr),sortCachedDataAsc);
}
