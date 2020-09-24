#include "rle_dec.h"
typedef struct 
{
    char groupType;
    short count;
} GroupHeader, *PGroupHeader;


int readAndDecompressRLE(FILE *stream, PERD db)
{
	int idx, nnz, inz, inode, s,
		status = 0,
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

	// Read the data for each specie according to their type (bulk, wall)
	for(s = 0; s < nspecies; s++) {
		unsigned short groupTypeMask = 0xC000;
		unsigned short countMask = 0x3FFF;
		unsigned short countBits = 14;
		if( species[s]->index!=-1) {
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
			// nnz is the number of nodes or links with non-zero specie concentrations
			if(fread(&nnz, sizeof(int), 1, stream) != 1)
				status = 1;

			for(inz = 0; inz < nnz; inz++) {
				float *c;
				unsigned short numGroups;
				int n = 0, 
					i,
					max = 0;

				if(fread(&idx, sizeof(int), 1, stream) != 1)
					status = 1;

				idx--;

				if( species[s]->type == bulk ) {
					nodes[idx].nz[s] = 1;
					c = nC[s][idx];
				} else {
					links[idx].nz[s] = 1;
					c = lC[s][idx];
				}

				
				if(fread(&numGroups, sizeof(short), 1, stream) != 1)
				status = 1;

				for(i = 0; i < numGroups; i++) {
					GroupHeader gh;
					short sval;
					if(fread(&sval, sizeof(short), 1, stream) != 1)
					status = 1;

					gh.groupType = (char)((sval & groupTypeMask) >> countBits);
					gh.count = sval & countMask;
					max += gh.count;
					if(gh.groupType == 1) {
						if(fread(&c[n], sizeof(float), gh.count, stream) != (unsigned int)gh.count)
							status = 1;
						n = max;
					}
					else {
						float v;
						if(gh.groupType == 0)
							v = 0;
						else if(gh.groupType == 2) {
							if(fread(&v, sizeof(float), 1, stream) != 1)
								status = 1;
						}
						else {
							erdError(1, "Invalid group type");
							v=0;
						}
						while(n < max) {
							c[n++] = v;
						}

					}
				}
			}
		}
	}

	if(status)
		return TRUE;
	return FALSE;
}
