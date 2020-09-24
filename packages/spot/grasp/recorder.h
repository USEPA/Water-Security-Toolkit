// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/*****************************************************************
 *
 * PMRecorder: stores the best value obtained by an iteractive 
 * algorithm --- with solution value, iteration number and time
 *
 ****************************************************************/

#ifndef RECORDER_H
#define RECORDER_H

class PMRecorder {
 public: 
  int best_iteration;
  double best_time;
  double best_value;

  PMRecorder () {
    best_time = 0;
    best_iteration = 0;
    best_value = POPSTAR_INFINITY;
  }

  bool report (int iteration, double time, double value) {
    if (value < best_value) {
      best_value = value;
      best_time = time;
      best_iteration = iteration;
      return true;
    }
    return false;
  }
};

#endif
