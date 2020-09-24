#ifndef BOOSTER_BOOSTERDATAWRITER_HPP__
#define BOOSTER_BOOSTERDATAWRITER_HPP__

#include <booster/BoosterNetworkSimulator.hpp>
#include <booster/BoosterOptions.hpp>

#include <iostream>
#include <fstream>
#include <string>

class BoosterDataWriter
{
public:

   BoosterDataWriter(NetworkSimulator *Net, BoosterNetworkSimulator *Chl_Net, NetworkSimulator *Tox_Net, BoosterOptions *Opts);
   virtual ~BoosterDataWriter();
   void clear();

   virtual void WriteWQHeadersFile(int earliest_timestep, int latest_timestep)=0;
   virtual void WriteWQNodeTypesFile()=0;
   virtual void WriteWQModelFile(int earliest_timestep, int latest_timestep)=0;
   virtual void WriteWQDemandsFile(int earliest_timestep, int latest_timestep)=0;
   virtual void WriteWQTankVolumesFile(int earliest_timestep, int latest_timestep)=0;

   virtual void WriteBoosterWQHeadersFile(int earliest_timestep, int latest_timestep)=0;
   virtual void WriteBoosterWQNodeTypesFile()=0;
   virtual void WriteBoosterWQModelFile(int earliest_timestep, int latest_timestep)=0;
   virtual void WriteBoosterWQDemandsFile(int earliest_timestep, int latest_timestep)=0;
   virtual void WriteBoosterWQTankVolumesFile(int earliest_timestep, int latest_timestep)=0;
   virtual void WriteBoosterCandidatesFile(int earliest_timestep, int latest_timestep)=0;
   virtual void WriteBoosterInjectionsFile(int earliest_timestep, int latest_timestep)=0;

   virtual void WriteToxinWQHeadersFile(int earliest_timestep, int latest_timestep)=0;
   virtual void WriteToxinWQNodeTypesFile()=0;
   virtual void WriteToxinWQModelFile(int earliest_timestep, int latest_timestep)=0;
   virtual void WriteToxinWQDemandsFile(int earliest_timestep, int latest_timestep)=0;
   virtual void WriteToxinWQTankVolumesFile(int earliest_timestep, int latest_timestep)=0;
   virtual void WriteToxinInjectionsFile()=0;

   virtual void WritePopulationDosageFile(int earliest_timestep, int latest_timestep)=0;
   virtual void AppendPopulationDosageData(std::string scenario_name,
                                           const std::vector<float>& node_total_dose_g,
                                           bool first_time,
                                           bool leave_open)=0;
   virtual void WriteDosageScenario(const std::vector<std::map<int,float> >& node_objective_terms,
                                    const std::vector<float>& always_dosed_g,
                                    const std::vector<std::map<std::vector<int>, int>::const_iterator>& new_cols,
                                    const InjScenario& toxin_scen,
                                    float scaling,
                                    bool is_first)=0;

   virtual void WriteScenarioSummaryFile()=0;
   
   virtual void WriteScenario(const std::vector<std::vector<int> >& z_rows,
                              const float* tox_conc_gpm3,
                              const BoosterScenario& scen,
                              bool is_first,
                              bool summary_only)=0;
   virtual void WriteReducedProblem(std::multimap<float, std::vector<int> >& reduced_z_rows)=0;

   std::map<int, std::string>& NodeMap() {return node_map_;}

protected:

   const NetworkSimulator *net_;
   const merlionUtils::MerlionModelContainer *model_;

   const BoosterNetworkSimulator *chl_net_;
   const merlionUtils::MerlionModelContainer *chl_model_;
   const NetworkSimulator *tox_net_;
   const merlionUtils::MerlionModelContainer *tox_model_;
   const BoosterOptions *options_;
   std::map<int,std::string> node_map_;

   std::ofstream out_SUM;  // SCENARIO set and all params indexed by it
   std::ofstream out_SCL;  // scaling used on scenarios
   std::ofstream out_SPCT; // SPACETIME set and all params indexed by it
   std::ofstream out_CONT; // z_rows matrix
   std::ofstream out_REDM; // map from objective terms
   std::ofstream out_DIND; // Index set for delta sets
   std::ofstream out_NMAP; // node map replaces string names with integer ids
   std::ofstream out_BC;   // set of booster candidates
   std::ofstream out_TOX;  // toxin injection profiles
   std::ofstream out_CHL;  // chlorine injection profiles
   std::ofstream out_DEM; // The demand profile
   std::ofstream out_TANK; // The tank volume profile
   std::ofstream out_WQM; // The sparse water quality matrices
   std::ofstream out_HEAD; // The water quality model indexing sets
   std::ofstream out_TYPES; // The water quality model node types
   std::ofstream out_PYSP; // PySp specific files
   std::ofstream out_POP; // Population Usage and Dosage Data

   std::string out_DEM_basename;
   std::string out_TANK_basename;
   std::string out_WQM_basename;
   std::string out_HEAD_basename;
   std::string out_TYPES_basename;

   std::string out_SUM_fname;
   std::string out_SCL_fname;
   std::string out_SPCT_fname;
   std::string out_CONT_fname;
   std::string out_REDM_fname;
   std::string out_DIND_fname;
   std::string out_NMAP_fname;
   std::string out_BC_fname;
   std::string out_TOX_fname;
   std::string out_CHL_fname;
   std::string out_DEM_fname;
   std::string out_TANK_fname;
   std::string out_WQM_fname;
   std::string out_HEAD_fname;
   std::string out_TYPES_fname;
   std::string out_PYSP_fname;
   std::string out_POP_fname;

};

#endif
