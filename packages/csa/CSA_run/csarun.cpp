/*Annamaria E. De Sanctis*/
/*Test example based on CSA Toolkit vers. 1 mod.September 2009*/

#include <CSArunOptions.hpp>
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

int main(int argc, char** argv)
{ 
  CSArunOptions options;
  options.parse_inputs(argc,argv);

  int errcode;
  errcode = ENopen(strdup(options.inp_filename.c_str()), strdup(options.epanet_report_filename.c_str()), "");  //uses Epanet to open the inp file
  errcode = CSAopen(options.meas_threshold, options.quality_step_sec, options.horizon, options.sim_duration);//opens CSA and set the threshold for the impact coefficient and the lenght of the historical analysis time frame
  errcode = CSAhydraulics(EN_MASS);//runs the hydraulic simulation and then sets the time step to integrate impact coefficients and the water quality source type for BTX
  errcode = CSAaddsensor(options.num_sensors, strdup(options.fixed_sensors_filename.c_str()));// sets the total number of sensors and the name of the file containing the sensor IDs
  errcode = CSAsetbinary(options.measurements_step_sec, strdup(options.measurements_filename.c_str()));// sets the sampling time step and the name of the file with all the water quality results for each sensor location and for each sampling time
  errcode = CSAupdate(strdup(options.output_prefix.c_str()));// runs the updating rule and computes the node and time pairs possible contamnation sources, etc...
  errcode = CSAcandidates(strdup(options.output_prefix.c_str()));//generates a text file called "candidates.txt"; writes for each analysis time the number of unsafe and safe node-time pairs identified during the historical analysis time frame
  errcode = CSAclose();// closes BTX and CSA
  errcode = ENclose();//close epanet
  std::cout<< "\nInversion Complete !\n";
  return 0;
}
