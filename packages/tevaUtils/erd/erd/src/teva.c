#include "erdinternal.h"

int readTEVAIndexData(PERD db, PQualitySim data, FILE *stream)
{
	int errorCode = 0,
		numSources = 0,
		s;
	PTEVAIndexData d;
	
	MEMCHECK(d = (PTEVAIndexData)calloc(1, sizeof(TEVAIndexData)), "d in readTEVAIndexData");
	if(fread(&numSources, sizeof(int), 1, stream) != 1)
		errorCode = 731;
	d->nsources = numSources;
	MEMCHECK(d->source = (PSource)calloc(numSources, sizeof(Source)), "d->source in readTEVAIndexData");
	for(s = 0; s < numSources; s++) {
		int val;
		PSource source = &d->source[s];
		if(fread(&source->sourceNodeIndex, sizeof(int), 1, stream) != 1)
			errorCode = 731;
		strcpy(source->sourceNodeID,db->nodes[source->sourceNodeIndex-1].id);
		if(fread(&source->speciesIndex, sizeof(int), 1, stream) != 1)
			errorCode = 731;
		if(fread(&source->sourceType, sizeof(int), 1, stream) != 1)
			errorCode = 731;
		if(fread(&source->sourceStrength, sizeof(float), 1, stream) != 1)
			errorCode = 731;
		if(fread(&val, sizeof(int), 1, stream) != 1)
			errorCode = 731;
		source->sourceStart=val;
		if(fread(&val, sizeof(int), 1, stream) != 1)
			errorCode = 731;
		source->sourceStop=val;
	}
	data->appData = d;
	
	return errorCode;
}

int writeTEVAIndexData(void *data, FILE *stream)
{
	PTEVAIndexData d = (PTEVAIndexData)data;
	int errorCode = 0,
		numSources = d->nsources,
		s;	
	PSource source = d->source;
	
	if(fwrite(&numSources, sizeof(int), 1, stream) != 1)
		errorCode = 741;
	
	for(s = 0; s < numSources; s++) {
		int val;
		if(fwrite(&source[s].sourceNodeIndex, sizeof(int), 1, stream) != 1)
			errorCode = 741;
		if(fwrite(&source[s].speciesIndex, sizeof(int), 1, stream) != 1)
			errorCode = 741;
		if(fwrite(&source[s].sourceType, sizeof(int), 1, stream) != 1)
			errorCode = 741;
		if(fwrite(&source[s].sourceStrength, sizeof(float), 1, stream) != 1)
			errorCode = 741;
		val=(int)source[s].sourceStart;
		if(fwrite(&val, sizeof(int), 1, stream) != 1)
			errorCode = 741;
		val=(int)source[s].sourceStop;
		if(fwrite(&val, sizeof(int), 1, stream) != 1)
			errorCode = 741;
	}

	return errorCode;
}

LIBEXPORT(PTEVAIndexData) newTEVAIndexData(int numSources, PSource source)
{
	PTEVAIndexData d;
	int s;
	
	MEMCHECK(d = (PTEVAIndexData)calloc(1, sizeof(TEVAIndexData)), "d in newTEVAIndexData");	
	MEMCHECK(d->source = (PSource)calloc(numSources, sizeof(Source)), "d->source in newTEVAIndexData");
	d->nsources = numSources;
	for(s = 0; s < numSources; s++) {
		PSource ss = &d->source[s];
		ss->sourceNodeIndex = source[s].sourceNodeIndex;
		ss->speciesIndex = source[s].speciesIndex;
		ss->sourceType = source[s].sourceType;
		ss->sourceStrength = source[s].sourceStrength;
		ss->sourceStart = source[s].sourceStart;
		ss->sourceStop = source[s].sourceStop;
		strncpy(ss->sourceNodeID, source[s].sourceNodeID, MAX_ID_LENGTH);
	}
	
	return d;
}

void freeTEVAIndexData(void **data)
{
	PTEVAIndexData d=(PTEVAIndexData)*data;
	free(d->source);
	free(d);
	*data=NULL;
}

