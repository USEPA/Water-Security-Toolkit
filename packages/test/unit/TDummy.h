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

#include <cxxtest/TestSuite.h>
#include <cxxtest/ValueTraits.h>
#include <cxxtest/StdValueTraits.h>
#include <utilib/Any.h>
#include <iostream>

class TestAny : public CxxTest::TestSuite
{
public:


   /// empty any
   void test_default_constructor()
   {
      utilib::Any a;
      TS_ASSERT(a.empty());
      TS_ASSERT(!a.is_reference());
      TS_ASSERT(!a.is_immutable());
   }
};
