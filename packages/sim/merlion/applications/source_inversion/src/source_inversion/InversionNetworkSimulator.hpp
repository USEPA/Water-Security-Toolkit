#ifndef INVERSION_NETWORK_SIMULATOR_HPP__
#define INVERSION_NETWORK_SIMULATOR_HPP__

#include <merlionUtils/NetworkSimulator.hpp>
#include <source_inversion/Events.hpp>
#include <epanet2.h>
#include <epanetmsx.h>
extern "C" {
   #include <enl.h>
}
#include <erd.h>
#include <tevautil.h>

#include <iostream>
#include <fstream>
#include <stack>
#include <sstream>
#include <set>
#include <map>

const float CONC_ZERO_TOL_GPM3 = 0.0;
typedef boost::shared_ptr<Event> PEvent;
typedef std::list<PEvent> EventList;

//const float ZERO_WQM_INV = 1e-9;
class Measure{

   private:
      std::string node_name_;
      double value_; // binary or g/m^3
      std::string Measure_type_;
      int Measure_time_; // seconds
      double Meas_binary_value_;

   public:
      Measure(std::string nName,double nValue,int seconds)
      {
         node_name_=nName;
         value_=nValue;
         if(value_==1 || value_==0)
	   { Meas_binary_value_ = value_;
	   Measure_type_=="BINARY";}
         else
         {Measure_type_=="READ_CONC";}
         Measure_time_= seconds;
      } 

   inline
   std::string& nodeName()
   {return node_name_;}

   inline
   double& value()
   {return value_;}

   inline 
   const std::string& Type() const 
   {return Measure_type_;}

   inline
   int Measure_timestep(float qual_step_minutes) const
   {return merlionUtils::SecondsToNearestTimestepBoundary(Measure_time_, qual_step_minutes);}

   inline
   int MeasureTimeSeconds() const
   {return Measure_time_;}
};

class InversionNetworkSimulator : public merlionUtils::NetworkSimulator
{
public:
  InversionNetworkSimulator(bool disable_warnings=false)
    :
    NetworkSimulator(disable_warnings)
  {
    
  }
  
  virtual ~InversionNetworkSimulator()
  {
     clear();
  }
  
  void ReadMEASUREMENTS(std::string Measurements_filename);
  void FilterMeasurements();
  void readAllowedNodes(std::string allowedFilename);
  void clear(); //hides the base class clear()   
  
  inline const std::vector<Measure*>& MeasureList() const {return MeasureList_;}
  inline const std::vector<Measure*>& FilteredMeasureList() const {return Filtered_MeasureList_;}
  inline void deleteMeasurement(std::vector<Measure*>::iterator pos)
  {
     delete *pos;
      MeasureList_.erase(pos);
   }
  int reduceMatrixNrows();
  int reduceMatrixNcols();
   //std::map<std::string,float>& probabilities();
  std::vector<std::string> EventProbabilities(float pf, 
					      float conf, 
					      std::string prefix, 
					      float threshold, 
					      float wqm_tol, 
					      bool merlion,
					      std::string inp_fname);
  std::set<int>& ImpactNT_IDX(float wqm_tol, float meas_tol);
  inline const std::vector<std::string>& AllowedNodes() const
  {return allowedNodes_;}

  void setSnapTimeStep(int snap_timestep){snap_timestep_ =snap_timestep;}
  int getSnapTimeStep(){return snap_timestep_;}
  EventList& probableEvents(){return probable_events_;}
  const EventList& probableEvents() const {return probable_events_;}
private:
   EventList probable_events_;
   std::vector<Measure*> MeasureList_;
   std::vector<Measure*> Filtered_MeasureList_;
   std::set<int> impactNTidx_;
   std::vector<std::string> allowedNodes_;
   std::vector<std::string> most_probable_nodes_;
   std::vector<float> top_prob_;
   std::set<int> grabsample_ids_;
   std::vector<int> Prob_IDX;
   std::vector<int> matches;
   int snap_timestep_;
   int run_quality_teva(PERD db, FILE *tsifile, PSourceData source, int stopTime, std::vector<std::pair<int,int> >& measurements, float threshold);
  
};

#endif
