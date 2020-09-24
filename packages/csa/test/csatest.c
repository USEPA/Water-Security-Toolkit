/*Annamaria E. De Sanctis*/
/*Test example based on CSA Toolkit vers. 1 mod.September 2009*/

#include <stdio.h>
#include <stdlib.h>
#include <malloc/malloc.h>
#include <string.h>
#include "epanetbtx.h"
#include "epanet2.h"
#include <math.h> 

int main()
{ 
  //printf("Begin ... ");
	fflush(stdout);
	int errcode;
	char * eparptfile = "rpt";
	char * inpfile = "net_true.inp";; //network input file
	float samplestep=3600.0;//1.00 monitoring stations sampling step
	double sourcestep=3600.0;
	//	printf("before ENopen ..");
	//fflush(stdout);
	errcode = ENopen(inpfile, eparptfile, "");  //uses Epanet to open the inp file
	printf("ENOPen errorcode: %d\n", errcode);
	//printf("\n sourcestep %f \n..", sourcestep);
	//fflush(stdout);
	errcode = CSAopen(1.0e-10,sourcestep, 24);//opens CSA and set the threshold for the impact coefficient and the lenght of the historical analysis time frame
	printf("CSAopen errorcode: %d\n", errcode);
	//printf("after CSAopen ..");
	//fflush(stdout);
	errcode = CSAhydraulics(EN_MASS);//runs the hydraulic simulation and then sets the time step to integrate impact coefficients and the water quality source type for BTX
	printf("CSAhydraulics errorcode: %d\n", errcode);
	errcode = CSAaddsensor(3, "sensoridNet3");// sets the total number of sensors and the name of the file containing the sensor IDs
	printf("CSAaddsensor errorcode: %d\n", errcode);
	errcode = CSAsetbinary(samplestep, "sm.txt");// sets the sampling time step and the name of the file with all the water quality results for each sensor location and for each sampling time
	printf("CSASetbinary errorcode: %d\n", errcode);
	errcode = CSAupdate();// runs the updating rule and computes the node and time pairs possible contamnation sources, etc...
	printf("CSAupdate errorcode: %d\n", errcode);
	errcode = CSAcandidates();//generates a text file called "candidates.txt"; writes for each analysis time the number of unsafe and safe node-time pairs identified during the historical analysis time frame
	printf("CSAcandidates errorcode: %d\n", errcode);
	errcode = CSAclose();// closes BTX and CSA
	printf("CSAclose errorcode: %d\n", errcode);
	errcode = ENclose();//close epanet
	printf("ENclose errorcode: %d\n", errcode);
	return 0;
}
