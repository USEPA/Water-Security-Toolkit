#ifndef MERLION_UTILS_NETWORK_SIMULATOR_HPP__
#define MERLION_UTILS_NETWORK_SIMULATOR_HPP__

#include <merlionUtils/SimTools.hpp>
#include <merlionUtils/Scenarios.hpp>

#include <iostream>
#include <fstream>
#include <stack>
#include <sstream>
#include <set>

namespace merlionUtils {


/** \brief Merlion water quality model reduced inverse
  \date 2012
  \author Gabe Hackebeil
 * This class describes the merlion water quality model reduced inverse. We use a 
 * sparse matrix representation with some additional information about the sensor
 * and grab sample information used to create the reduced inverse. In general,
 * the reduced inverse is a sparse matrix with the same dimension as the original
 * merlion water quality model. However, many of the rows will be empty except for
 * those where a sensor or grab sample existed. To indicate these rows, we store
 * the set of witness ids (upper triangular orientation). These witness ids adhere
 * to an ordering where a larger witness id indicates an earlier point in time (because
 * of the upper triangular orientation). We also keep track of the first (largest)
 * witness id for each possible event id (also upper triangular). For events that
 * are not witnessed, they are assigned the NoWitness id (-1).
*/
class WQM_Inverse : public SparseMatrix
{
public:

   WQM_Inverse() {}
   virtual
   ~WQM_Inverse() {clear();}

   enum {NoWitness=-1};
   
   void clear() {
      SparseMatrix::clear();
      first_witness_.clear();
      // make sure the vector clears its memory reserve
      std::vector<int>().swap(first_witness_);
      witnesses_.clear();
   }

   inline
   void Init(const MerlionModelContainer& model) {
      // initialize the first witness for every injection
      // id to the invalid witness index of -1
      first_witness_.resize(model.N,WQM_Inverse::NoWitness);
      AllocateNewCSRMatrix(model.N,model.N,1);
      witnesses_.clear();
   }

   inline
   void AddWitness(int witness_u_idx) {
      assert(witness_u_idx >= 0);
      assert(witness_u_idx < NRows());
      witnesses_.insert(witness_u_idx);
      // clearly, the first witness for the event at
      // the witness index is itself
      first_witness_[witness_u_idx] = witness_u_idx;
   }

   inline
   bool isWitness(int witness_u_idx) {
      // this is a necessary condition for an existing witness
      return ((witness_u_idx == NoWitness) || (first_witness_[witness_u_idx] != witness_u_idx))?false:true;
   }
   
   inline
   void WitnessEvent(int event_u_idx, int witness_u_idx) {
      assert(event_u_idx >= 0);
      assert(event_u_idx < NRows());
      // this is impossible, a witness id will always
      // be less than or equal to a event id, where
      // equality implies a witness exists at the event id
      assert(witness_u_idx <= event_u_idx);
      // Make sure this is an existing witness
      if (!isWitness(witness_u_idx)) {
	 std::cerr << std::endl;
	 std::cerr << "WitnessEvent() called with invalid witness id." << std::endl;
	 std::cerr << std::endl;
	 exit(1);
      }
      int& event_first_witness_u_idx = first_witness_[event_u_idx];
      if (event_first_witness_u_idx == event_u_idx) {
	 // No other witness can witness an event
	 // any sooner than the witness at the event u_idx.
	 // So there is no point in checking anything after this point
	 return;
      }
      // upper triangular orientation implies a higher index
      // referes to an earlier time
      if ( (event_first_witness_u_idx == NoWitness) ||
	   (witness_u_idx > event_first_witness_u_idx) ) {
	 event_first_witness_u_idx = witness_u_idx;
      }
   }

   inline
   const std::set<int>& Witnesses() const {return witnesses_;}
   inline
   int FirstWitness(int u_idx) const {return first_witness_[u_idx];}

private:
/**@name Default Compiler Generated Methods
    * (Hidden to avoid implicit creation/calling).
    * These methods are not implemented and
    * we do not want the compiler to implement
    * them for us, so we declare them private
    * and do not define them. This ensures that
    * they will not be implicitly created/called. */
   //@{
   /// Copy Constructor
   WQM_Inverse(const WQM_Inverse&);
   /// Overloaded Equals Operator
   void operator=(const WQM_Inverse&);
   //@}

   std::vector<int> first_witness_;
   // std::set will sort these smallest to largest,
   // this is usefull because it represents an ordering
   // in time, which we can leverage
   std::set<int> witnesses_;
};


/** \brief A utility class for running simulations with Merlion
  \date 2012
  \author Gabe Hackebeil
*/
class NetworkSimulator
{
public:

  NetworkSimulator(bool disable_warnings=false)
      :
     disable_warnings_(disable_warnings)
   {
   }

   virtual ~NetworkSimulator()
   {
      logfile_.close();
      clear();
   }

   /// Reset all class members.
   void clear();

   /** @name Methods that modify the Merlion water quality model reduced inverse
    * Modifies the following class members: 
    *  - wqm_inverse_ */
   //@{
   /** Generate a reduced inversion of the Merlion water quality model.
    *  This method should only be called when 
    *  a water quality model is defined (e.g. by
    *  calling ReadWQMFile(...) or ReadINPFile(...))
    *  as well as when the list of sensor nodes or
    *  grab samples node/times is non-empty (e.g. by
    *  calling ReadSensorFile(...)). */
   void GenerateReducedInverse(int sim_start_timestep,
			       int sim_stop_timestep,
                               float sensor_interval_minutes=0,
			       int max_rhs=1,
			       bool map_only=false,
			       float zero_wqm_inv=ZERO_WQM_INV); 
   /** Generate a reduced inversion of the Merlion water quality model.
    *  This method should only be called when 
    *  a water quality model is defined (e.g. by
    *  calling ReadWQMFile(...) or ReadINPFile(...)). */
   void GenerateReducedInverse(const std::set<int>& inverse_row_ids, 
			       int sim_start_timestep,
			       int max_rhs=1,
			       bool map_only=false, 
			       float zero_wqm_invv=ZERO_WQM_INV);
   /// Read a file containing the Merlion water quality model reduced inverse.
   void ReadInverse(std::string fname, bool binary=true);
   /// Clear the Merlion water quality model reduced inverse.
   void ResetInverse();
   //@}


   /** @name Methods that modify the population usage data pointers
    * Modifies the following class members: 
    *  - node_population_
    *  - volume_ingested_m3_ */
   //@{
   /// Use per-capita usage and ingestion rates to generate node population and consumption data estimates
   void GeneratePopulationUsageData(double demand_percapita_m3pmin,
                                    double ingestion_rate_m3pmin);
   /// Reset the population usage data pointers (to NULL)
   void ResetPopulationUsageData();
   //@}
   
   /** @name Methods that modify the injection scenario partition lists
    * Modifies the following class members: 
    *  - detected_scenarios_
    *  - timestep_to_detected_scenarios_
    *  - non_detected_scenarios_
    *  - discarded_scenarios_ */
   //@{
   /// Use the Merlion water quality model reduced inverse to determine scenario detections.
   void ClassifyScenarios(bool detection_times_provided, 
			  int latest_allowed_detection_timestep=-1,
			  float detection_threshold_gpm3=0.0f,
                          float detection_interval_minutes=0.0f);
   /// Reset the current lists of scenario classifications.
   void ResetScenarioClassifications();
   //@}

   /** @name Methods that modify the list of injection scenarios
    * Modifies the following class members: 
    *  - injection_scenarios_ */
   //@{
   /** Read a list of scenarios in a TSG file.
    *  This method should only be called when 
    *  a water quality model is defined (e.g. by 
    *  calling ReadWQMFile(...) or ReadINPFile(...)). 
    *  By default, the scenario list is emptied first
    *  unless the append flag is set to true.*/
   void ReadTSGFile(std::string fname, bool append=false);
   /** Read a list of scenarios in a TSI file.
    *  This method should only be called when 
    *  a water quality model is defined (e.g. by 
    *  calling ReadWQMFile(...) or ReadINPFile(...)). 
    *  By default, the scenario list is emptied first
    *  unless the append flag is set to true. The optional species_id flag
    *  indicates which species id Merlion will recognize for scenario
    *  injections (all others will be ignored, possibly creative scenarios
    *  with empty injections.*/
   void ReadTSIFile(std::string fname, bool append=false, int species_id=-1);
   /** Read a list of scenarios in a SCN file.
    *  This method should only be called when 
    *  a water quality model is defined (e.g. by
    *  calling ReadWQMFile(...) or ReadINPFile(...)).
    *  By default, the scenario list is emptied first
    *  unless the append flag is set to true.*/
   void ReadSCNFile(std::string fname, bool append=false);
   /** Copy the scenario list from another network simulator.
    *  This method should only be called when 
    *  a water quality model is defined (e.g. by 
    *  calling ReadWQMFile(...) or ReadINPFile(...)). 
    *  By default, the scenario list is emptied first
    *  unless the append flag is set to true.*/
   void CopyScenarioList(const NetworkSimulator& net, bool append=false);
   /// Clear the current list of scenarios.
   void ResetScenarios();
   //@}

   /** @name Methods that modify existing injection scenarios
   //@{
   /** Read a list of scenarios weights.
    *  This method should only be called after
    *  a list of injection scenarios is loaded.
    *  Weights must be positive. It is assumed the
    *  weights file is listed in the same order
    *  as the scenario list. */
   void ReadScenarioWeightsFile(std::string fname);
   //@}

   /** @name Methods that modify the Merlion water quality model
    * Modifies the following class members: 
    *  - model_ */
   //@{
   /// Read in a Merlion water quality model file.
   void ReadWQMFile(std::string fname, bool binary=true);
   /// Read in an EPANET input file and generate the Merlion water quality model.
   void ReadINPFile(std::string inp_file, 
                    float duration_min,
                    float qual_step_min,
                    std::string rpt_file,
                    std::string merlion_save_file,
                    float scale=-1.0,
                    int seed=-1,
                    bool ignore_merlion_warnings=false,
		    float deacy_k=0.0f);
   /// Clear the Merlion water quality model
   void ResetModel();
   //@}

   /** @name Methods that modify sensor and grab sample lists
    * Modifies the following class members: 
    *  - sensor_node_ids_
    *  - grab_sample_ids_ */
   //@{
   /** Read a sensor node and grab sample file.
    *  This method should only be called when 
    *  a water quality model is defined (e.g. by
    *  calling ReadWQMFile(...) or ReadINPFile(...)). */
   void ReadSensorFile(std::string fname);
   /// Clear the list of sensor nodes and grab samples.
   void ResetSensorData();
   //@}

   /** @name Output methods */
   //@{
   /** Save the Merlion water quality model reduced inverse.
    *  This method should only be called when a 
    *  Merlion water quality model inverse is defined (e.g
    *  by calling GenerateReducedInverse(...) or
    *  ReadWQMInverseFile(...)). */
   void SaveInverse(std::ostream& out, bool binary=true) const;
   /** Write the list of scenarios in SCN format including detection times.
    *  This method should only be called when a list of injection
    *  scenarios is defined (e.g. by calling ReadTSG(TSI)File(...)
    *  or ReadSCNFile(...) or by manually building one). */
   void WriteDetectedSCNFile(std::ostream& out) const;
   /// Begin logging of class methods.
   void StartLogging(std::string logname);
   /// End logging of class methods.
   void StopLogging();
   //@}

   /** @name Member access methods */
   //@{
   /// Const access to the list of injection scenarios.
   inline const InjScenList& InjectionScenarios() const {return injection_scenarios_;}
   /// Access to the list of injection scenarios.
   inline InjScenList& InjectionScenarios() {return injection_scenarios_;}
   /// Const access to the list of detected injection scenarios.
   inline const InjScenList& DetectedScenarios() const {return detected_scenarios_;}
   /// Const access to the list of non-detected injection scenarios.
   inline const InjScenList& NonDetectedScenarios() const {return non_detected_scenarios_;}
   /// Const access to the list of discared injection scenarios.
   inline const InjScenList& DiscardedScenarios() const {return discarded_scenarios_;}
   /// Const access to the map of timestep to detected scenarios.
   inline const std::map<int, InjScenList>& TimstepToDetectedScenariosMap() const {return timestep_to_detected_scenarios_;}
   /// Const access to the Merlion water quality model.
   inline const MerlionModelContainer& Model() const {return model_;}
   /// Const access to the Merlion water quality model reduced inverse.
   inline const WQM_Inverse& WQMInverse() const {return wqm_inverse_;}
   /// Const access to the list of sensor node ids.
   inline const std::vector<int>& SensorNodeIDS() const {return sensor_node_ids_;}
   /// Const access to the list of grab sample node time pairs (node id, time seconds).
   inline const std::vector<std::pair<int, int> >& GrabSampleIDS() const {return grab_sample_ids_;}
   /// Access to the log file output stream.
   inline std::ofstream& Log() {return logfile_;}
   /// Const access to node population
   inline const std::vector<int>& NodePopulation() const {return node_population_;}
   /// Const access to volume ingested (cubic meters)
   inline const std::vector<double>& VolumeIngested_m3() const {return volume_ingested_m3_;}
   //@}

protected:

   /**@name Default Compiler Generated Methods
    * (Hidden to avoid implicit creation/calling).
    * These methods are not implemented and
    * we do not want the compiler to implement
    * them for us, so we declare them private
    * and do not define them. This ensures that
    * they will not be implicitly created/called. */
   //@{
   /// Copy Constructor
   NetworkSimulator(const NetworkSimulator&);
   /// Overloaded Equals Operator
   void operator=(const NetworkSimulator&);
   //@}

   /// Validates scenarios read from TSG and SCN files
   void ValidateScenarios();

   /// Merlion water quality model
   MerlionModelContainer model_;

   /// Sensor node ids
   std::vector<int> sensor_node_ids_;
   /// Grab sample node time pairs (node id, time seconds)
   std::vector<std::pair<int, int> > grab_sample_ids_;

   /// Merlion water quality model reduced inverse
   WQM_Inverse wqm_inverse_; 

   /// List of injection scenarios
   InjScenList injection_scenarios_;
   
   /** @name Injection scenario classifications */
   //@{
   /// List of detected injection scenarios
   InjScenList detected_scenarios_;
   /// List of non-detected injection scenarios
   InjScenList non_detected_scenarios_;
   /// List of discarded (entirely stagnant flow) injection scenarios
   InjScenList discarded_scenarios_;
   /// Map from timesteps to detected injection scenarios
   std::map<int, InjScenList> timestep_to_detected_scenarios_;
   //@}

   /// Log file output stream used by methods
   std::ofstream logfile_;

   /// Log file name
   std::string logfilename_;

   /// A flag to disable all warnings printed to stderr
   bool disable_warnings_;

   /// Node population and usage data
   std::vector<int> node_population_;
   std::vector<double> volume_ingested_m3_;
};

} // end of merlionUtils namespace

#endif // end of MERLION_UTILS_NETWORK_SIMULATOR_HPP__
