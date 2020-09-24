#ifndef MEASURE_NETWORK_SIMULATOR_HPP__
#define MEASURE_NETWORK_SIMULATOR_HPP__

#include <merlionUtils/NetworkSimulator.hpp>

#include <iostream>
#include <fstream>
#include <stack>
#include <sstream>

class MeasureGenNetworkSimulator : public merlionUtils::NetworkSimulator
{
public:

   MeasureGenNetworkSimulator(bool disable_warnings=false)
     :
     NetworkSimulator(disable_warnings)
   {
   }

   virtual ~MeasureGenNetworkSimulator()
   {
      clear();
   }

   void clear(); //hides the base class clear()   
};

#endif
