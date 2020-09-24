/*
 * Copyright © 2008 UChicago Argonne, LLC
 * NOTICE: This computer software, TEVA-SPOT, was prepared for UChicago Argonne, LLC
 * as the operator of Argonne National Laboratory under Contract No. DE-AC02-06CH11357
 * with the Department of Energy (DOE). All rights in the computer software are reserved
 * by DOE on behalf of the United States Government and the Contractor as provided in
 * the Contract.
 * NEITHER THE GOVERNMENT NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
 * ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
 *
 * This software is distributed under the BSD License.
 */


#include "teva-assess.h"

LIBEXPORT(void) Response(IndividualIngestionDataPtr *idp, IngestionDataPtr ingData, PDR dr,PNode node,PU u)
{
    int i;
	if(dr==NULL) return;
	if(((ingData->timingMode==IMFixed5 || ingData->timingMode==IMFixed2 || ingData->timingMode==IMUserFixed) && ingData->volumeMode==VMMean) || ingData->timingMode==IMDemand) {
		float *dos=ingData->nodeDoses[0];
		float *res=u->res;
		for(i=0;i<dr->maxsteps;i++){
			if(i==0 || dos[i] != dos[i-1])
				res[i]=DoseResponse(dos[i],dr);
			else
				res[i]=res[i-1];
		}
	} else {
		float *res=u->res;
		int p,pop=(int)node->pop;

		
		memset(res,0,sizeof(float)*dr->maxsteps);
		for(p=0;p<pop;p++) {
			float pres;
			for(i=0;i<dr->maxsteps;i++){
				float *dos=ingData->nodeDoses[p];
				IndividualIngestionDataPtr iidp=idp[p];
				if(i==0 || dos[i] != dos[i-1]) {
					pres=DoseResponse(dos[i],dr);
				}
				res[i]+=pres;
			}
		}
		for(i=0;i<dr->maxsteps;i++){
			res[i] = res[i]/pop;
		}
	}

}
