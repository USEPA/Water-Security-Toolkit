#ifndef MERLION_UTILS_TASKTIMER_HPP__
#define MERLION_UTILS_TASKTIMER_HPP__

#include <time.h>
#include <iostream>

namespace merlionUtils {

class TaskTimer
{
public:
   TaskTimer()
   {
      Restart();
   }

   TaskTimer(bool start)
   {
      if (start) {Restart();}
      else {
	 active = false;
	 t = 0.0;
	 lap = 0.0;
      }
   }

   void Start()
   {
      active = true;
      s = clock();
   }

   void Stop()
   {
      e = clock();
      if (active) {
	 lap = (static_cast<double>(e-s))/CLOCKS_PER_SEC;
	 t += lap;
      }
      active = false;
   }

   void Restart()
   {
      t = 0.0;
      lap = 0.0;
      Start();
   }

   void Print(std::ostream& out, std::string tag, int n=1)
   {
      Stop();
      out<< tag << " (time in seconds): " << t/n << std::endl;
      Start();
   }

   void StopAndPrint(std::ostream& out, std::string tag, int n=1)
   {
      Stop();
      out<< tag << " (time in seconds): " << t/n << std::endl;
   }

   double Status(int n=1)
   {
      bool was_active = active;
      Stop();
      if (was_active) {Start();}
      return static_cast<double>(t/n);
   }

   double LastLap(int n = 1)
   {
      return static_cast<double>(lap/n);
   }

   template <typename T>
   void Save(T& time, int n=1)
   {
      bool was_active = active;
      Stop();
      time = static_cast<T>(t/n);
      if (was_active) {Start();}
   }

   template <typename T>
   void StopAndSave(T& time, int n=1)
   {
      Stop();
      time = static_cast<T>(t/n);
   }

private:
   clock_t s;
   clock_t e;
   double t;
   double lap;
   bool active;
};

} /* end of merlionUtils namespace */

#endif
