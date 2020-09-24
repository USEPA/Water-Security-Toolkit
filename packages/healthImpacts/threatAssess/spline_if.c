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
#include <stdlib.h>
#include <memory.h>
#include "spline_if.h"

void spline_coef(int *method, int *n, double *x, double *y,
				 double *b, double *c, double *d, double *e);
void spline_eval(int *method, int *nu, double *u, double *v,
		 int *n, double *x, double *y, double *b, double *c, double *d);

SplineDataPtr init_spline(int method,int n, double *x, double *y)
{
	SplineDataPtr sdp = (SplineDataPtr)calloc(1,sizeof(SplineData));
	sdp->method=method;
	sdp->n=n;
	sdp->x=(double*)calloc(n,sizeof(double)); memcpy(sdp->x,x,n*sizeof(double));
	sdp->y=(double*)calloc(n,sizeof(double)); memcpy(sdp->y,y,n*sizeof(double));
	sdp->b=(double*)calloc(n,sizeof(double));
	sdp->c=(double*)calloc(n,sizeof(double));
	sdp->d=(double*)calloc(n,sizeof(double));
	sdp->e=(double*)calloc(n,sizeof(double));

	spline_coef(&sdp->method, &sdp->n, sdp->x, sdp->y,sdp->b,sdp->c,sdp->d,sdp->e);
//	for(i=0;i<10;i++) {
//		printf("%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n",sdp->x[i],sdp->y[i],sdp->b[i],sdp->c[i],sdp->d[i],sdp->e[i]);
//	}
	return sdp;
}

double eval_spline(SplineDataPtr sdp,double x)
{
	int nu=1;
	spline_eval(&sdp->method,&nu,&x,sdp->dv,&sdp->n,sdp->x, sdp->y,sdp->b,sdp->c,sdp->d);
	return sdp->dv[0];
}
void free_spline(SplineDataPtr sdp)
{
	free(sdp->b);
	free(sdp->c);
	free(sdp->d);
	free(sdp->e);
	free(sdp->x);
	free(sdp->y);
	free(sdp);
}
