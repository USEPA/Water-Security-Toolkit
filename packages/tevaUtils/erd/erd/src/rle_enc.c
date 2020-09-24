#include "rle_enc.h"
#include <time.h>

int compressAllAndStoreRLE(PERD db, FILE *stream)
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
	size_t bw = 0;

	// Data for each specie, according to type (bulk, wall)
	for(s = 0; s < db->network->numSpecies; s++) 
	{
		if(species[s]->index != -1) {
			// Determine which nodes have at least 1 nonzero concentration value
			if( species[s]->type == bulk ) {
				// Node concentration values

				for(n = 0; n < nnodes; n++) {
					nodes[n].nz[s] = 0;
				}
				for(n = 0; n < nnodes; n++) {
					for(t = 0; t < nsteps; t++) {
						if(nC[s][n][t] != 0) {
							nodes[n].nz[s] = 1;
						}
					}
				}
				nnz = 0;
				for(n = 0; n < nnodes; n++) 
					nnz += nodes[n].nz[s];

				if(fwrite(&nnz, sizeof(int), 1, stream) < 1) 
					return 744;
				bw += sizeof(int);
				for(n = 1; n <= nnodes; n++) {
					PNodeInfo node = &nodes[n - 1];
					if(node->nz[s]) {
						int size;
						if(fwrite(&n, sizeof(int), 1, stream) < 1) 
							return 744;
						bw += sizeof(int);
						size = compressAndStoreRLE(nC[s][n-1], nsteps, stream);
						if(size == -1) 
							return 744;
						bw += size;
					}
				}
			} else {
				// Link concentration values
				for(n = 0; n < nlinks; n++) {
					links[n].nz[s] = 0;
				}
				for(n = 0; n < nlinks; n++) {
					for(t = 0; t < nsteps; t++) {
						if(lC[s][n][t] != 0) 
							links[n].nz[s] = 1;
					}
				}
				nnz = 0;
				for(n = 0; n < nlinks; n++) 
					nnz += links[n].nz[s];

				if(fwrite(&nnz, sizeof(int), 1, stream) < 1) 
					return 744;
				bw += sizeof(int);
				for(n = 1; n <= nlinks; n++) {
					PLinkInfo link = &links[n - 1];
					if(link->nz[s]) {
						int size;
						if(fwrite(&n, sizeof(int), 1, stream) < 1) 
							return 744;
						bw += sizeof(int);
						size = compressAndStoreRLE(lC[s][n-1], nsteps, stream);
						if(size == -1) 
							return 744;
						bw += size;
					}
				}
			}
		}
	}
	return (int)bw;
}
int compressAndStoreRLE(float *dtc, int nsteps, FILE *stream)
{
	char *cd;
	float *values;
	short *counts;
	// numGroups is a short (rather than a char) to allow for the rare (at least currently)
	// condition of having more than 255 groups in one record (I have recently seen one
	// with 177 groups.  I chose to make this a short rather than doing anything more complicated
	// algorithmically because adding one byte to every record is negligable, and any algorithm
	// implemented to deal with more than 255 groups would be time intensive and would use extra space,
	short numGroups = 0;
	int numValues;
	int p = 2;
	int i, j, rv;
	
	MEMCHECK(cd = (char *)calloc((nsteps + 10) * sizeof(float), sizeof(char)), "cd in compressAndStore3");
	MEMCHECK(values = (float *)calloc(nsteps, sizeof(float)), "values in compressAndStore3");
	MEMCHECK(counts = (short *)calloc(nsteps, sizeof(short)), "counts in compressAndStore3");

	values[0] = dtc[0];
	counts[0] = 1;
	i = 1;
	j = 0;
	while(i < nsteps) {
		if(dtc[i] != values[j]) {
			j++;
			values[j] = dtc[i];
		}
		counts[j]++;
		i++;
	}
	numValues = j + 1;
	i = 0;
	while(i < numValues) {
		if(counts[i] > 1 || values[i] == 0) {
			float v = values[i];
			unsigned short mask = v == 0 ? (unsigned short)0x0000 : (unsigned short)0x8000;
			short sv = counts[i] | mask;
			memcpy(&cd[p], &sv, sizeof(short));
			p += 2;
			if(v != 0) {
				memcpy(&cd[p], &v, sizeof(float));
				p += 4;
			}
			i++;
		} 
		else {
			// save the start pos (to write out the total later)
			int cp = p;
			short count = 0;
			p += 2; // start after the size word
			while(i < numValues && counts[i] == 1) {
				memcpy(&cd[p], &values[i], sizeof(float));
				p += 4;
				count++;
				i++;
			}
			count |= 0x4000;
			memcpy(&cd[cp], &count, sizeof(short));
		}
		numGroups++;
	}
	memcpy(&cd[0], &numGroups, sizeof(short));
	fflush(stream);
	rv = fwrite(cd, sizeof(char), p, stream) == (unsigned int)p;
	fflush(stream);
	free(cd);
	free(values);
	free(counts);
	return rv ? p : -1;
}

