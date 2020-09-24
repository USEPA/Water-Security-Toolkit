#ifndef BOOSTER_BOOSTERDATAWRITERPYSP_HPP__
#define BOOSTER_BOOSTERDATAWRITERPYSP_HPP__

#include <booster/BoosterDataWriterAMPL.hpp>

class BoosterDataWriterPYSP: public BoosterDataWriterAMPL
{
public:

   BoosterDataWriterPYSP(NetworkSimulator *Net, BoosterNetworkSimulator *Chl_Net, NetworkSimulator *Tox_Net, BoosterOptions *Opts)
      :
      BoosterDataWriterAMPL(Net, Chl_Net, Tox_Net, Opts)
   {
   }
   virtual ~BoosterDataWriterPYSP()
   {
   }

   virtual void WriteScenarioSummaryFile()
   {
      std::cerr << std::endl;
      std::cerr << "ERROR: Function Not Implemented!" << std::endl;
      std::cerr << std::endl;
      exit(1);
   }

   virtual void WriteScenario(std::map<int,std::vector<int> >& z_rows,
                              const float* tox_conc_gpm3,
                              const BoosterScenario& scen,
                              bool is_first,
                              bool summary_only);
   
   virtual void WriteReducedProblem(std::multimap<float, std::vector<int> >& reduced_z_rows)
   {
      std::cerr << std::endl;
      std::cerr << "ERROR: Function Not Implemented!" << std::endl;
      std::cerr << std::endl;
      exit(1);
   }
   
private:

   void pysp_write_scenario_limit_model(const BoosterScenario& scen, std::string fname, bool include_root);
   void pysp_write_scenario_neutral_model(std::map<int,std::vector<int> >& z_rows,
                                          const float* tox_conc_gpm3,
                                          const BoosterScenario& scen,
                                          std::string fname);
   void pysp_write_scenario_structure();

};

#endif
