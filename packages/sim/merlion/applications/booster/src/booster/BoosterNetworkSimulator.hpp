#ifndef BOOSTER_NETWORK_SIMULATOR_HPP__
#define BOOSTER_NETWORK_SIMULATOR_HPP__

#include <merlionUtils/NetworkSimulator.hpp>

#include <iostream>
#include <fstream>
#include <stack>
#include <sstream>

const float CHL_CONC_ZERO_TOL_GPM3     = 1e-30f;

// Clean up syntax for setting up a for loop using an stl container iterator
#ifdef STL_ITERATE
#undef STL_ITERATE
#endif
#define STL_ITERATE(type,iter,stop,container) (type::iterator iter = container.begin(), stop = container.end(); iter != stop; ++iter )
#ifdef STL_CONST_ITERATE
#undef STL_CONST_ITERATE
#endif
#define STL_CONST_ITERATE(type,iter,stop,container) (type::const_iterator iter = container.begin(), stop = container.end(); iter != stop; ++iter )
/*
usage:
   for STL_ITERATE(std::vector<int>, pos, stop, myvector) {
      ...   
   }

turns into:
   for (std::vector<int>::iterator pos = myvector.begin(), stop = myvector.end(); pos != stop; ++pos) {
      ...
   }
*/

using namespace merlionUtils;

class BoosterScenario: public InjScenario {
public:

   BoosterScenario()
      :
      sim_stop_timestep(-1)
   {
   }

   virtual
   ~BoosterScenario()
   {
      sim_stop_timestep = -1;
      toxin_scenarios_.clear();
   }

   inline const InjScenList& ToxinScenarios() const {return toxin_scenarios_;}
   inline InjScenList& ToxinScenarios() {return toxin_scenarios_;}

   int sim_stop_timestep;
private:
   InjScenList toxin_scenarios_;
};
typedef boost::shared_ptr<BoosterScenario> PBoosterScenario;
typedef std::list<PBoosterScenario> BoosterScenList;

class BoosterNetworkSimulator: public NetworkSimulator
{
public:

  BoosterNetworkSimulator(bool disable_warnings=false)
   :
     NetworkSimulator(disable_warnings),
     booster_delay_seconds_(0),
     booster_inj_seconds_(0),
     sim_after_detect_timesteps_(0),
     booster_inj_type_(InjType_UnDef),
     booster_inj_strength_(0.0f)
   {
   }

   virtual ~BoosterNetworkSimulator()
   {
      clear();
   }

   void ClassifyBoosterScenarios(bool detection_times_provided,
                                 bool scenario_aggregation=true, 
                                 float detection_tol_gpm3=0.0f,
                                 float detection_interval_min=0.0f);  
   void ReadBoosterFile(std::string fname);
   void ResetBoosterScenarios();
   void clear(); //hides base class clear(), but this method calls the base class clear()

   inline const BoosterScenList& BoosterScenarios() const {return booster_scenarios_;}
   inline BoosterScenList& BoosterScenarios() {return booster_scenarios_;}
   inline const std::vector<int>& BoosterCandidates() const {return booster_candidates_;}
   inline InjType BoosterInjectionType() const {return booster_inj_type_;}
   inline float BoosterInjectionStrength() const {return booster_inj_strength_;}

protected:

   inline double GetBoosterStartTimeSeconds(double t) {return t+booster_delay_seconds_;}
   inline double GetBoosterStopTimeSeconds(double t) {return t+booster_inj_seconds_+booster_delay_seconds_;} 
   inline int GetCalcStopTimestep(int t) {return t+sim_after_detect_timesteps_;}

   // Read from *.scen file or defined
   // by sensor layout detection code
   // The list of scenarios used in the booster station placement
   BoosterScenList booster_scenarios_;

   //booster station info
   double booster_delay_seconds_;
   double booster_inj_seconds_;
   int sim_after_detect_timesteps_;
   InjType booster_inj_type_;
   float booster_inj_strength_;
   std::vector<int> booster_candidates_;

};

#endif
