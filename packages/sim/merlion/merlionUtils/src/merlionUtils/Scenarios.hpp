#ifndef MERLION_UTILS_SCENARIOS_HPP__
#define MERLION_UTILS_SCENARIOS_HPP__


#include <merlionUtils/SimTools.hpp>

#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <fstream>

#include <boost/shared_ptr.hpp>

namespace merlionUtils {

//// Injection Type enumeration and string<->enum helper functions
enum InjType {
   InjType_Flow,
   InjType_Mass,
   InjType_UnDef
};

inline std::string InjTypeToString(InjType x)
{
   if (x == InjType_Mass) {
      return "MASS";
   }
   else if (x == InjType_Flow) {
      return "FLOWPACED";
   }
   return "Undefined";
}
inline InjType StringToInjType(std::string x)
{
   if ((x == "MASS") || (x == "mass")) {
      return InjType_Mass;
   }
 else if ((x == "FLOWPACED") || (x == "flowpaced")) {
      return InjType_Flow;
   }
   else {
      return InjType_UnDef;
   }
}
////

//// Injection and Scenario typenames and structs
//Define an injection into the network  
class Injection {
public:
   Injection()
      :
      node_name_(""),
      strength_(0.0f),
      type_(InjType_UnDef),
      start_time_seconds_(-1.0f),
      stop_time_seconds_(-1.0f)
   {
   }

   Injection(std::string name, float strength, InjType type, float start, float stop)
      :
      node_name_(name),
      strength_(strength),
      type_(type),
      start_time_seconds_(start),
      stop_time_seconds_(stop)
   {
   }

   Injection(const Injection& inj)
      :
      node_name_(inj.NodeName()),
      strength_(inj.Strength()),
      type_(inj.Type()),
      start_time_seconds_(inj.StartTimeSeconds()),
      stop_time_seconds_(inj.StopTimeSeconds())
   {
   }
   
   virtual
   ~Injection()
   {
      node_name_ = "";
      strength_ = 0.0f;
      type_ = InjType_UnDef;
      start_time_seconds_ = -1.0f;
      stop_time_seconds_ = -1.0f;
   }

   static int SecondsToTimestep(double seconds, double stepsize_minutes)
   {
      // Old Method: round to the nearest timestep boundary
      // New Method: floor to obtain the containing timestep
      return SecondsToOwningTimestep(seconds, stepsize_minutes);
   }

   inline const std::string& NodeName() const {return node_name_;}
   inline std::string& NodeName() {return node_name_;}

   inline const double& Strength() const {return strength_;}
   inline double& Strength() {return strength_;}

   inline const InjType& Type() const {return type_;}
   inline InjType& Type() {return type_;}

   inline const double& StartTimeSeconds() const {return start_time_seconds_;}
   inline double& StartTimeSeconds() {return start_time_seconds_;}

   inline const double& StopTimeSeconds() const {return stop_time_seconds_;}
   inline double& StopTimeSeconds() {return stop_time_seconds_;}

   inline int StartTimestep(const MerlionModelContainer& model) const {return SecondsToTimestep(start_time_seconds_,model.qual_step_minutes);}
   inline int StopTimestep(const MerlionModelContainer& model) const {return SecondsToTimestep(stop_time_seconds_,model.qual_step_minutes);}

   // Investigate the total mass injected for the injection
   float MassInjected(const MerlionModelContainer& model) const;
   int MaxIndexChanged(const MerlionModelContainer& model) const;

   // Set the right-hand side of the merlion linear system (Dm)
   template<typename T>
   void SetArray(const MerlionModelContainer& model, T& x_gpmin, float& mass_injected_g, int& max_row_index) const;
   template<typename T>
   void SetArray(const MerlionModelContainer& model, T& x_gpmin) const;
   template<typename T>
   void SetMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin, float& mass_injected_g, int& max_row_index) const;
   template<typename T>
   void SetMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const;
   template<typename T>
   void SetArrayScaled(float scale, const MerlionModelContainer& model, T& x_gpmin, float& mass_injected_g, int& max_row_index) const;
   template<typename T>
   void SetArrayScaled(float scale, const MerlionModelContainer& model, T& x_gpmin) const;
   template<typename T>
   void SetMultiArrayScaled(float scale, const MerlionModelContainer& model, int ncols, int col, T& x_gpmin, float& mass_injected_g, int& max_row_index) const;
   template<typename T>
   void SetMultiArrayScaled(float scale, const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const;

   // clears (zeros) array indices where injection was set
   template<typename T>
   void ClearArray(const MerlionModelContainer& model, T& x_gpmin) const;
   template<typename T>
   void ClearMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const;
   
   // subtracts injection amount from array indices
   template<typename T>
   void UnsetArray(const MerlionModelContainer& model, T& x_gpmin) const;
   template<typename T>
   void UnsetMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const;
   template<typename T>
   void UnsetArrayScaled(float scale, const MerlionModelContainer& model, T& x_gpmin) const;
   template<typename T>
   void UnsetMultiArrayScaled(float scale, const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const;

   bool isValid(const MerlionModelContainer& model) const;
   // scaling that will be applied to the start timestep based
   // on fraction of time contained in that timestep
   float TimeScaleStartTimestep(const MerlionModelContainer& model) const;
   // scaling that will be applied to the stop timestep based
   // on fraction of time contained in that timestep
   float TimeScaleStopTimestep(const MerlionModelContainer& model) const;

private:

   /**@name Default Compiler Generated Methods
    * (Hidden to avoid implicit creation/calling).
    * These methods are not implemented and
    * we do not want the compiler to implement
    * them for us, so we declare them private
    * and do not define them. This ensures that
    * they will not be implicitly created/called. */
   //@{
   /// Overloaded Equals Operator
   void operator=(const Injection&);
   //@}
   
   enum ArrayOp {
      ArrayOp_Add      = 1,
      ArrayOp_NoOp     = 2,
      ArrayOp_Zero     = 3,
      ArrayOp_Subtract = 4
   };

   template<typename T>
   inline bool _SetArrayImp(float scale, const MerlionModelContainer& model, int ncols, int col, T& x_gpmin, float& mass_injected_g, int& max_row_index, ArrayOp operation=ArrayOp_Add) const;

   std::string node_name_;
   double strength_; // g/min for MASS or g/m^3 for FLOWPACED
   InjType type_;
   double start_time_seconds_; // seconds
   double stop_time_seconds_;  // seconds
};
typedef boost::shared_ptr<Injection> PInjection;
typedef std::list<PInjection> InjectionList;

// Define an injection scenario consisting of a list of injections
class InjScenario {
public:

   enum {NoDetectionTime=-1};

   InjScenario()
      :
      name_(""),
      detection_time_seconds_(NoDetectionTime)
   {
   }

   virtual
   ~InjScenario()
   {
      name_ = "";
      ResetDetectionStatus();
      injections_.clear();
      impacts_.clear();
   };

   inline const std::string& Name() const {return name_;}
   inline std::string& Name() {return name_;}

   inline const InjectionList& Injections() const {return injections_;}
   inline InjectionList& Injections() {return injections_;}

   inline double DetectionTimeSeconds() const {return detection_time_seconds_;}

   inline int DetectionTimestep(const MerlionModelContainer& model) const {
      return (RoundToInt(detection_time_seconds_) == NoDetectionTime)?(NoDetectionTime):(SecondsToNearestTimestepBoundary(detection_time_seconds_,model.qual_step_minutes));
   }

   inline void SetDetectionTimeSeconds(double time_seconds) {
      assert(time_seconds >= 0.0f);
      detection_time_seconds_ = time_seconds;
   }

   inline void ResetDetectionStatus() {
      detection_time_seconds_=NoDetectionTime;
   }
   
   inline bool isDetected() const {
      return (RoundToInt(detection_time_seconds_) == NoDetectionTime)?(false):(true);
   }

   void AddImpact(float value, std::string label);
   bool HasImpact(std::string label) const;
   void ClearImpact(std::string label);
   void ClearImpacts();
   void CopyImpacts(std::map<std::string,float>& impacts) const;
   float Impact(std::string label) const;

   // print using seconds and node names
   void Print(std::ostream& out, std::string format="", std::string offset="") const;
   // print using node names and timesteps (for debugging)
   void Print(std::ostream& out, const MerlionModelContainer& model) const;
   
   // Investigate properties of this Scenario's Injections
   int EarliestInjectionTimestep(const MerlionModelContainer& model) const;
   double EarliestInjectionTimeSeconds() const;
   int LatestInjectionTimestep(const MerlionModelContainer& model) const;
   double LatestInjectionTimeSeconds() const;
   float MassInjected(const MerlionModelContainer& model) const;
   int MaxIndexChanged(const MerlionModelContainer& model) const;

   // Set the right-hand side of the merlion linear system (Dm)
   template<typename T>
   void SetArray(const MerlionModelContainer& model, T& x) const;
   template<typename T>
   void SetArray(const MerlionModelContainer& model, T& x_gpmin, float& mass_injected_g, int& max_row_index) const;
   template<typename T>
   void SetMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x) const;
   template<typename T>
   void SetMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin, float& mass_injected_g, int& max_row_index) const;   
   template<typename T>
   void SetArrayScaled(float scale, const MerlionModelContainer& model, T& x) const;
   template<typename T>
   void SetArrayScaled(float scale, const MerlionModelContainer& model, T& x_gpmin, float& mass_injected_g, int& max_row_index) const;
   template<typename T>
   void SetMultiArrayScaled(float scale, const MerlionModelContainer& model, int ncols, int col, T& x) const;
   template<typename T>
   void SetMultiArrayScaled(float scale, const MerlionModelContainer& model, int ncols, int col, T& x_gpmin, float& mass_injected_g, int& max_row_index) const;   

   // clears (zeros) array indices where injection was set
   template<typename T> 
   void ClearArray(const MerlionModelContainer& model, T& x_gpmin) const;
   template<typename T> 
   void ClearMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const;

   // subtracts injection amount from array indices
   template<typename T> 
   void UnsetArray(const MerlionModelContainer& model, T& x_gpmin) const;
   template<typename T> 
   void UnsetMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const;
   template<typename T> 
   void UnsetArrayScaled(float scale, const MerlionModelContainer& model, T& x_gpmin) const;
   template<typename T> 
   void UnsetMultiArrayScaled(float scale, const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const;
   
   // Both injection lists will end up referencing the same injection objects
   void Copy(const InjScenario& orig) {
      //copy name
      name_ = orig.Name();
      //copy detection time
      ResetDetectionStatus();
      if (orig.isDetected()) {
         SetDetectionTimeSeconds(orig.DetectionTimeSeconds());
      }
      //copy impacts
      orig.CopyImpacts(impacts_);
      //copy injection list
      injections_.clear();
      const InjectionList& injlist = orig.Injections();
      for (InjectionList::const_iterator pos = injlist.begin(), stop = injlist.end(); pos != stop; ++pos) {
         // The two injection lists now hold the same smart pointer
         injections_.push_back(*pos);
      }
   }

   // Both injection lists will _NOT_ end up referencing the same injection objects
   void DeepCopy(const InjScenario& orig) {
      //copy name
      name_ = orig.Name();
      //copy detection time
      ResetDetectionStatus();
      if (orig.isDetected()) {
         SetDetectionTimeSeconds(orig.DetectionTimeSeconds());
      }
      //copy impacts
      orig.CopyImpacts(impacts_);
      //deepcopy injections and injection list
      injections_.clear();
      const InjectionList& injlist = orig.Injections();
      for (InjectionList::const_iterator pos = injlist.begin(), stop = injlist.end(); pos != stop; ++pos) {
         PInjection copied_injection(new Injection(**pos)); // call the Injection copy constructor         
         // The two injection lists share no common reference
         injections_.push_back(copied_injection);
      }
   }


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
   InjScenario(const InjScenario&);
   /// Overloaded Equals Operator
   void operator=(const InjScenario&);
   //@}
   
   template<typename T>
   inline void _SetArrayImp(float scale, const MerlionModelContainer& model, int ncols, int col, T& x, float& mass_injected_g, int& max_row_index) const;

   template<typename T> 
   inline void _ClearArrayImp(const MerlionModelContainer& model, int ncols, int col, T& x) const;
   
   template<typename T> 
   inline void _UnsetArrayImp(float scale, const MerlionModelContainer& model, int ncols, int col, T& x) const;

   std::string name_;
   InjectionList injections_;
   double detection_time_seconds_;
   std::map<std::string, float> impacts_;
};
typedef boost::shared_ptr<InjScenario> PInjScenario;
typedef std::list<PInjScenario> InjScenList;

// Helper functions for printing scenario lists to various formats for 
// debugging or for use with other applications
void PrintScenarioList(const InjScenList& scenario_list, std::ostream& out);
void PrintScenarioListYAML(const InjScenList& scenario_list, std::ostream& out);
void PrintScenarioListJSON(const InjScenList& scenario_list, std::ostream& out);

//////////////////////////////////////////////
// Define the Injection templated functions //
//////////////////////////////////////////////

// Set the right-hand side of the merlion linear system (Dm)
template<typename T>
void Injection::SetArray(const MerlionModelContainer& model, T& x_gpmin, float& mass_injected_g, int& max_row_index) const
{
   if (!_SetArrayImp<T>(1.0, model, 1, 0, x_gpmin, mass_injected_g, max_row_index)) {
      exit(1);
   }
}
template<typename T>
void Injection::SetArray(const MerlionModelContainer& model, T& x_gpmin) const
{
   float tmp_float(0.0f);
   int tmp_int(0);
   if (!_SetArrayImp<T>(1.0, model, 1, 0, x_gpmin, tmp_float, tmp_int)) {
      exit(1);
   }
}
template<typename T>
void Injection::SetMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin, float& mass_injected_g, int& max_row_index) const
{
   if (!_SetArrayImp<T>(1.0, model, ncols, col, x_gpmin, mass_injected_g, max_row_index)) {
      exit(1);
   }
}
template<typename T>
void Injection::SetMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const
{
   float tmp_float(0.0f);
   int tmp_int(0);
   if (!_SetArrayImp<T>(1.0, model, ncols, col, x_gpmin, tmp_float, tmp_int)) {
      exit(1);
   }
}
template<typename T>
void Injection::SetArrayScaled(float scale, const MerlionModelContainer& model, T& x_gpmin, float& scaled_mass_injected_g, int& max_row_index) const
{
   if (!_SetArrayImp<T>(scale, model, 1, 0, x_gpmin, scaled_mass_injected_g, max_row_index)) {
      exit(1);
   }
}
template<typename T>
void Injection::SetArrayScaled(float scale, const MerlionModelContainer& model, T& x_gpmin) const
{
   float tmp_float(0.0f);
   int tmp_int(0);
   if (!_SetArrayImp<T>(scale, model, 1, 0, x_gpmin, tmp_float, tmp_int)) {
      exit(1);
   }
}
template<typename T>
void Injection::SetMultiArrayScaled(float scale, const MerlionModelContainer& model, int ncols, int col, T& x_gpmin, float& scaled_mass_injected_g, int& max_row_index) const
{
   if (!_SetArrayImp<T>(scale, model, ncols, col, x_gpmin, scaled_mass_injected_g, max_row_index)) {
      exit(1);
   }
}
template<typename T>
void Injection::SetMultiArrayScaled(float scale, const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const
{
   float tmp_float(0.0f);
   int tmp_int(0);
   if (!_SetArrayImp<T>(scale, model, ncols, col, x_gpmin, tmp_float, tmp_int)) {
      exit(1);
   }
}
template<typename T>
bool Injection::_SetArrayImp(float scale,
                             const MerlionModelContainer& model, 
                             int ncols, int col, 
                             T& x_gpmin, 
                             float& scaled_mass_injected_g, 
                             int& max_row_index, 
                             ArrayOp operation/*=ArrayOp_Add*/) const
{
   assert(ncols >= 1);
   assert(col >= 0 && col < ncols);
   max_row_index = 0;

   float scaled_mass_injected_gpmin(0.0f);
   const float timestep_minutes = model.qual_step_minutes;

   const int node_id = model.NodeID(NodeName());
   const int start_timestep = StartTimestep(model);
   const int stop_timestep = StopTimestep(model);

   float stop_fraction = TimeScaleStopTimestep(model);
   float start_fraction = 1.0;
   // Don't apply the time scaling twice
   if (start_timestep != stop_timestep) {
      start_fraction = TimeScaleStartTimestep(model);
   }

   const int nt_idx_start = node_id*model.n_steps + start_timestep;
   const int nt_idx_stop = node_id*model.n_steps + stop_timestep;
   const NodeType node_type = model.merlionModel->NodeIdxTypeMap()[node_id];

   // These combinations are not yet supported
   if ((node_type == NodeType_Tank) && (Type() == InjType_Flow)) {
      std::cerr << std::endl;
      std::cerr << "Injection ERROR: Flowpaced tank injections not supported." << std::endl;
      std::cerr << std::endl;
      return false;
   }
   /*
   if ((node_type == NodeType_Tank) && (Type() == InjType_Mass)) {
      std::cerr << std::endl;
      std::cerr << "Injection ERROR: Mass tank injections not supported." << std::endl;
      std::cerr << std::endl;
      return false;
   }
   */
   if ((node_type == NodeType_Reservoir) && (Type() == InjType_Flow)) {
      std::cerr << std::endl;
      std::cerr << "Injection ERROR: Flowpaced reservoir injections not supported." << std::endl;
      std::cerr << std::endl;
      return false;
   }

   // These are invalid for the model
   if (start_timestep <= -1 || start_timestep >= model.n_steps) {
      std::cerr << std::endl;
      std::cerr << "Injection ERROR: Injection start timestep is not within simulation period.\n";
      std::cerr << std::endl;
      return false;
   }
   if (stop_timestep <= -1 || stop_timestep >= model.n_steps) {
      std::cerr << std::endl;
      std::cerr << "Injection ERROR: Injection stop timestep is not within simulation period.\n";
      std::cerr << std::endl;
      return false;
   }
   if (stop_timestep < start_timestep) {
      std::cerr << std::endl;
      std::cerr << "Injection ERROR: Injection stop timestep is before simulation start timestep.\n";
      std::cerr << std::endl;
      return false;
   }
   if (node_id <= -1 || node_id >= model.n_nodes) {
      std::cerr << std::endl;
      std::cerr << "Injection ERROR: Injection node_id is not in the list of Merlion node ids.\n";
      std::cerr << std::endl;
      return false;
   }
   if ((Type() != InjType_Flow) && (Type() != InjType_Mass)) {
      std::cerr << std::endl;
      std::cerr << "Injection ERROR: Unsupported injection type." << std::endl;
      std::cerr << std::endl;
      return false;
   }
   
   // loop through the injection ids
   for (int nt_idx = nt_idx_start; nt_idx <= nt_idx_stop; ++nt_idx) {
      if (model.D[nt_idx]) {
         int u_idx = model.perm_nt_to_upper[nt_idx];
         if (u_idx > max_row_index) {
            max_row_index = u_idx;
         }
         float idx_mass_gpmin = scale*strength_;
         if (Type() == InjType_Flow) {
            idx_mass_gpmin *= model.flow_m3pmin[nt_idx];
         }
         if (nt_idx == nt_idx_start) {
            idx_mass_gpmin *= start_fraction;
         }
         if (nt_idx == nt_idx_stop) {
            idx_mass_gpmin *= stop_fraction;
         }
         scaled_mass_injected_gpmin += idx_mass_gpmin;
         // in the case where the total mass injected is required
         // x_gpmin is passed as NULL
         if (operation == ArrayOp_Add) {x_gpmin[u_idx*ncols + col] += idx_mass_gpmin;}
         else if (operation == ArrayOp_Zero) {x_gpmin[u_idx*ncols + col] = 0.0;}
         else if (operation == ArrayOp_Subtract) {x_gpmin[u_idx*ncols + col] -= idx_mass_gpmin;}
      }
   }

   scaled_mass_injected_g = scaled_mass_injected_gpmin*timestep_minutes;
   return true;
}


// clears (zeros) array indices where injection was set
template<typename T>
void Injection::ClearArray(const MerlionModelContainer& model, T& x_gpmin) const 
{
   float tmp_mass(0.0f);
   int tmp_int(0);
   if (!_SetArrayImp(1.0, model, 1, 0, x_gpmin, tmp_mass, tmp_int, ArrayOp_Zero)) {
      exit(1);
   }
}
template<typename T>
void Injection::ClearMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const 
{
   float tmp_mass(0.0f);
   int tmp_int(0);
   if (!_SetArrayImp(1.0, model, ncols, col, x_gpmin, tmp_mass, tmp_int, ArrayOp_Zero)) {
      exit(1);
   }
}

// subtracts injection amount from array indices
template<typename T>
void Injection::UnsetArray(const MerlionModelContainer& model, T& x_gpmin) const 
{
   float tmp_mass(0.0f);
   int tmp_int(0);
   if (!_SetArrayImp(1.0, model, 1, 0, x_gpmin, tmp_mass, tmp_int, ArrayOp_Subtract)) {
      exit(1);
   }
}
template<typename T>
void Injection::UnsetMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const 
{
   float tmp_mass(0.0f);
   int tmp_int(0);
   if (!_SetArrayImp(1.0, model, ncols, col, x_gpmin, tmp_mass, tmp_int, ArrayOp_Subtract)) {
      exit(1);
   }
}
template<typename T>
void Injection::UnsetArrayScaled(float scale, const MerlionModelContainer& model, T& x_gpmin) const 
{
   float tmp_mass(0.0f);
   int tmp_int(0);
   if (!_SetArrayImp(scale, model, 1, 0, x_gpmin, tmp_mass, tmp_int, ArrayOp_Subtract)) {
      exit(1);
   }
}
template<typename T>
void Injection::UnsetMultiArrayScaled(float scale, const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const 
{
   float tmp_mass(0.0f);
   int tmp_int(0);
   if (!_SetArrayImp(scale, model, ncols, col, x_gpmin, tmp_mass, tmp_int, ArrayOp_Subtract)) {
      exit(1);
   }
}

////////////////////////////////////////////////
// Define the InjScenario templated functions //
////////////////////////////////////////////////

// Set the right-hand side of the merlion linear system (Dm)
template<typename T> 
void InjScenario::SetArray(const MerlionModelContainer& model, T& x) const
{
   float tmp_mass_inj_g(0.0);
   int tmp_row_index(0);
   _SetArrayImp<T>(1.0, model, 1, 0, x, tmp_mass_inj_g, tmp_row_index);
}
template<typename T>
void InjScenario::SetArray(const MerlionModelContainer& model, T& x_gpmin, float& mass_injected_g, int& max_row_index) const
{
   _SetArrayImp<T>(1.0, model, 1, 0, x_gpmin, mass_injected_g, max_row_index);
}
template<typename T> 
void InjScenario::SetMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const
{
   float tmp_mass_inj_g(0.0);
   int tmp_row_index(0);
   _SetArrayImp<T>(1.0, model, ncols, col, x_gpmin, tmp_mass_inj_g, tmp_row_index);
}
template<typename T>
void InjScenario::SetMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin, float& mass_injected_g, int& max_row_index) const
{
   _SetArrayImp<T>(1.0, model, ncols, col, x_gpmin, mass_injected_g, max_row_index);
}
template<typename T> 
void InjScenario::SetArrayScaled(float scale, const MerlionModelContainer& model, T& x) const
{
   float tmp_mass_inj_g(0.0);
   int tmp_row_index(0);
   _SetArrayImp<T>(scale, model, 1, 0, x, tmp_mass_inj_g, tmp_row_index);
}
template<typename T>
void InjScenario::SetArrayScaled(float scale, const MerlionModelContainer& model, T& x_gpmin, float& scaled_mass_injected_g, int& max_row_index) const
{
   _SetArrayImp<T>(scale, model, 1, 0, x_gpmin, scaled_mass_injected_g, max_row_index);
}
template<typename T> 
void InjScenario::SetMultiArrayScaled(float scale, const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const
{
   float tmp_mass_inj_g(0.0);
   int tmp_row_index(0);
   _SetArrayImp<T>(scale, model, ncols, col, x_gpmin, tmp_mass_inj_g, tmp_row_index);
}
template<typename T>
void InjScenario::SetMultiArrayScaled(float scale, const MerlionModelContainer& model, int ncols, int col, T& x_gpmin, float& scaled_mass_injected_g, int& max_row_index) const
{
   _SetArrayImp<T>(scale, model, ncols, col, x_gpmin, scaled_mass_injected_g, max_row_index);
}
template<typename T>
void InjScenario::_SetArrayImp(float scale, const MerlionModelContainer& model, int ncols, int col, T& x_gpmin, float& scaled_mass_injected_g, int& max_row_index) const
{
   assert(ncols >= 1);
   assert(col >= 0 && col < ncols);   
   scaled_mass_injected_g = 0.0f;
   max_row_index = 0;

   if (ncols > 1) {
      for (InjectionList::const_iterator p_inj = injections_.begin(), inj_end = injections_.end(); p_inj != inj_end; ++p_inj) {
	 float scaled_inj_mass_injected_g;
	 int inj_max_row_index;
	 (*p_inj)->SetMultiArrayScaled(scale, model, ncols, col, x_gpmin, scaled_inj_mass_injected_g, inj_max_row_index);
	 if (inj_max_row_index > max_row_index) {
	    max_row_index = inj_max_row_index; 
	 }
	 scaled_mass_injected_g += scaled_inj_mass_injected_g;
      }
   }
   else {
      for (InjectionList::const_iterator p_inj = injections_.begin(), inj_end = injections_.end(); p_inj != inj_end; ++p_inj) {
	 float scaled_inj_mass_injected_g;
	 int inj_max_row_index;
	 (*p_inj)->SetArrayScaled(scale, model, x_gpmin, scaled_inj_mass_injected_g, inj_max_row_index);
	 if (inj_max_row_index > max_row_index) {
	    max_row_index = inj_max_row_index; 
	 }
	 scaled_mass_injected_g += scaled_inj_mass_injected_g;
      }
   }
}

// clears (zeros) array indices where injection was set
template<typename T> 
void InjScenario::ClearArray(const MerlionModelContainer& model, T& x_gpmin) const
{
   _ClearArrayImp<T>(model, 1, 0, x_gpmin);
}
template<typename T> 
void InjScenario::ClearMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const
{
   _ClearArrayImp<T>(model, ncols, col, x_gpmin);
}
template<typename T> 
void InjScenario::_ClearArrayImp(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const
{
   assert(ncols >= 1);
   assert(col >= 0 && col < ncols);
   
   if (ncols > 1) {
      for (InjectionList::const_iterator p_inj = injections_.begin(), inj_end = injections_.end(); p_inj != inj_end; ++p_inj) {
	 (*p_inj)->ClearMultiArray(model, ncols, col, x_gpmin);
      }
   }
   else {
      for (InjectionList::const_iterator p_inj = injections_.begin(), inj_end = injections_.end(); p_inj != inj_end; ++p_inj) {
	 (*p_inj)->ClearArray(model, x_gpmin);
      }
   }
}

// subtracts injection amount from array indices
template<typename T> 
void InjScenario::UnsetArray(const MerlionModelContainer& model, T& x_gpmin) const
{
   _UnsetArrayImp<T>(1.0, model, 1, 0, x_gpmin);
}
template<typename T> 
void InjScenario::UnsetMultiArray(const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const
{
   _UnsetArrayImp<T>(1.0, model, ncols, col, x_gpmin);
}
template<typename T> 
void InjScenario::UnsetArrayScaled(float scale, const MerlionModelContainer& model, T& x_gpmin) const
{
   _UnsetArrayImp<T>(scale, model, 1, 0, x_gpmin);
}
template<typename T> 
void InjScenario::UnsetMultiArrayScaled(float scale, const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const
{
   _UnsetArrayImp<T>(scale, model, ncols, col, x_gpmin);
}
template<typename T> 
void InjScenario::_UnsetArrayImp(float scale, const MerlionModelContainer& model, int ncols, int col, T& x_gpmin) const
{
   assert(ncols >= 1);
   assert(col >= 0 && col < ncols);
   
   if (ncols > 1) {
      for (InjectionList::const_iterator p_inj = injections_.begin(), inj_end = injections_.end(); p_inj != inj_end; ++p_inj) {
	 (*p_inj)->UnsetMultiArrayScaled(scale, model, ncols, col, x_gpmin);
      }
   }
   else {
      for (InjectionList::const_iterator p_inj = injections_.begin(), inj_end = injections_.end(); p_inj != inj_end; ++p_inj) {
	 (*p_inj)->UnsetArrayScaled(scale, model, x_gpmin);
      }
   }
}

} // end of merlionUtils namespace

#endif // end of MERLION_UTILS_SCENARIOS_HPP__
