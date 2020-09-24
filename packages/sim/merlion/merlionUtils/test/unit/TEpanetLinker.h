// Test suite for the CreateMerlionFromEpanet function
#include <cxxtest/TestSuite.h>

#include <merlion/Merlion.hpp>
#include <merlionUtils/EpanetLinker.hpp>

#include <fstream>

using namespace merlionUtils;

namespace {

const double _AbsEqualTol = 1e-6;
const double _RelEqualTol = 1e-1;

template <typename T, typename SIZE_T>
bool _ArraysAreAbsEqual(const T& x, const T& y, SIZE_T N)
{
   // Check array values are the same
   for (SIZE_T i = 0; i < N; ++i) {
      if (x[i] != y[i]) {
         std::cout.precision(10);
         std::cout << x[i] << " != " << y[i] << std::endl;
         std::cout.precision();         
         return false;
      }
   }
   return true;
}

template <typename T, typename SIZE_T>
bool _ArraysAreAbsEqual(const T& x, const T& y, SIZE_T N, double tol, double power=1.0)
{
   // Check array values are the same
   for (SIZE_T i = 0; i < N; ++i) {
      if (x[i] != y[i]) {
         if (power == 1.0) {
            if (fabs(x[i]-y[i]) > tol) {
               std::cout.precision(10);
               std::cout << x[i] << " " << y[i] << std::endl;
               std::cout << fabs(x[i]-y[i]) << " > " << tol << std::endl;
               std::cout.precision();
               return false;
            }
         }
         else {
            if(pow(fabs(x[i]-y[i]), power) > tol) {
               std::cout.precision(10);
               std::cout << x[i] << " " << y[i] << std::endl;
               std::cout << fabs(pow(x[i]-y[i], power)) << " > " << tol << std::endl;
               std::cout.precision();
               return false;
            } 
         }
      }
   }
   return true;
}

template <typename T, typename SIZE_T>
bool _ArraysAreRelEqual(const T& x, const T& y, SIZE_T N, double rel_tol, double power=1.0)
{
   // Check array values are the same
   for (SIZE_T i = 0; i < N; ++i) {
      if (x[i] != y[i]) {
         double denom = (x[i] == 0)?(1.0):(x[i]);
         if (power == 1.0) {
            if (fabs(x[i]-y[i])/denom > rel_tol) {
               std::cout.precision(10);
               std::cout << x[i] << " " << y[i] << std::endl;
               std::cout << fabs(x[i]-y[i])/denom << " > " << rel_tol << std::endl;
               std::cout.precision();
               return false;
            }
         }
         else {
            if(pow(fabs(x[i]-y[i])/denom, power) > rel_tol) {
               std::cout.precision(10);
               std::cout << x[i] << " " << y[i] << std::endl;
               std::cout << pow(fabs(x[i]-y[i])/denom, power) << " > " << rel_tol << std::endl;
               std::cout.precision();
               return false;
            } 
         }
      }
   }
   return true;
}

bool diff_merlion_models(Merlion& m1, Merlion& m2) 
{

   if (m1.NNodes() != m2.NNodes()) {
      std::cout << "NNodes() differ" << std::endl;
      return false;
   }

   if (m1.NSteps() != m2.NSteps()) {
      std::cout << "NSteps() differ" << std::endl;
      return false;
   }
   if (m1.Stepsize_min() != m2.Stepsize_min()) {
      std::cout << "Stepsize_min() differ" << std::endl;
      return false;
   }
   int n_nodes = m1.NNodes();
   int n_steps = m1.NSteps();
   int N = n_nodes*n_steps;

   if (!::_ArraysAreAbsEqual(m1.NodeIdxNameMap(),m2.NodeIdxNameMap(), n_nodes)) {
      std::cout << "NodeIdxNameMap() differ" << std::endl;
      return false;
   }
   if (!::_ArraysAreAbsEqual(m1.NodeIdxTypeMap(),m2.NodeIdxTypeMap(), n_nodes)) {
      std::cout << "NodeIdxTypeMap() differ" << std::endl;
      return false;
   }
   if (m1.LinkIdxNameMap().size() != m2.LinkIdxNameMap().size()) {
      std::cout << "LinkIdxNameMap().size() differ" << std::endl;
      return false;
   }
   if (!::_ArraysAreAbsEqual(m1.LinkIdxNameMap(),m2.LinkIdxNameMap(), m1.LinkIdxNameMap().size())) {
      std::cout << "LinkIdxNameMap() differ" << std::endl;
      return false;
   }
   if (m1.LinkIdxTypeMap().size() != m2.LinkIdxTypeMap().size()) {
      std::cout << "LinkIdxTypeMap().size() differ" << std::endl;
      return false;
   }
   if (!::_ArraysAreAbsEqual(m1.LinkIdxTypeMap(),m2.LinkIdxTypeMap(), m1.LinkIdxTypeMap().size())) {
      std::cout << "LinkIdxTypeMap() differ" << std::endl;
      return false;
   }
   int n_tanks = 0;
   for (int n = 0; n < n_nodes; ++n) {
      if (m1.NodeIdxTypeMap()[n] == NodeType_Tank) {
         n_tanks++;
      }
   }
   if (!::_ArraysAreAbsEqual(m1.GetVelocities_mpmin(),m2.GetVelocities_mpmin(), m1.LinkIdxNameMap().size()*n_steps, ::_AbsEqualTol)) {
      std::cout << "GetVelocities_mpmin() differ" << std::endl;
      return false;
   }
   if (!::_ArraysAreAbsEqual(m1.GetNodeDemands_m3pmin(),m2.GetNodeDemands_m3pmin(), N, ::_AbsEqualTol)) {
      std::cout << "GetNodeDemands_m3pmin() differ" << std::endl;
      return false;
   }
   /*
   for (int idx = 0; idx < n_tanks*n_steps; ++idx) {
      std::cout << m1.GetTankVolumes_m3()[idx] << " " << m2.GetTankVolumes_m3()[idx] << std::endl;
   }
   */
   if (!::_ArraysAreAbsEqual(m1.GetTankVolumes_m3(),m2.GetTankVolumes_m3(), n_tanks*n_steps, ::_AbsEqualTol)) {
      std::cout << "GetTankVolumes_m3() differ" << std::endl;
      return false;
   }
   if (!::_ArraysAreAbsEqual(m1.GetFlowAcrossNodes_m3pmin(),m2.GetFlowAcrossNodes_m3pmin(), N, ::_AbsEqualTol)) {
      std::cout << "GetFlowAcrossNodes_m3pmin() differ" << std::endl;
      return false;
   }
   const SparseMatrix& m1G = *m1.GetLHSMatrix();
   const SparseMatrix& m2G = *m2.GetLHSMatrix();
   if (m1G.isCSRMatrix() != m2G.isCSRMatrix()) {
      std::cout << "isCSRMatrix() differ" << std::endl;
      return false;
   }
   if (m1G.isCSCMatrix() != m2G.isCSCMatrix()) {
      std::cout << "isCSCMatrix() differ" << std::endl;
      return false;
   }
   if (m1G.isCOOMatrix() != m2G.isCOOMatrix()) {
      std::cout << "isCOOMatrix() differ" << std::endl;
      return false;
   }
   if (m1G.NRows() != m2G.NRows()) {
      std::cout << "NRows() differ" << std::endl;
      return false;
   }
   if (m1G.NCols() != m2G.NCols()) {
      std::cout << "NCols() differ" << std::endl;
      return false;
   }
   /*
   if (m1G.NNonzeros() != m2G.NNonzeros()) {
      std::cout << m1G.NNonzeros() << " != " << m2G.NNonzeros() << std::endl;
      std::cout << "NNonzeros() differ" << std::endl;
      return false;
   }
   if (!::_ArraysAreAbsEqual(m1G.Values(),m2G.Values(), m1G.NNonzeros(), ::_AbsEqualTol)) {
      std::cout << "G.Values() differ" << std::endl;
      return false;
   }
   if (m1G.isCSCMatrix() || m1G.isCOOMatrix()) {
      if (!::_ArraysAreAbsEqual(m1G.iRows(),m2G.iRows(), m1G.NNonzeros())) {
         std::cout << "G.iRows() differ" << std::endl;
         return false;
      }
   }
   if (m1G.isCSRMatrix() || m1G.isCOOMatrix()) {
      if (!::_ArraysAreAbsEqual(m1G.jCols(),m2G.jCols(), m1G.NNonzeros())) {
         std::cout << "G.jCols() differ" << std::endl;
         return false;
      }
   }
   if (m1G.isCSRMatrix()) {
      if (!::_ArraysAreAbsEqual(m1G.pRows(),m2G.pRows(), m1G.NRows()+1)) {
         std::cout << "G.pRows() differ" << std::endl;
         return false;
      }
   }
   if (m1G.isCSCMatrix()) {
      if (!::_ArraysAreAbsEqual(m1G.pCols(),m2G.pCols(), m1G.NCols()+1)) {
         std::cout << "G.pCols() differ" << std::endl;
         return false;
      }
   }
   */
   if (!::_ArraysAreAbsEqual(m1.GetRHSDiagMatrix(),m2.GetRHSDiagMatrix(), N)) {
      std::cout << "GetRHSDiagMatrix() differ" << std::endl;
      return false;
   }
   if (!::_ArraysAreAbsEqual(m1.GetPermuteNTtoUT(),m2.GetPermuteNTtoUT(), N)) {
      std::cout << "GetPermuteNTtoUT() differ" << std::endl;
      return false;
   }
   if (!::_ArraysAreAbsEqual(m1.GetPermuteUTtoNT(),m2.GetPermuteUTtoNT(), N)) {
      std::cout << "GetPermuteUTtoNT() differ" << std::endl;
      return false;
   }

   return true;
}

} // end of empty namespace

///////////////////////////// Begin Testing Suits
class CreateMerlionFromEpanetTestSuite : public CxxTest::TestSuite
{
 public:
   
   Merlion *model_; 
   
   static CreateMerlionFromEpanetTestSuite* createSuite()
   {
      CreateMerlionFromEpanetTestSuite *suite(new CreateMerlionFromEpanetTestSuite);
      suite->model_ = new Merlion;
      std::ifstream in;
      in.open("linearNetwork.wqm", std::ios::in);
      if (!in.good()) {
	 TS_FAIL("Problem Reading Test Baseline File");
      }
      bool binary=false;
      suite->model_->ReadWQM(in,binary);
      in.close();

      return suite;
   }

   static void destroySuite(CreateMerlionFromEpanetTestSuite* suite)
   {
      suite->model_->reset();
      delete suite->model_;
      suite->model_ = NULL;
      delete suite;
      suite = NULL;
   }

   void units_test_method(std::string test_file) {
      Merlion *test_model(NULL);
      test_model = CreateMerlionFromEpanet((char*)test_file.c_str(),"junk.rpt","",-1.0,-1.0,-1.0,-1,true,-1.0);
      if (model_ && test_model) {
         TS_ASSERT(::diff_merlion_models(*model_,*test_model) == true);
      }
      else {
         TS_FAIL("Failed to create one or more Merlion models for comparison");
      }
      delete test_model;
      test_model = NULL;
   }

   void test_INP_GPM_orig_units() { units_test_method("linearNetwork.inp"); }
   void test_INP_GPM_units() { units_test_method("linearNetwork.inp.GPM.inp"); }
   void test_INP_AFD_units() { units_test_method("linearNetwork.inp.AFD.inp"); } 
   void test_INP_CFS_units() { units_test_method("linearNetwork.inp.CFS.inp"); } 
   void test_INP_CMD_units() { units_test_method("linearNetwork.inp.CMD.inp"); } 
   void test_INP_CMH_units() { units_test_method("linearNetwork.inp.CMH.inp"); } 
   void test_INP_IMGD_units() { units_test_method("linearNetwork.inp.IMGD.inp"); } 
   void test_INP_LPM_units() { units_test_method("linearNetwork.inp.LPM.inp"); } 
   void test_INP_LPS_units() { units_test_method("linearNetwork.inp.LPS.inp"); } 
   void test_INP_MGD_units() { units_test_method("linearNetwork.inp.MGD.inp"); } 
   void test_INP_MLD_units() { units_test_method("linearNetwork.inp.MLD.inp"); } 

};
