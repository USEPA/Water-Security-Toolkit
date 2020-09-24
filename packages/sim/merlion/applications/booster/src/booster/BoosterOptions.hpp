#ifndef BOOSTER_BOOSTEROPTIONS_HPP__
#define BOOSTER_BOOSTEROPTIONS_HPP__

#include <utilib/OptionParser.h>

class BoosterOptions
{
public:

   BoosterOptions();
   virtual ~BoosterOptions() {}
   virtual void ParseInputs(int argc, char** argv)=0;
   virtual void PrintSummary(std::ostream& out)=0;

   bool logging;
   bool output_merlion_labels;
   bool disable_warnings;
   bool ignore_merlion_warnings;
   std::string nodemap_filename;
   std::string output_prefix;
   float sim_duration_min;
   float qual_step_min;

protected:
   utilib::OptionParser option_parser_;
};

#endif
