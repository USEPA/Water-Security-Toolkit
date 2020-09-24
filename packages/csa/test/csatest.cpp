/*Annamaria E. De Sanctis*/
/*Test example based on CSA Toolkit vers. 1 mod.September 2009*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
//#include <malloc/malloc.h>
#include <string.h>
#include <math.h> 
extern "C" {
#include "epanet2.h"
#include "epanetbtx.h"
#include "csa.h"
}

int main()
{ 
	int errcode;
	char * eparptfile = "rpt";
	char * inpfile = "net_true.inp";; //network input file
	float samplestep=3600.0;//1.00 monitoring stations sampling step
	float sourcestep=3600.0;
	errcode = ENopen(inpfile, eparptfile, "");  //uses Epanet to open the inp file
	errcode = CSAopen(1.0e-10,sourcestep, 24, 24);//opens CSA and set the threshold for the impact coefficient and the lenght of the historical analysis time frame
	errcode = CSAhydraulics(EN_MASS);//runs the hydraulic simulation and then sets the time step to integrate impact coefficients and the water quality source type for BTX
	errcode = CSAaddsensor(3, "sensoridNet3");// sets the total number of sensors and the name of the file containing the sensor IDs
	errcode = CSAsetbinary(samplestep, "sm.txt");// sets the sampling time step and the name of the file with all the water quality results for each sensor location and for each sampling time
	errcode = CSAupdate("");// runs the updating rule and computes the node and time pairs possible contamnation sources, etc...
	errcode = CSAcandidates("");//generates a text file called "candidates.txt"; writes for each analysis time the number of unsafe and safe node-time pairs identified during the historical analysis time frame
	errcode = CSAclose();// closes BTX and CSA
	errcode = ENclose();//close epanet
	std::cout<< "\nInversion Complete !\n";
	return 0;
}
