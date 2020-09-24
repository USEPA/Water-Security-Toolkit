// Test suite for the Merlion TSG file reader
#include <cxxtest/TestSuite.h>

#include <merlionUtils/TSG_Reader.hpp>

#include <algorithm>
#include <vector>
#include <map>
#include <set>

using namespace merlionUtils;

namespace {

} // end of empty namespace

///////////////////////////// Begin Testing Suits
class TSG_ReaderTestSuite : public CxxTest::TestSuite
{
 public:

   Merlion *model_; 
   
   static TSG_ReaderTestSuite* createSuite()
   {
      TSG_ReaderTestSuite *suite(new TSG_ReaderTestSuite);
      suite->model_ = new Merlion;
      std::ifstream in;
      in.open("Net3_24h_15min.wqm", std::ios::in);
      if (!in.good()) {
	 TS_FAIL("Problem Reading Test File");
      }
      bool binary=false;
      suite->model_->ReadWQM(in,binary);
      in.close();
      return suite;
   }

   static void destroySuite(TSG_ReaderTestSuite* suite)
   {
      suite->model_->reset();
      delete suite->model_;
      suite->model_ = NULL;
      delete suite;
      suite = NULL;
   }

   void test_mass_single_line() {
      InjScenList scenarios;
      ReadTSG("Net3_Mass_Single.tsg", model_, scenarios);
      TS_ASSERT(scenarios.size() == 1);
   }

   void test_flowpaced_single_line() {
      InjScenList scenarios;
      ReadTSG("Net3_Flowpaced_Single.tsg", model_, scenarios);
      TS_ASSERT(scenarios.size() == 1);
   }

   void test_multiline() {
      InjScenList scenarios;
      ReadTSG("multiline.tsg", model_, scenarios);
      TS_ASSERT(scenarios.size() != 0);
   }

   void test_multiline_with_comments() {
      InjScenList scenarios;
      ReadTSG("multiline_comments.tsg", model_, scenarios);
      TS_ASSERT(scenarios.size() != 0);
   }

   void test_single_species_bio() {
      InjScenList scenarios;
      ReadTSG("bio.tsg", model_, scenarios);
      TS_ASSERT(scenarios.size() != 0);
   }

};
