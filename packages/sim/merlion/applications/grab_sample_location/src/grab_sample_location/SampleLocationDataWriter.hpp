#ifndef SAMPLE_LOCATION_DATAWRITER_HPP__
#define SAMPLE_LOCATION_DATAWRITER_HPP__

#include <grab_sample_location/SampleNetworkSimulator.hpp>
#include <grab_sample_location/SampleLocationOptions.hpp>
#include <epanet2.h>
//#include <epanetmsx.h>
extern "C" {
 #include <enl.h>
 #include <erd.h>
}

#include <iostream>
#include <string>


class SampleLocationDataWriterMerlion
{
public:

   SampleLocationDataWriterMerlion(SampleNetworkSimulator *Net, SampleLocationOptions *Opts);
   ~SampleLocationDataWriterMerlion();
   void clear();
   void WriteProblem();
   void WriteNodesUncertainty();
   

private:

   const SampleNetworkSimulator *net_;
   const merlionUtils::MerlionModelContainer *model_;
   const SampleLocationOptions *options_;
   std::map<int,std::string> node_map_;

   std::ofstream out_data;
   std::ofstream out_NMAP;
   std::ofstream out_MATRIX;
   std::ofstream out_UNCERTAIN;
   
   std::string out_fname;
   std::string out_NMAP_fname;
   std::string out_MATRIX_fname;
   std::string out_UNCERTAIN_fname;
};

class SampleLocationDataWriterEpanet
{
public:

  SampleLocationDataWriterEpanet(SampleNetworkSimulator *Net, SampleLocationOptions *Opts, PERD db);
  ~SampleLocationDataWriterEpanet();
  void clear();
  void WriteProblem();
  

private:

  const SampleNetworkSimulator *net_;
  //const merlionUtils::EpanetModelContainer *model_;
  PERD db_;
  const SampleLocationOptions *options_;
  std::map<int,std::string> node_map_;

  std::ofstream out_data;
  std::ofstream out_NMAP;
  std::ofstream out_MATRIX;
  
  std::string out_fname;
  std::string out_NMAP_fname;
  std::string out_MATRIX_fname;
};

#endif
