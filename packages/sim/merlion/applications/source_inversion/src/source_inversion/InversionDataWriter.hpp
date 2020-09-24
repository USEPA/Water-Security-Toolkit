#ifndef INVERSION_DATAWRITER_HPP__
#define INVERSION_DATAWRITER_HPP__

#include <source_inversion/InversionSimOptions.hpp>
#include <source_inversion/InversionNetworkSimulator.hpp>

#include <iostream>
#include <string>


class InversionDataWriter
{
public:

   InversionDataWriter(InversionNetworkSimulator *Net, InversionSimOptions *Opts);
   ~InversionDataWriter();
   void clear();
   void WriteProblem(float *tox_conc_gpm3);

   void WriteReduceMAtrixGInverse();
   void WriteGMAtrixToAMPL(int start_timestep,int stop_timestep);
   void WriteMeasurments(int start_time, int stop_time);
   void WriteImpactNodeNames(std::vector<std::string> nodes_v);
 
   void WriteUncertainNodes();
  

private:

   const InversionNetworkSimulator *net_;
   const merlionUtils::MerlionModelContainer *model_;
   const InversionSimOptions *options_;
   std::map<int,std::string> node_map_;

   std::ofstream out_WQM;
   std::ofstream out_CONC;
   std::ofstream out_index;
   std::ofstream out_vals;
   std::ofstream out_NMAP;
   std::ofstream out_IMPACTN;
   std::ofstream out_UNCERTAIN;

   std::string out_WQM_fname;
   std::string out_CONC_fname;
   std::string out_INDEX_fname;
   std::string out_VALS_fname;
   std::string out_NMAP_fname;
   std::string out_IMPACTN_fname;
   std::string out_UNCERTAIN_fname;

};

#endif
