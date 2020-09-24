/*  _________________________________________________________________________
 *
 *  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README.txt file in the top SPOT directory.
 *  _________________________________________________________________________
 */

#ifdef HAVE_CONFIG_H
#include <teva_config.h>
#endif

#include <iostream>
#include <vector>

#include "sp/VecTrie.h"

using namespace std;

main()
{

   VecTrie<int, vector<int> > a, *ap = &a;


   VecTrieKey<int> key1, key2, key3;
   vector<int> v1, v2, data0;
   int d0 = 10;
//  int d1 = 11;
//  int d2 = 12;

   v1.push_back(2);
   v1.push_back(-1);
   data0.push_back(10);
   data0.push_back(20);
   data0.push_back(30);
   data0.push_back(40);
   VecTrieKey<int> key_(v2);

   if (ap->getnode(key_) == NULL)
      cout << "good" << endl;
   v1[0] = 7;      // 7234
   key2 = v1;      // 7234

   v2 = key_.getkey();   // 1234
   v2.push_back(7);      // 1274
   /*
     key3 = v2;      // 1274

     cout << "v1:" << v1[0] << v1[1] << v1[2] << v1[3] << endl;
     cout << "v2:" << v2[0] << v2[1] << v2[2] << v2[3] << endl;
     cout << "k1:" << key1[0] << key1[1] << key1[2] << key1[3] << endl;
     cout << "k2:" << key2[0] << key2[1] << key2[2] << key2[3] << endl;
     cout << "k3:" << key3[0] << key3[1] << key3[2] << key3[3] << endl;

     a.insert(key1, data0);

     vector<int>& dref = a.getdata(key1);
     dref.push_back(50);
     vector<int>& dref2 = a.getdata(key1);
     for (int i=0; i<dref2.size(); i++)
      cout << "data[" << i << "]: " << dref2[i] << endl;


     //a.dumpnode(a.headnode());
   */
   return 0;
}
