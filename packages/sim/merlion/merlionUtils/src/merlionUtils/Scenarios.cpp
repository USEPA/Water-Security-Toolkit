#include <merlionUtils/Scenarios.hpp>
#include <merlionUtils/SimTools.hpp>

#include <merlion/Merlion.hpp>
#include <merlion/SparseMatrix.hpp>

namespace merlionUtils {


float Injection::TimeScaleStartTimestep(const MerlionModelContainer& model) const
{
   const int start_timestep = StartTimestep(model);
   const int stop_timestep = StopTimestep(model);
   const float timestep_seconds = model.qual_step_minutes*60.0;

   if (start_timestep == stop_timestep) {
      return (StopTimeSeconds() - StartTimeSeconds()) / timestep_seconds;
   }
   return (((start_timestep+1)*timestep_seconds) - StartTimeSeconds()) / timestep_seconds;
}

float Injection::TimeScaleStopTimestep(const MerlionModelContainer& model) const
{
   const int start_timestep = StartTimestep(model);
   const int stop_timestep = StopTimestep(model);
   const float timestep_seconds = model.qual_step_minutes*60.0;

   if (start_timestep == stop_timestep) {
      return (StopTimeSeconds() - StartTimeSeconds()) / timestep_seconds;
   }
   return (StopTimeSeconds() - (stop_timestep*timestep_seconds)) / timestep_seconds;
}

bool Injection::isValid(const MerlionModelContainer& model) const
{
   float tmp_float(0.0f);
   float *tmp(NULL);
   int tmp_int(0);
   return _SetArrayImp(1.0, model, 1, 0, tmp, tmp_float, tmp_int, ArrayOp_NoOp);
}

float Injection::MassInjected(const MerlionModelContainer& model) const
{
   float mass_injected_g(0.0f);
   float *tmp(NULL);
   int tmp_int(0);
   if (!_SetArrayImp(1.0, model, 1, 0, tmp, mass_injected_g, tmp_int, ArrayOp_NoOp)) {
      exit(1);
   }
   return mass_injected_g;
}

int Injection::MaxIndexChanged(const MerlionModelContainer& model) const
{
   float tmp_float(0.0f);
   float *tmp(NULL);
   int max_index(0);
   if (!_SetArrayImp(1.0, model, 1, 0, tmp, tmp_float, max_index, ArrayOp_NoOp)) {
      exit(1);
   }
   return max_index;
}

float InjScenario::MassInjected(const MerlionModelContainer& model) const
{
   float mass_injected_g = 0.0f;
   for (InjectionList::const_iterator p_inj = injections_.begin(), inj_end = injections_.end(); p_inj != inj_end; ++p_inj) {      
      mass_injected_g += (*p_inj)->MassInjected(model);
   }
   return mass_injected_g;
}

int InjScenario::MaxIndexChanged(const MerlionModelContainer& model) const
{
   int max_index(0);
   for (InjectionList::const_iterator p_inj = injections_.begin(), inj_end = injections_.end(); p_inj != inj_end; ++p_inj) {
      int this_max_index = (*p_inj)->MaxIndexChanged(model);
      if (this_max_index > max_index) {
         max_index = this_max_index;
      }
   }
   return max_index;
}

void InjScenario::AddImpact(float value, std::string label) {
   impacts_[label] = value;
}

void InjScenario::ClearImpact(std::string label) {   
   if (impacts_.find(label) != impacts_.end()) {
      impacts_.erase(label);
      return;
   }
   std::cerr << std::endl;
   std::cerr << "ERROR: Scenario impact label not found: \"" << label << "\"" << std::endl;
   std::cerr << std::endl;
   exit(1);
}

void InjScenario::ClearImpacts() {
   impacts_.clear();
}

void InjScenario::CopyImpacts(std::map<std::string,float>& impacts) const {
   impacts = impacts_; //copy
}

float InjScenario::Impact(std::string label) const {
   for (std::map<std::string, float>::const_iterator pos = impacts_.begin(), stop = impacts_.end(); pos != stop; ++pos) {
      if (label == pos->first) {
         return pos->second; 
      }
   }
   std::cerr << std::endl;
   std::cerr << "ERROR: Scenario impact label not found: \"" << label << "\"" << std::endl;
   std::cerr << std::endl;
   exit(1);
}

bool InjScenario::HasImpact(std::string label) const {
   for (std::map<std::string, float>::const_iterator pos = impacts_.begin(), stop = impacts_.end(); pos != stop; ++pos) {
      if (label == pos->first) {
         return true; 
      }
   }
   return false;
}

int InjScenario::EarliestInjectionTimestep(const MerlionModelContainer& model) const
{
   /*
   Returns the timestep of the earliest ToxinInjection.
   If the InjectionList is empty, returns 0
   */ 
   return Injection::SecondsToTimestep(EarliestInjectionTimeSeconds(), model.qual_step_minutes);
}

double InjScenario::EarliestInjectionTimeSeconds() const
{
   /*
   Returns the time (seconds) of the earliest ToxinInjection.
   If the InjectionList is empty, returns 0
   */ 
   if (!injections_.empty()) {
      double min_time = injections_.front()->StartTimeSeconds();
      for (InjectionList::const_iterator p_inj = injections_.begin(), inj_end = injections_.end(); p_inj != inj_end; ++p_inj) {
         if ((*p_inj)->StartTimeSeconds() < min_time) {
            min_time = (*p_inj)->StartTimeSeconds(); 
         }
      }
      return min_time;
   }
   return -1;
}

int InjScenario::LatestInjectionTimestep(const MerlionModelContainer& model) const
{
   /*
   Returns the timestep of the latest ToxinInjection.
   If the InjectionList is empty, returns 0
   */ 
   return Injection::SecondsToTimestep(LatestInjectionTimeSeconds(), model.qual_step_minutes);
}

double InjScenario::LatestInjectionTimeSeconds() const
{
   /*
   Returns the time (seconds) of the latest ToxinInjection.
   If the InjectionList is empty, returns 0
   */ 
   if (!injections_.empty()) {
      double max_time = injections_.front()->StopTimeSeconds();
      for (InjectionList::const_iterator p_inj = injections_.begin(), inj_end = injections_.end(); p_inj != inj_end; ++p_inj) {
         if ((*p_inj)->StopTimeSeconds() > max_time) {
            max_time = (*p_inj)->StopTimeSeconds(); 
         }
      }
      return max_time;
   }
   return -1;
}

void InjScenario::Print(std::ostream& out, std::string format/*=""*/, std::string offset/*=""*/) const
{
   if (format == "yaml") { // yaml format
      out << offset    << "- Name: " << name_ << "\n";
      out << offset    << "  Injections:\n";
      for (InjectionList::const_iterator p_inj = injections_.begin(), inj_end = injections_.end(); p_inj != inj_end; ++p_inj) {
         out << offset << "  - NodeName:      " << (*p_inj)->NodeName() << "\n" <<
            offset << "    StartTime: {Value: " << (*p_inj)->StartTimeSeconds() << ", Units: s}\n" << 
            offset << "    StopTime:  {Value: " << (*p_inj)->StopTimeSeconds()  << ", Units: s}\n" <<
            offset << "    InjectionType: " << InjTypeToString((*p_inj)->Type()) << "\n";
         std::string units = ""	 ;
         if ((*p_inj)->Type() == InjType_Mass) {
            units = "g/min";
         }
         else if ((*p_inj)->Type() == InjType_Flow) {
            units = "g/m^3";
         }
         out << offset << "    Strength:  {Value: " << (*p_inj)->Strength()         << ", Units: " << units << "}\n";
      }
      if (isDetected()) {
         out << offset << "  DetectionTime:\n" << 
            "    Value:  " << detection_time_seconds_ << "\n" <<
            "    Units:  s\n";
      }
      else {
         out << offset << "  DetectionTime:\n" << 
            "    Value:  null\n"  <<
            "    Units:  s\n";
      }
      for (std::map<std::string, float>::const_iterator pos = impacts_.begin(), stop = impacts_.end(); pos != stop; ++pos) {
         out << offset << "  " << pos->first << ": " << pos->second << "\n"; 
      }
   }
   else if (format == "json") { //json format
      out << offset    << "{\"Name\": " << name_ << ",\n";      
      out << offset    << " \"Injections\": [\n";
      for (InjectionList::const_iterator p_inj = injections_.begin(), inj_end = injections_.end(); p_inj != inj_end; ++p_inj) {
         out << offset << "  {\"NodeName\":      \"" << (*p_inj)->NodeName() << "\",\n" <<
            "    \"StartTime\":     {\"Value\": " << (*p_inj)->StartTimeSeconds() << ", \"Units\": \"s\"},\n" << 
            "    \"StopTime\":      {\"Value\": " << (*p_inj)->StopTimeSeconds()  << ", \"Units\": \"s\"},\n" <<
            "    \"InjectionType\": \"" << InjTypeToString((*p_inj)->Type()) << "\",\n";
         std::string units = ""	 ;
         if ((*p_inj)->Type() == InjType_Mass) {
            units = "g/min";
         }
         else if ((*p_inj)->Type() == InjType_Flow) {
            units = "g/m^3";
         }
         out << offset << "   \"Strength\":      {\"Value\": " << (*p_inj)->Strength()         << ", \"Units\": \"" << units << "\"}},\n";
      }
      out.seekp(-2, std::ios::end);
      out << "\n";
      out << offset <<    " ],\n";
      if (isDetected()) {
         out << offset << " \"DetectionTime\":   {\"Value\":   " << detection_time_seconds_ << ", \"Units\": \"s\"},\n";
      }
      else {
         out << offset << " \"DetectionTime\":   {\"Value\":   null, \"Units\": \"s\"},\n";
      }
      for (std::map<std::string, float>::const_iterator pos = impacts_.begin(), stop = impacts_.end(); pos != stop; ++pos) {
         out << offset    << " \"" << pos->first << "\": " << pos->second << ",\n";      
      }
      out.seekp(-2,std::ios::end);
      out << "\n";
      out << offset <<    "}\n";
   }
   else { //default text file format
      out << "Scenario Summary:\n";
      out << "\tName - " << name_ << "\n";
      if (injections_.empty()) {
         out << "\tNo Injections\n";
      }
      else {
         int cntr(0);
         for (InjectionList::const_iterator p_inj = injections_.begin(), inj_end = injections_.end(); p_inj != inj_end; ++p_inj) {
            out << "\tInjection " << ++cntr << ":\n";
            out << "\t\tNode Name:        " << (*p_inj)->NodeName() << "\n";
            out << "\t\tStart Time (s):   " << (*p_inj)->StartTimeSeconds() << "\n";
            out << "\t\tStop Times (s):   " << (*p_inj)->StopTimeSeconds() << "\n";
            out << "\t\tInjection Type:   " << InjTypeToString((*p_inj)->Type()) << "\n";
            std::string units = ""	 ;
            if ((*p_inj)->Type() == InjType_Mass) {
               units = "g/min";
            }
            else if ((*p_inj)->Type() == InjType_Flow) {
               units = "g/m^3";
            }
            out << "\t\tStrength (" << units << "): " << (*p_inj)->Strength() << "\n";
         }
      }
      if (isDetected()) {
         out << "\tDetection Time (s): " << detection_time_seconds_ << "\n";
      }
      else {
         out << "\tDetection Time (s):  None \n";
      }
      for (std::map<std::string, float>::const_iterator pos = impacts_.begin(), stop = impacts_.end(); pos != stop; ++pos) {
         out << "\t" << pos->first << ": " << pos->second << "\n";      
      }
   }
}

void InjScenario::Print(std::ostream& out, const MerlionModelContainer& model) const
{
   out << "Scenario Summary:\n";
   out << "\tName - " << name_ << "\n";
   if (injections_.empty()) {
      out << "\tNo Injections\n";
   }
   else {
      int cntr(0);
      for (InjectionList::const_iterator p_inj = injections_.begin(), inj_end = injections_.end(); p_inj != inj_end; ++p_inj) {
         out << "\tInjection " << ++cntr << ":\n";
         out << "\t\tNode Name:        " << (*p_inj)->NodeName() << "\n";
         out << "\t\tStart Timestep:   " << (*p_inj)->StartTimestep(model) << "\n";
         out << "\t\tStop Timestep:    " << (*p_inj)->StopTimestep(model) << "\n";
         out << "\t\tInjection Type:   " << InjTypeToString((*p_inj)->Type()) << "\n";
         std::string units = ""	 ;
         if ((*p_inj)->Type() == InjType_Mass) {
            units = "g/min";
         }
         else if ((*p_inj)->Type() == InjType_Flow) {
            units = "g/m^3";
         }
         out << "\t\tStrength (" << units << "): " << (*p_inj)->Strength() << "\n";
      }
   }
   if (isDetected()) {
      out << "\tDetection Timestep: " << DetectionTimestep(model) << "\n";
   }
   else {
      out << "\tDetection Timestep: None \n";
   }
   for (std::map<std::string, float>::const_iterator pos = impacts_.begin(), stop = impacts_.end(); pos != stop; ++pos) {
      out << "\t" << pos->first << ": " << pos->second << "\n";      
   }
}

void PrintScenarioList(const InjScenList& scenario_list, std::ostream& out)
{
   for (InjScenList::const_iterator pos = scenario_list.begin(), stop = scenario_list.end(); pos != stop; ++pos) {
      (*pos)->Print(out);
   }
}
void PrintScenarioListYAML(const InjScenList& scenario_list, std::ostream& out)
{
   for (InjScenList::const_iterator pos = scenario_list.begin(), stop = scenario_list.end(); pos != stop; ++pos) {
      (*pos)->Print(out,"yaml");
   }
}
void PrintScenarioListJSON(const InjScenList& scenario_list, std::ostream& out)
{
   out <<    "[\n";
   std::string offset = " "; 
   for (InjScenList::const_iterator pos = scenario_list.begin(), stop = scenario_list.end(); pos != stop; ++pos) {   
      (*pos)->Print(out,"json",offset);
      out.seekp(-1, std::ios::end);
      out << ",\n";
   }
   out.seekp(-2, std::ios::end);
   out << "\n]\n";
}

} // end of merlionUtils namespace

