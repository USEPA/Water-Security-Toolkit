#ifndef MEASURE_DATAWRITER_HPP__
#define MEASURE_DATAWRITER_HPP__

#include <measure_gen/MeasureSimOptions.hpp>
#include <measure_gen/MeasureNetworkSimulator.hpp>

#include <iostream>
#include <string>


class MeasureGenDataWriter
{
public:

   MeasureGenDataWriter(MeasureGenNetworkSimulator *Net, MeasureGenOptions *Opts);
   ~MeasureGenDataWriter();
   void clear();
   void WriteProblem(float *tox_conc_gpm3);
   void WriteBinaryMeasurements(float *tox_conc_gpm3);
   void set_sensor_grabsample_ids(float *tox_conc_gpm3, int initialTime, int stopTime, int numMeasuresHour);
   void WriteConcentrations(float *tox_conc_gpm3);
   void addMeasurementError();
   void printDemands(const merlionUtils::MerlionModelContainer& model_not_noise);

private:

   const MeasureGenNetworkSimulator *net_;
   const merlionUtils::MerlionModelContainer *model_;
   const MeasureGenOptions *options_;
   std::map<int,std::string> node_map_;
   std::vector<int> positive_utidx;
   std::vector<int> negative_utidx;
   std::ofstream out_WQM;
   std::ofstream out_CONC;
   std::ofstream out_NMAP; // node map replaces string names with integer ids

   std::string out_WQM_fname;
   std::string out_CONC_fname;
   std::string out_NMAP_fname;
};

#endif
