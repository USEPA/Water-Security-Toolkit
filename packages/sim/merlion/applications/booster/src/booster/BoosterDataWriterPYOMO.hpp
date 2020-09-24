#ifndef BOOSTER_BOOSTERDATAWRITERPYOMO_HPP__
#define BOOSTER_BOOSTERDATAWRITERPYOMO_HPP__

#include <booster/BoosterDataWriter.hpp>

class BoosterDataWriterPYOMO: public BoosterDataWriter
{
public:

   BoosterDataWriterPYOMO(NetworkSimulator *Net, BoosterNetworkSimulator *Chl_Net, NetworkSimulator *Tox_Net, BoosterOptions *Opts)
      :
      BoosterDataWriter(Net, Chl_Net, Tox_Net, Opts)
   {
   }
   virtual ~BoosterDataWriterPYOMO()
   {
   }

   virtual void WriteWQHeadersFile(int earliest_timestep, int latest_timestep);
   virtual void WriteWQNodeTypesFile();
   virtual void WriteWQModelFile(int earliest_timestep, int latest_timestep);
   virtual void WriteWQDemandsFile(int earliest_timestep, int latest_timestep);
   virtual void WriteWQTankVolumesFile(int earliest_timestep, int latest_timestep);

   virtual void WriteBoosterWQHeadersFile(int earliest_timestep, int latest_timestep);
   virtual void WriteBoosterWQNodeTypesFile();
   virtual void WriteBoosterWQModelFile(int earliest_timestep, int latest_timestep);
   virtual void WriteBoosterWQDemandsFile(int earliest_timestep, int latest_timestep);
   virtual void WriteBoosterWQTankVolumesFile(int earliest_timestep, int latest_timestep);
   virtual void WriteBoosterCandidatesFile(int earliest_timestep, int latest_timestep);
   virtual void WriteBoosterInjectionsFile(int earliest_timestep, int latest_timestep);

   virtual void WriteToxinWQHeadersFile(int earliest_timestep, int latest_timestep);
   virtual void WriteToxinWQNodeTypesFile();
   virtual void WriteToxinWQModelFile(int earliest_timestep, int latest_timestep);
   virtual void WriteToxinWQDemandsFile(int earliest_timestep, int latest_timestep);
   virtual void WriteToxinWQTankVolumesFile(int earliest_timestep, int latest_timestep);
   virtual void WriteToxinInjectionsFile();

   virtual void WritePopulationDosageFile(int earliest_timestep, int latest_timestep);
   virtual void AppendPopulationDosageData(std::string scenario_name,
                                           const std::vector<float>& node_total_dose_g,
                                           bool first_time,
                                           bool leave_open);
   virtual void WriteDosageScenario(const std::vector<std::map<int,float> >& node_objective_terms,
                                    const std::vector<float>& always_dosed_g,
                                    const std::vector<std::map<std::vector<int>, int>::const_iterator>& new_cols,
                                    const InjScenario& toxin_scen,
                                    float scaling,
                                    bool is_first);

   virtual void WriteScenarioSummaryFile();
   
   virtual void WriteScenario(const std::vector<std::vector<int> >& z_rows,
                              const float* tox_conc_gpm3,
                              const BoosterScenario& scen,
                              bool is_first,
                              bool summary_only);
   virtual void WriteReducedProblem(std::multimap<float, std::vector<int> >& reduced_z_rows);

};

#endif
