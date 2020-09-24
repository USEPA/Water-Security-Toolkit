#ifdef WIN32
#include <io.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "lzma_enc.h"
#include <time.h>

static PLZMAOutStream getLZMAOutStream();
static PQualInStream getQualInStream();
static PFloat2DInStream getFloat2DInStream();
static void * AllocForLzma(void *p, size_t size) { p=p; return calloc(size,1); }
static void FreeForLzma(void *p, void *address) { p=p; free(address); }
static ISzAlloc SzAllocForLzma = { &AllocForLzma, &FreeForLzma };

size_t LZMAOutStream_Write(void *p, const void *buf, size_t size);
SRes QualInStream_Read(void *p, void *buf, size_t *size);
SRes Float2DInStream_Read(void *p, void *buf, size_t *size);

static PLZMAOutStream lzmaOut=NULL;
static PQualInStream qualIn=NULL;
static PFloat2DInStream float2dIn=NULL;
static CLzmaEncProps *props=NULL;
static CLzmaEncHandle encoder=NULL;

#ifdef LZMA_TESTING
static int fileNum=1;
static FILE *fp_raw=NULL;
static FILE *fp=NULL;

void ensureRawFileOpen() {
	char fn[256];
	if(fp_raw != NULL) return;
	sprintf(fn,"rawFiles\\file_%04d.raw",fileNum);
	fileNum++;
	fp_raw=fopen(fn,"wb");
}
void ensureCompFileOpen(CLzmaEncProps *props) {
	char fn[256];
	CLzmaEncProps p;
	int dictSize=0;
	int d;
	if(fp!= NULL) return;
	
	p=*props;
	LzmaEncProps_Normalize(&p);
	d=p.dictSize;
	dictSize=0;
	while(d !=1) {
		d=d>>1;
		dictSize++;
	}
	sprintf(fn,"comp-l%d-d%d-lc%d-lp%d-pb%d-a%d-fb%d-bt%d_%d-mc%d.txt",
		p.level,dictSize,p.lc,p.lp,p.pb,p.algo,p.fb,p.btMode,p.numHashBytes,p.mc);
	fp=fopen(fn,"w");
	fprintf(fp,"%s\n",fn);
}
void closeCompFile() {
	fclose(fp);
}
#endif

static CLzmaEncHandle getEncoder() {
	if(encoder==NULL) {
		encoder = LzmaEnc_Create(&SzAllocForLzma);
	}
	return encoder;
}

void setProps(CLzmaEncProps *p) {
	if(props==NULL) {
		props=(CLzmaEncProps*)calloc(1,sizeof(CLzmaEncProps));
	}
	memcpy(props,p,sizeof(CLzmaEncProps));
}

static CLzmaEncProps *getProps() {
	if(props==NULL) {
		props=(CLzmaEncProps*)calloc(1,sizeof(CLzmaEncProps));
		LzmaEncProps_Init(props);

		props->level=0;
		props->lp=2;
		props->lc=0;
		props->btMode=1;
		props->fb=256;
		props->algo=0;
	}
	return props;
}
static PLZMAOutStream getLZMAOutStream()
{
	if(lzmaOut==NULL) {
		lzmaOut=(PLZMAOutStream)calloc(1,sizeof(LZMAOutStream));
		lzmaOut->buf=NULL;
		lzmaOut->curPos=0;
		lzmaOut->size=0;
		lzmaOut->SeqOutStream.Write=&LZMAOutStream_Write;
	}
	return lzmaOut;
}
static PQualInStream getQualInStream()
{
	if(qualIn==NULL) {
		qualIn=(PQualInStream)calloc(1,sizeof(QualInStream));
		qualIn->curN=0;
		qualIn->p=NULL;
		qualIn->remain=0;
		qualIn->curID=0;
		qualIn->state=writingInit;
		qualIn->SeqInStream.Read=&QualInStream_Read;
	}
	return qualIn;
}
static PFloat2DInStream getFloat2DInStream()
{
	if(float2dIn==NULL) {
		float2dIn=(PFloat2DInStream)calloc(1,sizeof(Float2DInStream));
		float2dIn->r=0;
		float2dIn->p=NULL;
		float2dIn->remain=0;
		float2dIn->SeqInStream.Read=&Float2DInStream_Read;
	}
	return float2dIn;
}
void lzma_enc_close()
{
	if(qualIn) {
		free(qualIn);
		qualIn=NULL;
	}
	if(float2dIn) {
		free(float2dIn);
		float2dIn=NULL;
	}
	if(lzmaOut) {
		if(lzmaOut->buf) free(lzmaOut->buf);
		free(lzmaOut);
		lzmaOut=NULL;
	}
	if(props) {
		free(props);
		props=NULL;
	}
	if(encoder) {
		LzmaEnc_Destroy(encoder, &SzAllocForLzma, &SzAllocForLzma);
		encoder=NULL;
	}
}
void QualInStream_advanceToNextRecord(PQualInStream cis) {
	cis->p=NULL;
	cis->remain=0;
	cis->curN++;
	while(cis->state!=writingID && cis->curN < cis->nelems) {
		if(cis->curN < cis->nelems && cis->nz[cis->curN]) {
			cis->state=writingID;
			cis->curID=cis->curN+1;
			cis->p=(unsigned char *)&(cis->curID);
			cis->remain=sizeof(int);
			cis->state=writingID;
		} else {
			cis->curN++;
		}
	}
}
SRes QualInStream_Read(void *p, void *buf, size_t *size)
{
	QualInStream *cis = (QualInStream*)p;
	int pos=0;
	if(cis->state==writingInit) {
		cis->curN=-1;
		QualInStream_advanceToNextRecord(cis);
	}
	while(pos < *size && cis->p != NULL) {
		if(cis->remain <= *size-pos) {
			memcpy(&((char*)buf)[pos],cis->p,cis->remain);
			pos+=cis->remain;
			if(cis->state==writingID) {
				cis->p=(unsigned char *)cis->conc[cis->curN];
				cis->remain=cis->nsteps*sizeof(float);
				cis->state=writingRecord;
			} else {
				// we just finished writing a concentration record, now find the next one
				QualInStream_advanceToNextRecord(cis);
			}
		} else {
			int n=(int)(*size-pos);
			memcpy(&((char*)buf)[pos],cis->p,n);
			cis->remain -= n;
			cis->p += n;
			pos += n;
		}
	}
#ifdef LZMA_TESTING
	cis->count += pos;
	ensureRawFileOpen();
	fwrite(buf,1,pos,fp_raw);
	if(pos == 0) {
		fclose(fp_raw);
		fp_raw=NULL;
	}
#endif
	*size=pos;
	return SZ_OK;
}
SRes Float2DInStream_Read(void *p, void *buf, size_t *size)
{
	Float2DInStream *fis = (Float2DInStream*)p;
	int pos=0;
	// while the buffer isn't ful and we still have elements to process
	while (pos < *size && fis->p != NULL) {
		// we still have room...
		// if we have written out a partial data chunk...
		// write more of one node/link concentration data
		if(fis->remain <= *size-pos) {
			memcpy(&((char*)buf)[pos],fis->p,fis->remain);
			pos+=fis->remain;
			fis->r++;
			if(fis->r < fis->nrows) {
				fis->p=(unsigned char *)fis->f[fis->r];
				fis->remain=fis->ncols * sizeof(float);
			} else {
				fis->p=NULL;
				fis->remain=0;
			}
		} else {
			int n=(int)(*size-pos);
			memcpy(&((char*)buf)[pos],fis->p,n);
			fis->remain -= n;
			fis->p += n;
			pos += n;
		}
	}
#ifdef LZMA_TESTING
	fis->count+=pos;
	ensureRawFileOpen();
	fwrite(buf,1,pos,fp_raw);
	if(pos == 0) {
		fclose(fp_raw);
		fp_raw=NULL;
	}
#endif
	*size=pos;
	return SZ_OK;
}
static void ensureSpace(LZMAOutStream *os, int additionalSize) {
	if(os->buf==NULL) {
		os->buf=(unsigned char *)calloc(additionalSize,sizeof(unsigned char));
		os->size=additionalSize;
	}
	if(os->curPos+additionalSize > os->size) {
		os->size=os->curPos+additionalSize;
	    os->buf = (unsigned char *)realloc(os->buf,os->size*sizeof(unsigned char));
		if(os->buf==NULL) {
			printf("Unable to allocate %d more bytes\n",additionalSize);
		}
	}
}

size_t LZMAOutStream_Write(void *p, const void *buf, size_t size)
{
	LZMAOutStream *cos = (LZMAOutStream*)p;

	if (size) {
	    unsigned oldPos = cos->curPos;
		ensureSpace(cos,(int)size);
		memcpy(&cos->buf[oldPos], buf, size);
		cos->curPos+=(int)size;
	}
	return size;
}
int lzma_compress_qual(float **conc, int *nz, int nelems, int nsteps, FILE *stream)
{
	CLzmaEncHandle enc = getEncoder();
	SRes res;
	size_t propsSize;
	QualInStream *inStream = getQualInStream();
	LZMAOutStream *outStream = getLZMAOutStream();
	unsigned int lzmasize;
#ifdef LZMA_TESTING
	clock_t t0,t1;

	t0=clock();
#endif

	inStream->conc=conc;
	inStream->curN=0;
	inStream->p=NULL;
	inStream->remain=0;
	inStream->curID=0;
	inStream->state=writingInit;
	inStream->nz=nz;
	inStream->nelems=nelems;
	inStream->nsteps=nsteps;
#ifdef LZMA_TESTING
	inStream->count=0;
#endif
	outStream->curPos=0;

	res = LzmaEnc_SetProps(enc, getProps());

	propsSize = LZMA_PROPS_SIZE;
	ensureSpace(outStream,(int)propsSize);

	res = LzmaEnc_WriteProperties(enc, &outStream->buf[0], &propsSize);
	outStream->curPos+=(int)propsSize;

	res = LzmaEnc_Encode(enc,(ISeqOutStream*)outStream, (ISeqInStream*)inStream,
						 0, &SzAllocForLzma, &SzAllocForLzma);
	lzmasize=outStream->curPos;

#ifdef LZMA_TESTING
	t1=clock();
	ensureCompFileOpen(getProps());
	fprintf(fp,"%d\t%d\t%d\t%f\t%lf\n",getProps()->level,inStream->count,lzmasize,lzmasize*1.0/inStream->count,(double)(t1 - t0) / CLOCKS_PER_SEC);
	fflush(fp);
#endif

	if(fwrite(&lzmasize, sizeof(int), 1, stream) < 1)
		return -1;
	if(fwrite(outStream->buf, sizeof(unsigned char), lzmasize, stream) < lzmasize) 
		return -1;
	return lzmasize+sizeof(int);
}

int lzma_store_qual(PERD db, FILE *stream)
{
	int s,n,t,nnz;
	int nsteps = db->network->numSteps,
		nnodes = db->network->numNodes,
		nlinks = db->network->numLinks;
	PSpeciesData *species = db->network->species;
	PNodeInfo nodes = db->nodes;
	PLinkInfo links = db->links;
	float ***nC = db->network->qualResults->nodeC;
	float ***lC = db->network->qualResults->linkC;
	unsigned int bw = 0;
	int nspecies=db->network->numSpecies;

	// Data for each specie, according to type (bulk, wall)
	for(s = 0; s < nspecies; s++) {
		if(species[s]->index != -1) {
			// Determine which nodes have at least 1 nonzero concentration value
			if( species[s]->type == bulk ) {
				int lzmasize=0;
				int *nz=(int *)calloc(nnodes,sizeof(int));
				// Node concentration values

				for(n = 0; n < nnodes; n++) {
					nodes[n].nz[s] = 0;
				}
				for(n = 0; n < nnodes; n++) {
					for(t = 0; t < nsteps; t++) {
						if(nC[s][n][t] != 0) {
							nodes[n].nz[s] = 1;
							nz[n]=1;
						}
					}
				}
				nnz = 0;
				for(n = 0; n < nnodes; n++) 
					nnz += nodes[n].nz[s];

				if(fwrite(&nnz, sizeof(int), 1, stream) < 1) 
					return 744;
				bw += sizeof(int);
				if(nnz>0) {
					lzmasize=lzma_compress_qual(nC[s], nz, nnodes, nsteps,stream);
				} else {
					lzmasize=0;
				}
				free(nz);
				bw+=lzmasize;
			} else {
				int lzmasize=0;
				// Link concentration values
				int *nz=(int *)calloc(nlinks,sizeof(int));
				for(n = 0; n < nlinks; n++) {
					links[n].nz[s] = 0;
				}
				for(n = 0; n < nlinks; n++) {
					for(t = 0; t < nsteps; t++) {
						if(lC[s][n][t] != 0) {
							links[n].nz[s] = 1;
							nz[n]=1;
						}
					}
				}
				nnz = 0;
				for(n = 0; n < nlinks; n++) 
					nnz += links[n].nz[s];

				if(fwrite(&nnz, sizeof(int), 1, stream) < 1) 
					return 744;
				bw += sizeof(int);
				lzmasize=lzma_compress_qual( lC[s], nz, nlinks, nsteps, stream);
				if(lzmasize==-1)
					return 744;
				bw+=lzmasize;

			}
		}
	}
	return bw;
}

int lzma_compress_float_2d(float **f, int nrows, int ncols, FILE *stream)
{
	CLzmaEncHandle enc = getEncoder();
	SRes res;
	size_t propsSize;
	Float2DInStream *inStream = getFloat2DInStream();
	LZMAOutStream *outStream = getLZMAOutStream();
	unsigned int lzmasize;
#ifdef LZMA_TESTING
	clock_t t0,t1;

	t0=clock();
#endif

	inStream->f=f;
	inStream->r=0;
	inStream->p=(unsigned char*)f[0];
	inStream->remain=ncols*sizeof(float);
	inStream->nrows=nrows;
	inStream->ncols=ncols;
#ifdef LZMA_TESTING
	inStream->count=0;
#endif
	outStream->curPos=0;

	res = LzmaEnc_SetProps(enc, getProps());

	propsSize = LZMA_PROPS_SIZE;
	ensureSpace(outStream,(int)propsSize);

	res = LzmaEnc_WriteProperties(enc, &outStream->buf[0], &propsSize);
	outStream->curPos+=(int)propsSize;

	res = LzmaEnc_Encode(enc,(ISeqOutStream*)outStream, (ISeqInStream*)inStream,
						 0, &SzAllocForLzma, &SzAllocForLzma);
	lzmasize=outStream->curPos;

#ifdef LZMA_TESTING
	t1=clock();
	ensureCompFileOpen(getProps());
	fprintf(fp,"%d\t%d\t%d\t%f\t%lf\n",getProps()->level,inStream->count,lzmasize,lzmasize*1.0/inStream->count,(double)(t1 - t0) / CLOCKS_PER_SEC);
	fflush(fp);
#endif

	if(fwrite(&lzmasize, sizeof(int), 1, stream) < 1)
		return -1;
	if(fwrite(outStream->buf, sizeof(unsigned char), lzmasize, stream) < lzmasize) 
		return -1;
	return lzmasize+sizeof(int);
}

int lzma_compress_float_1d(float *f, int num, FILE *stream)
{
	CLzmaEncHandle enc = getEncoder();
	SRes res;
	size_t propsSize;
	Float2DInStream *inStream = getFloat2DInStream();
	LZMAOutStream *outStream = getLZMAOutStream();
	unsigned int lzmasize;
#ifdef LZMA_TESTING
	clock_t t0,t1;

	t0=clock();
#endif

	inStream->f=&f;
	inStream->r=0;
	inStream->p=(unsigned char*)f;
	inStream->remain=num*sizeof(float);
	inStream->nrows=1;
	inStream->ncols=num;
#ifdef LZMA_TESTING
	inStream->count=0;
#endif
	outStream->curPos=0;

	res = LzmaEnc_SetProps(enc, getProps());

	propsSize = LZMA_PROPS_SIZE;
	ensureSpace(outStream,(int)propsSize);

	res = LzmaEnc_WriteProperties(enc, &outStream->buf[0], &propsSize);
	outStream->curPos+=(int)propsSize;

	res = LzmaEnc_Encode(enc,(ISeqOutStream*)outStream, (ISeqInStream*)inStream,
						 0, &SzAllocForLzma, &SzAllocForLzma);
	lzmasize=outStream->curPos;

#ifdef LZMA_TESTING
	t1=clock();
	ensureCompFileOpen(getProps());
	fprintf(fp,"%d\t%d\t%d\t%f\t%lf\n",getProps()->level,inStream->count,lzmasize,lzmasize*1.0/inStream->count,(double)(t1 - t0) / CLOCKS_PER_SEC);
	fflush(fp);
#endif

	if(fwrite(&lzmasize, sizeof(int), 1, stream) < 1)
		return -1;
	if(fwrite(outStream->buf, sizeof(unsigned char), lzmasize, stream) < lzmasize) 
		return -1;
	return lzmasize+sizeof(int);
}
