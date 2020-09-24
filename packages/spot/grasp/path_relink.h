// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#ifndef PATH_RELINK_H
#define PATH_RELINK_H

#include "solution.h"
#include "elite.h"

#include <vector>

class PMRelink {
 private:
  static void fatal (const char *func, const char *msg);

 public:
  static double combinePR(PMSolution *t, PMSolution *s1, PMSolution *s2, const std::vector<int> &facilityLocationConstraints);
  //static double combineSub(PMSolution *t, PMSolution *s1, PMSolution *s2);	
  static double combineSubPR(PMSolution *t, PMSolution *s1, PMSolution *s2, const std::vector<int> &facilityLocationConstraints);
  static double combinePartial(PMSolution *t, const std::vector<int> &facilityLocationConstraints, PMSolution *s1, PMSolution *s2);
  static double combine_r(PMSolution *t, PMSolution *s1, PMSolution *s2, const char* method, const std::vector<int> &facilityLocationConstraints);
  static double combine(PMSolution *t, const std::vector<int> &facilityLocationConstraints, PMSolution *s1, PMSolution *s2, const char* method);
  //static double combine2(PMSolution *t, PMSolution *s1, PMSolution *s2);
  static void pathRelinking(PMSolution *best_s, PMElite *elite, const char *method, const std::vector<int> &facilityLocationConstraints, bool stats = false);
};

#endif
