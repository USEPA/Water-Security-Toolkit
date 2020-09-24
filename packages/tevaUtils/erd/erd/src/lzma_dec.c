#ifdef WIN32
#include <io.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "erd.h"
#include "lzma_dec.h"


static void * AllocForLzma(void *p, size_t size) { p=p; return malloc(size); }
static void FreeForLzma(void *p, void *address) { p=p; free(address); }
static ISzAlloc SzAllocForLzma = { &AllocForLzma, &FreeForLzma };

static unsigned char *inputBuffer=NULL;
static size_t inputBufferSize=0;
int lzma_decode(CLzmaDec *dec, unsigned char *inBuf, size_t cSize, size_t iPos, unsigned char *destBuf, size_t bufLen);

static unsigned char *getInputBuffer(size_t size)
{
	if(size < inputBufferSize)
		return inputBuffer;
	if(inputBuffer==NULL) {
		inputBuffer=(unsigned char *)calloc(size,sizeof(unsigned char));
	} else {
		inputBuffer=(unsigned char *)realloc(inputBuffer,size*sizeof(unsigned char));
	}
	inputBufferSize=size;
	return inputBuffer;
}

void lzma_dec_close()
{
	if(inputBuffer != NULL) {
		free(inputBuffer);
		inputBuffer=NULL;
		inputBufferSize=0;
	}
}
int readAndDecompressLZMA(FILE *stream, PERD db)
{
	int nnz, s,
		nnodes = db->network->numNodes,
		nlinks = db->network->numLinks,
		nsteps = db->network->numSteps,
		nspecies = db->network->numSpecies;
	PSpeciesData *species = db->network->species;
	PNodeInfo nodes = db->nodes;
	PLinkInfo links = db->links;
	PQualData qual = db->network->qualResults;
	float ***lC = qual->linkC;
	float ***nC = qual->nodeC;
	int i;
	unsigned char *inBuf;

	for(s=0;s<nspecies;s++) {
		if(species[s]->index!=-1) {
			unsigned bufLen=db->network->numSteps*sizeof(float);
			unsigned cSize;
			unsigned iPos=0;
			int inode;
			CLzmaDec dec;
			if( species[s]->type == bulk ) {
				for(inode = 0; inode < nnodes; inode++) {
					nodes[inode].nz[s] = 0;
					memset(nC[s][inode], 0, sizeof(float) * nsteps);
				}
			} else {
				for(inode = 0; inode < nlinks; inode++) {
					links[inode].nz[s] = 0;
					memset(lC[s][inode], 0, sizeof(float) * nsteps);
				}
			}

			fread(&nnz,sizeof(int),1,stream);
			if(nnz) {
				fread(&cSize,sizeof(int),1,stream);
				inBuf=getInputBuffer(cSize);
				fread(inBuf,sizeof(unsigned char),cSize,stream);
				LzmaDec_Construct(&dec);
				LzmaDec_Allocate(&dec,&inBuf[0],LZMA_PROPS_SIZE,&SzAllocForLzma);
				LzmaDec_Init(&dec);
				iPos=LZMA_PROPS_SIZE;


				for(i=0;i<nnz;i++) {
					int id;
					unsigned char *destBuffer;

					iPos=lzma_decode(&dec,inBuf,cSize,iPos,(unsigned char *)&id,sizeof(int));
					// get the node id

					id=id-1; // node id

					destBuffer = db->network->species[s]->type==bulk ? (unsigned char *)nC[s][id] : (unsigned char *)lC[s][id];
					if(db->network->species[s]->type==bulk) {
						nodes[id].nz[s]=1;
					} else {
						links[id].nz[s]=1;
					}
					iPos=lzma_decode(&dec,inBuf,cSize,iPos,destBuffer,bufLen);

				}
				LzmaDec_Free(&dec,&SzAllocForLzma);
			}
		}
	}
	return FALSE;
}

int lzma_decode(CLzmaDec *dec, unsigned char *inBuf, size_t cSize, size_t iPos, unsigned char *destBuf, size_t bufLen)
{
	SRes res;
	ELzmaStatus status;
	size_t destLen,srcLen;
	size_t tPos=0;
	while(tPos < bufLen) {
		destLen=bufLen-tPos;
		srcLen=cSize-iPos;
		res=LzmaDec_DecodeToBuf(dec,&destBuf[tPos],&destLen,&inBuf[iPos],&srcLen,
			LZMA_FINISH_ANY, &status);
		tPos+=destLen;
		iPos+=srcLen;
	}
	return (int)iPos;
}

int lzma_decompress_float_2d(float **f, int nrows, int ncols, FILE *stream)
{
	unsigned bufLen=ncols*sizeof(float);
	unsigned cSize;
	unsigned iPos=0;
	int r;
	unsigned char *inBuf;
	CLzmaDec dec;

	fread(&cSize,sizeof(int),1,stream);
	inBuf=getInputBuffer(cSize);
	fread(inBuf,sizeof(unsigned char),cSize,stream);
	LzmaDec_Construct(&dec);
	LzmaDec_Allocate(&dec,&inBuf[0],LZMA_PROPS_SIZE,&SzAllocForLzma);
	LzmaDec_Init(&dec);
	iPos=LZMA_PROPS_SIZE;

	for(r=0;r<nrows;r++) {
		unsigned char *destBuffer;
		destBuffer = (unsigned char *)f[r];
		iPos=lzma_decode(&dec,inBuf,cSize,iPos,destBuffer,bufLen);
	}
	LzmaDec_Free(&dec,&SzAllocForLzma);
	return 0;
}
int lzma_decompress_float_1d(float *f, int num, FILE *stream)
{
	unsigned bufLen=num*sizeof(float);
	unsigned cSize;
	unsigned iPos=0;
	unsigned char *inBuf;
	unsigned char *destBuffer;
	CLzmaDec dec;

	fread(&cSize,sizeof(int),1,stream);
	inBuf=getInputBuffer(cSize);
	fread(inBuf,sizeof(unsigned char),cSize,stream);
	LzmaDec_Construct(&dec);
	LzmaDec_Allocate(&dec,&inBuf[0],LZMA_PROPS_SIZE,&SzAllocForLzma);
	LzmaDec_Init(&dec);
	iPos=LZMA_PROPS_SIZE;

	destBuffer = (unsigned char *)f;
	iPos=lzma_decode(&dec,inBuf,cSize,iPos,destBuffer,bufLen);

	LzmaDec_Free(&dec,&SzAllocForLzma);
	return 0;
}
