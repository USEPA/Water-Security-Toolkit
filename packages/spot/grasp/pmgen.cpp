// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#include <stdio.h>
#include <stdlib.h>
#include "bossa_random.h"

#include <iostream>
#include <iomanip>


void showUsage(char **argv) 
{
  std::cout << "Usage: " << argv[0] << " <customers> <facilities> <seed>" << std::endl;
  exit(-1);
}


int main (int argc, char *argv[]) 
{
  if(argc!=4)
    {
      showUsage(argv);
    }

  int n(atoi(argv[1]));
  int m(atoi(argv[2]));
  int s(atoi(argv[3]));

  BossaRandom::randomize(s);
  std::cout << "p " << n << " " << m << std::endl;
  std::cout << "s " << s << std::endl;
  for (int i=1; i<=n; i++) 
    {
      for (int j=1; j<=n; j++) 
	{
	  std::cout << "a " << i << " " << j << " " << BossaRandom::getInteger(1, n) << std::endl;
	}
    }
  return 0;
}
