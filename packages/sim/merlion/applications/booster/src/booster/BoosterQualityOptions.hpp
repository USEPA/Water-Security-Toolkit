#ifndef BOOSTER_BOOSTERQUALITYOPTIONS_HPP__
#define BOOSTER_BOOSTERQUALITYOPTIONS_HPP__

#include "BoosterOptions.hpp"

class BoosterQualityOptions : public BoosterOptions
{
public:
   BoosterQualityOptions();
   virtual ~BoosterQualityOptions() {}
   virtual void ParseInputs(int argc, char** argv);
   virtual void PrintSummary(std::ostream& out);
   virtual bool isDefault(std::string option_name);

   std::string epanet_output_filename;
   std::string merlion_save_filename;
   std::string inp_filename;
   std::string wqm_filename;
   std::string booster_filename;
   float decay_k;

};

#endif
