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

#ifndef _SPLINE_IF_H_
#define _SPLINE_IF_H_

typedef struct SplineDataRec {
	int n;
	int method;
	double *x;
	double *y;
	double *b;
	double *c;
	double *d;
	double *e;
	double dv[1];
} SplineData, *SplineDataPtr;

SplineDataPtr init_spline(int method, int n, double *x, double *y);
double eval_spline(SplineDataPtr sdp,double x);
void free_spline(SplineDataPtr sdp);

#endif
