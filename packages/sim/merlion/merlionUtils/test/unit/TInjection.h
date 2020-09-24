// Test suite for the Merlion upper triangular linear solver
#include <cxxtest/TestSuite.h>

#include <merlionUtils/NetworkSimulator.hpp>

#include <algorithm>
#include <vector>
#include <map>
#include <set>

using namespace merlionUtils;

namespace {

const double _EqualAbsTol = 1e-4;

template <typename T>
T _Sum(T* x, int step, int N, float scale=1.0)
{
   // Sum array
   double sum = 0.0;
   for (int i = 0; i < N; ++i) {
      sum += (*x)*scale;
      x += step;
   }
   return sum;
}

template <typename T>
T _Sum(T* x, int N, float scale=1.0)
{
   return _Sum(x,1,N,scale);
}

template <typename T, typename SIZE_T>
bool _ArraysAreEqual(T* x, int stepx, T* y, int stepy, SIZE_T N, double tol=0.0, double power=1.0)
{
   // Check array values are the same
   for (SIZE_T i = 0; i < N; ++i) {
      if(fabs(pow((*x)-(*y), power)) > tol) {
	 std::cout.precision(10);
	 std::cout << fabs(pow((*x)-(*y), power)) << " > " << tol << std::endl;
	 std::cout.precision();
         return false;
      } 
      x += stepx;
      y += stepy;
   }
   return true;
}

template <typename T, typename SIZE_T>
bool _ArraysAreEqual(T* x,T* y, SIZE_T N, double tol=0.0, double power=1.0)
{
   return _ArraysAreEqual(x,1,y,1,N,tol,power);
}

} // end of empty namespace

///////////////////////////// Begin Testing Suits
class InjectionTestSuite : public CxxTest::TestSuite
{
 public:

   NetworkSimulator *network_; 
   
   static InjectionTestSuite* createSuite()
   {
      InjectionTestSuite *suite(new InjectionTestSuite);
      suite->network_ = new NetworkSimulator;
      bool is_binary = false;
      suite->network_->ReadWQMFile("Net3_24h_15min.wqm", is_binary);
      return suite;
   }

   static void destroySuite(InjectionTestSuite* suite)
   {
      suite->network_->clear();
      delete suite->network_;
      suite->network_ = NULL;
      delete suite;
      suite = NULL;
   }

   const PInjection& extract_single_injection(const NetworkSimulator* net) {
      const InjScenList& scenarios = net->InjectionScenarios();
      TS_ASSERT(!scenarios.empty());
      const InjectionList& injections = scenarios.front()->Injections();
      TS_ASSERT(!injections.empty());
      return injections.front();
   }

   void test_MassInjected_TypeMass() {
      const MerlionModelContainer& model = network_->Model();
      float m(0.0);
      network_->ReadTSGFile("Net3_Mass_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Mass);
      m = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(m != 0.0);
   }

   void test_MassInjected_TypeFlowpaced() {
      const MerlionModelContainer& model = network_->Model();
      float m(0.0);
      network_->ReadTSGFile("Net3_Flowpaced_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Flow);
      m = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(m != 0.0);
   }

   void test_SetArray_FloatPoiner_TypeMass() {
      const MerlionModelContainer& model = network_->Model();
      float *x1(new float[model.N]()), *x2(new float[model.N]());
      float mass_inj_g(0.0);
      int min_row_idx(0);
      float mass_injected_g(0.0);
      network_->ReadTSGFile("Net3_Mass_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Mass);
      extract_single_injection(network_)->SetArray(model,x1);
      extract_single_injection(network_)->SetArray(model,x2,mass_inj_g,min_row_idx);
      mass_injected_g = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(mass_injected_g != 0.0);
      TS_ASSERT(::_ArraysAreEqual(x1,x2,model.N));
      float array_mass = ::_Sum(x1,model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass,mass_injected_g, ::_EqualAbsTol);
      TS_ASSERT_DELTA(mass_inj_g,mass_injected_g, ::_EqualAbsTol);
   }

   void test_SetArray_FloatPoiner_TypeFLowpaced() {
      const MerlionModelContainer& model = network_->Model();
      float *x1(new float[model.N]()), *x2(new float[model.N]());
      float mass_inj_g(0.0);
      int min_row_idx(0);
      float mass_injected_g(0.0);
      network_->ReadTSGFile("Net3_Flowpaced_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Flow);
      extract_single_injection(network_)->SetArray(model,x1);
      extract_single_injection(network_)->SetArray(model,x2,mass_inj_g,min_row_idx);
      mass_injected_g = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(mass_injected_g != 0.0);
      TS_ASSERT(::_ArraysAreEqual(x1,x2,model.N));
      float array_mass = ::_Sum(x1,model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass,mass_injected_g, ::_EqualAbsTol);
      TS_ASSERT_DELTA(mass_inj_g,mass_injected_g, ::_EqualAbsTol);
   }

   void test_SetArray_FloatVector_TypeMass() {
      const MerlionModelContainer& model = network_->Model();
      std::vector<float> x1(model.N,0.0), x2(model.N,0.0);
      float mass_inj_g(0.0);
      int min_row_idx(0);
      float mass_injected_g(0.0);
      network_->ReadTSGFile("Net3_Mass_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Mass);
      extract_single_injection(network_)->SetArray(model,x1);
      extract_single_injection(network_)->SetArray(model,x2,mass_inj_g,min_row_idx);
      mass_injected_g = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(mass_injected_g != 0.0);
      TS_ASSERT(::_ArraysAreEqual(&(x1[0]),&(x2[0]),model.N));
      float array_mass = ::_Sum(&(x1[0]),model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass,mass_injected_g, ::_EqualAbsTol);
      TS_ASSERT_DELTA(mass_inj_g,mass_injected_g, ::_EqualAbsTol);
   }

   void test_SetArray_FloatVector_TypeFlowpaced() {
      const MerlionModelContainer& model = network_->Model();
      std::vector<float> x1(model.N,0.0), x2(model.N,0.0);
      float mass_inj_g(0.0);
      int min_row_idx(0);
      float mass_injected_g(0.0);
      network_->ReadTSGFile("Net3_Flowpaced_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Flow);
      extract_single_injection(network_)->SetArray(model,x1);
      extract_single_injection(network_)->SetArray(model,x2,mass_inj_g,min_row_idx);
      mass_injected_g = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(mass_injected_g != 0.0);
      TS_ASSERT(::_ArraysAreEqual(&(x1[0]),&(x2[0]),model.N));
      float array_mass = ::_Sum(&(x1[0]),model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass,mass_injected_g, ::_EqualAbsTol);
      TS_ASSERT_DELTA(mass_inj_g,mass_injected_g, ::_EqualAbsTol);
   }

   void test_SetArray_FloatMap_TypeMass() {
      const MerlionModelContainer& model = network_->Model();
      std::map<int,float> x1, x2;
      std::vector<float> v1(model.N,0.0), v2(model.N,0.0);
      float mass_inj_g(0.0);
      int min_row_idx(0);
      float mass_injected_g(0.0);
      network_->ReadTSGFile("Net3_Mass_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Mass);
      extract_single_injection(network_)->SetArray(model,x1);
      for (std::map<int,float>::const_iterator pos = x1.begin(), stop = x1.end(); pos != stop; ++pos) {
	 v1[pos->first] += pos->second;
      }
      extract_single_injection(network_)->SetArray(model,x2,mass_inj_g,min_row_idx);
      for (std::map<int,float>::const_iterator pos = x2.begin(), stop = x2.end(); pos != stop; ++pos) {
	 v2[pos->first] += pos->second;
      }
      mass_injected_g = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(mass_injected_g != 0.0);
      TS_ASSERT(::_ArraysAreEqual(&(v1[0]),&(v2[0]),model.N));
      float array_mass = ::_Sum(&(v1[0]),model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass,mass_injected_g, ::_EqualAbsTol);
      TS_ASSERT_DELTA(mass_inj_g,mass_injected_g, ::_EqualAbsTol);
   }

   void test_SetArray_FloatMap_TypeFlowpaced() {
      const MerlionModelContainer& model = network_->Model();
      std::map<int,float> x1, x2;
      std::vector<float> v1(model.N,0.0), v2(model.N,0.0);
      float mass_inj_g(0.0);
      int min_row_idx(0);
      float mass_injected_g(0.0);
      network_->ReadTSGFile("Net3_Flowpaced_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Flow);
      extract_single_injection(network_)->SetArray(model,x1);
      for (std::map<int,float>::const_iterator pos = x1.begin(), stop = x1.end(); pos != stop; ++pos) {
	 v1[pos->first] += pos->second;
      }
      extract_single_injection(network_)->SetArray(model,x2,mass_inj_g,min_row_idx);
      for (std::map<int,float>::const_iterator pos = x2.begin(), stop = x2.end(); pos != stop; ++pos) {
	 v2[pos->first] += pos->second;
      }
      mass_injected_g = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(mass_injected_g != 0.0);
      TS_ASSERT(::_ArraysAreEqual(&(v1[0]),&(v2[0]),model.N));
      float array_mass = ::_Sum(&(v1[0]),model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass,mass_injected_g, ::_EqualAbsTol);
      TS_ASSERT_DELTA(mass_inj_g,mass_injected_g, ::_EqualAbsTol);
      TS_ASSERT_DELTA(mass_inj_g,mass_injected_g, ::_EqualAbsTol);
   }

   void test_ClearArray_TypeMass() {
      const MerlionModelContainer& model = network_->Model();
      std::vector<float> x(model.N,0.0);
      network_->ReadTSGFile("Net3_Mass_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Mass);
      extract_single_injection(network_)->SetArray(model,x);
      TS_ASSERT(::_Sum(&(x[0]),model.N) != 0.0);
      extract_single_injection(network_)->ClearArray(model,x);
      TS_ASSERT(::_Sum(&(x[0]),model.N) == 0.0);
   }

   void test_ClearArray_TypeFlowpaced() {
      const MerlionModelContainer& model = network_->Model();
      std::vector<float> x(model.N,0.0);
      network_->ReadTSGFile("Net3_Flowpaced_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Flow);
      extract_single_injection(network_)->SetArray(model,x);
      TS_ASSERT(::_Sum(&(x[0]),model.N) != 0.0);
      extract_single_injection(network_)->ClearArray(model,x);
      TS_ASSERT(::_Sum(&(x[0]),model.N) == 0.0);
   }

   void test_UnsetArray_TypeMass() {
      const MerlionModelContainer& model = network_->Model();
      std::vector<float> x(model.N,0.0);
      network_->ReadTSGFile("Net3_Mass_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Mass);
      extract_single_injection(network_)->SetArray(model,x);
      extract_single_injection(network_)->SetArray(model,x);
      float m = extract_single_injection(network_)->MassInjected(model);
      m += extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(m != 0.0);
      TS_ASSERT_DELTA(::_Sum(&(x[0]),model.N,model.qual_step_minutes),m, ::_EqualAbsTol);
      extract_single_injection(network_)->UnsetArray(model,x);
      m = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(m != 0.0);
      TS_ASSERT_DELTA(::_Sum(&(x[0]),model.N,model.qual_step_minutes),m, ::_EqualAbsTol);
      extract_single_injection(network_)->UnsetArray(model,x);
      std::vector<float> zero(model.N,0.0);
      // At this point the array should be approximately zero
      // everywhere. Because of floating point error it may be
      // slightly off.
      TS_ASSERT(::_ArraysAreEqual(&(x[0]),&(zero[0]),model.N,::_EqualAbsTol,2));
   }

   void test_UnsetArray_TypeFlowpaced() {
      const MerlionModelContainer& model = network_->Model();
      std::vector<float> x(model.N,0.0);
      network_->ReadTSGFile("Net3_Flowpaced_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Flow);
      extract_single_injection(network_)->SetArray(model,x);
      extract_single_injection(network_)->SetArray(model,x);
      float m = extract_single_injection(network_)->MassInjected(model);
      m += extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(m != 0.0);
      float array_mass = ::_Sum(&(x[0]),model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass,m, ::_EqualAbsTol);
      extract_single_injection(network_)->UnsetArray(model,x);
      m = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(m != 0.0);
      array_mass = ::_Sum(&(x[0]),model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass,m, ::_EqualAbsTol);
      extract_single_injection(network_)->UnsetArray(model,x);
      std::vector<float> zero(model.N,0.0);
      // At this point the array should be approximately zero
      // everywhere. Because of floating point error it may be
      // slightly off.
      TS_ASSERT(::_ArraysAreEqual(&(x[0]),&(zero[0]),model.N,::_EqualAbsTol,2));
   }


   void test_SetMultiArray_FloatPoiner_TypeMass() {
      const MerlionModelContainer& model = network_->Model();
      float *x(new float[model.N*2]());
      float mass_inj_g(0.0);
      int min_row_idx(0);
      float mass_injected_g(0.0);
      network_->ReadTSGFile("Net3_Mass_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Mass);
      mass_injected_g = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(mass_injected_g != 0.0);

      extract_single_injection(network_)->SetMultiArray(model,2,0,x);
      float array_mass1 = ::_Sum(x,2,model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass1,mass_injected_g, ::_EqualAbsTol);

      TS_ASSERT(::_Sum(&(x[0])+1,2,model.N) == 0.0);
      extract_single_injection(network_)->SetMultiArray(model,2,1,x,mass_inj_g,min_row_idx);
      TS_ASSERT(::_ArraysAreEqual(x,2,x+1,2,model.N));
      float array_mass2 = ::_Sum(x+1,2,model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass2,mass_injected_g, ::_EqualAbsTol);
      TS_ASSERT_DELTA(mass_inj_g,mass_injected_g, ::_EqualAbsTol);
   }

   void test_SetMultiArray_FloatPoiner_TypeFLowpaced() {
      const MerlionModelContainer& model = network_->Model();
      float *x(new float[model.N*2]());
      float mass_inj_g(0.0);
      int min_row_idx(0);
      float mass_injected_g(0.0);
      network_->ReadTSGFile("Net3_Flowpaced_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Flow);
      mass_injected_g = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(mass_injected_g != 0.0);

      extract_single_injection(network_)->SetMultiArray(model,2,0,x);
      float array_mass1 = ::_Sum(x,2,model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass1,mass_injected_g, ::_EqualAbsTol);

      TS_ASSERT(::_Sum(&(x[0])+1,2,model.N) == 0.0);
      extract_single_injection(network_)->SetMultiArray(model,2,1,x,mass_inj_g,min_row_idx);
      TS_ASSERT(::_ArraysAreEqual(x,2,x+1,2,model.N));
      float array_mass2 = ::_Sum(x+1,2,model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass2,mass_injected_g, ::_EqualAbsTol);
      TS_ASSERT_DELTA(mass_inj_g,mass_injected_g, ::_EqualAbsTol);
   }

   void test_SetMultiArray_FloatVector_TypeMass() {
      const MerlionModelContainer& model = network_->Model();
      std::vector<float> x(model.N*2,0.0);
      float mass_inj_g(0.0);
      int min_row_idx(0);
      float mass_injected_g(0.0);
      network_->ReadTSGFile("Net3_Mass_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Mass);
      mass_injected_g = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(mass_injected_g != 0.0);

      extract_single_injection(network_)->SetMultiArray(model,2,0,x);
      float array_mass1 = ::_Sum(&(x[0]),2,model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass1,mass_injected_g, ::_EqualAbsTol);

      TS_ASSERT(::_Sum(&(x[0])+1,2,model.N) == 0.0);
      extract_single_injection(network_)->SetMultiArray(model,2,1,x,mass_inj_g,min_row_idx);
      TS_ASSERT(::_ArraysAreEqual(&(x[0]),2,&(x[0])+1,2,model.N));
      float array_mass2 = ::_Sum(&(x[0])+1,2,model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass2,mass_injected_g, ::_EqualAbsTol);
      TS_ASSERT_DELTA(mass_inj_g,mass_injected_g, ::_EqualAbsTol);
   }

   void test_SetMultiArray_FloatVector_TypeFlowpaced() {
      const MerlionModelContainer& model = network_->Model();
      std::vector<float> x(model.N*2,0.0);
      float mass_inj_g(0.0);
      int min_row_idx(0);
      float mass_injected_g(0.0);
      network_->ReadTSGFile("Net3_Flowpaced_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Flow);
      mass_injected_g = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(mass_injected_g != 0.0);

      extract_single_injection(network_)->SetMultiArray(model,2,0,x);
      float array_mass1 = ::_Sum(&(x[0]),2,model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass1,mass_injected_g, ::_EqualAbsTol);

      TS_ASSERT(::_Sum(&(x[0])+1,2,model.N) == 0.0);
      extract_single_injection(network_)->SetMultiArray(model,2,1,x,mass_inj_g,min_row_idx);
      TS_ASSERT(::_ArraysAreEqual(&(x[0]),2,&(x[0])+1,2,model.N));
      float array_mass2 = ::_Sum(&(x[0])+1,2,model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass2,mass_injected_g, ::_EqualAbsTol);
      TS_ASSERT_DELTA(mass_inj_g,mass_injected_g, ::_EqualAbsTol);
   }

   void test_SetMultiArray_FloatMap_TypeMass() {
      const MerlionModelContainer& model = network_->Model();
      std::map<int,float> x1, x2;
      std::vector<float> x(model.N*2,0.0);
      float mass_inj_g(0.0);
      int min_row_idx(0);
      float mass_injected_g(0.0);
      network_->ReadTSGFile("Net3_Mass_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Mass);
      mass_injected_g = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(mass_injected_g != 0.0);

      extract_single_injection(network_)->SetMultiArray(model,2,0,x);
      for (std::map<int,float>::const_iterator pos = x1.begin(), stop = x1.end(); pos != stop; ++pos) {
	 x[pos->first] += pos->second;
      }
      float array_mass1 = ::_Sum(&(x[0]),2,model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass1,mass_injected_g, ::_EqualAbsTol);

      TS_ASSERT(::_Sum(&(x[0])+1,2,model.N) == 0.0);
      extract_single_injection(network_)->SetMultiArray(model,2,1,x,mass_inj_g,min_row_idx);
      for (std::map<int,float>::const_iterator pos = x2.begin(), stop = x2.end(); pos != stop; ++pos) {
	 x[pos->first] += pos->second;
      }
      TS_ASSERT(::_ArraysAreEqual(&(x[0]),2,&(x[0])+1,2,model.N));
      float array_mass2 = ::_Sum(&(x[0])+1,2,model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass2,mass_injected_g, ::_EqualAbsTol);
      TS_ASSERT_DELTA(mass_inj_g,mass_injected_g, ::_EqualAbsTol);
   }

   void test_SetMultiArray_FloatMap_TypeFlowpaced() {
      const MerlionModelContainer& model = network_->Model();
      std::map<int,float> x1, x2;
      std::vector<float> x(model.N*2,0.0);
      float mass_inj_g(0.0);
      int min_row_idx(0);
      float mass_injected_g(0.0);
      network_->ReadTSGFile("Net3_Flowpaced_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Flow);
      mass_injected_g = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(mass_injected_g != 0.0);

      extract_single_injection(network_)->SetMultiArray(model,2,0,x);
      for (std::map<int,float>::const_iterator pos = x1.begin(), stop = x1.end(); pos != stop; ++pos) {
	 x[pos->first] += pos->second;
      }
      float array_mass1 = ::_Sum(&(x[0]),2,model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass1,mass_injected_g, ::_EqualAbsTol);

      TS_ASSERT(::_Sum(&(x[0])+1,2,model.N) == 0.0);
      extract_single_injection(network_)->SetMultiArray(model,2,1,x,mass_inj_g,min_row_idx);
      for (std::map<int,float>::const_iterator pos = x2.begin(), stop = x2.end(); pos != stop; ++pos) {
	 x[pos->first] += pos->second;
      }
      TS_ASSERT(::_ArraysAreEqual(&(x[0]),2,&(x[0])+1,2,model.N));
      float array_mass2 = ::_Sum(&(x[0])+1,2,model.N,model.qual_step_minutes);
      TS_ASSERT_DELTA(array_mass2,mass_injected_g, ::_EqualAbsTol);
      TS_ASSERT_DELTA(mass_inj_g,mass_injected_g, ::_EqualAbsTol);
   }

   void test_ClearMultiArray_TypeMass() {
      const MerlionModelContainer& model = network_->Model();
      std::vector<float> x(model.N*2,0.0);
      network_->ReadTSGFile("Net3_Mass_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Mass);
      extract_single_injection(network_)->SetMultiArray(model,2,0,x);
      extract_single_injection(network_)->SetMultiArray(model,2,1,x);
      TS_ASSERT(::_Sum(&(x[0]),2,model.N) != 0.0);
      extract_single_injection(network_)->ClearMultiArray(model,2,0,x);
      TS_ASSERT(::_Sum(&(x[0]),2,model.N) == 0.0);
      TS_ASSERT(::_Sum(&(x[0])+1,2,model.N) != 0.0);
      extract_single_injection(network_)->ClearMultiArray(model,2,1,x);
      TS_ASSERT(::_Sum(&(x[0])+1,2,model.N) == 0.0);
   }

   void test_ClearMultiArray_TypeFlowpaced() {
      const MerlionModelContainer& model = network_->Model();
      std::vector<float> x(model.N*2,0.0);
      network_->ReadTSGFile("Net3_Flowpaced_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Flow);
      extract_single_injection(network_)->SetMultiArray(model,2,0,x);
      extract_single_injection(network_)->SetMultiArray(model,2,1,x);
      TS_ASSERT(::_Sum(&(x[0]),2,model.N) != 0.0);
      extract_single_injection(network_)->ClearMultiArray(model,2,0,x);
      TS_ASSERT(::_Sum(&(x[0]),2,model.N) == 0.0);
      TS_ASSERT(::_Sum(&(x[0])+1,2,model.N) != 0.0);
      extract_single_injection(network_)->ClearMultiArray(model,2,1,x);
      TS_ASSERT(::_Sum(&(x[0])+1,2,model.N) == 0.0);
   }

   void test_UnsetMultiArray_TypeMass() {
      const MerlionModelContainer& model = network_->Model();
      std::vector<float> x(model.N*2,0.0);
      network_->ReadTSGFile("Net3_Mass_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Mass);
      extract_single_injection(network_)->SetMultiArray(model,2,0,x);
      extract_single_injection(network_)->SetMultiArray(model,2,0,x);
      extract_single_injection(network_)->SetMultiArray(model,2,1,x);
      extract_single_injection(network_)->SetMultiArray(model,2,1,x);
      float m = extract_single_injection(network_)->MassInjected(model);
      m += extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(m != 0.0);
      TS_ASSERT_DELTA(::_Sum(&(x[0]),2,model.N,model.qual_step_minutes),m, ::_EqualAbsTol);
      extract_single_injection(network_)->UnsetMultiArray(model,2,0,x);
      m = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(m != 0.0);
      TS_ASSERT_DELTA(::_Sum(&(x[0]),2,model.N,model.qual_step_minutes),m, ::_EqualAbsTol);
      extract_single_injection(network_)->UnsetMultiArray(model,2,0,x);
      std::vector<float> zero(model.N,0.0);
      // At this point the array should be approximately zero
      // everywhere. Because of floating point error it may be
      // slightly off.
      TS_ASSERT(::_ArraysAreEqual(&(x[0]),2,&(zero[0]),1,::_EqualAbsTol,2));

      m = extract_single_injection(network_)->MassInjected(model);
      m += extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(m != 0.0);
      TS_ASSERT_DELTA(::_Sum(&(x[0])+1,2,model.N,model.qual_step_minutes),m, ::_EqualAbsTol);
      extract_single_injection(network_)->UnsetMultiArray(model,2,1,x);
      m = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(m != 0.0);
      TS_ASSERT_DELTA(::_Sum(&(x[0])+1,2,model.N,model.qual_step_minutes),m, ::_EqualAbsTol);
      extract_single_injection(network_)->UnsetMultiArray(model,2,1,x);
      // At this point the array should be approximately zero
      // everywhere. Because of floating point error it may be
      // slightly off.
      TS_ASSERT(::_ArraysAreEqual(&(x[0])+1,2,&(zero[0]),1,::_EqualAbsTol,2));
   }

   void test_UnsetMultiArray_TypeFlowpaced() {
      const MerlionModelContainer& model = network_->Model();
      std::vector<float> x(model.N*2,0.0);
      network_->ReadTSGFile("Net3_Flowpaced_Single.tsg");
      TS_ASSERT(extract_single_injection(network_)->Type() == InjType_Flow);
      extract_single_injection(network_)->SetMultiArray(model,2,0,x);
      extract_single_injection(network_)->SetMultiArray(model,2,0,x);
      extract_single_injection(network_)->SetMultiArray(model,2,1,x);
      extract_single_injection(network_)->SetMultiArray(model,2,1,x);
      float m = extract_single_injection(network_)->MassInjected(model);
      m += extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(m != 0.0);
      TS_ASSERT_DELTA(::_Sum(&(x[0]),2,model.N,model.qual_step_minutes),m, ::_EqualAbsTol);
      extract_single_injection(network_)->UnsetMultiArray(model,2,0,x);
      m = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(m != 0.0);
      TS_ASSERT_DELTA(::_Sum(&(x[0]),2,model.N,model.qual_step_minutes),m, ::_EqualAbsTol);
      extract_single_injection(network_)->UnsetMultiArray(model,2,0,x);
      std::vector<float> zero(model.N,0.0);
      // At this point the array should be approximately zero
      // everywhere. Because of floating point error it may be
      // slightly off.
      TS_ASSERT(::_ArraysAreEqual(&(x[0]),2,&(zero[0]),1,::_EqualAbsTol,2));

      m = extract_single_injection(network_)->MassInjected(model);
      m += extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(m != 0.0);
      TS_ASSERT_DELTA(::_Sum(&(x[0])+1,2,model.N,model.qual_step_minutes),m, ::_EqualAbsTol);
      extract_single_injection(network_)->UnsetMultiArray(model,2,1,x);
      m = extract_single_injection(network_)->MassInjected(model);
      TS_ASSERT(m != 0.0);
      TS_ASSERT_DELTA(::_Sum(&(x[0])+1,2,model.N,model.qual_step_minutes),m, ::_EqualAbsTol);
      extract_single_injection(network_)->UnsetMultiArray(model,2,1,x);
      // At this point the array should be approximately zero
      // everywhere. Because of floating point error it may be
      // slightly off.
      TS_ASSERT(::_ArraysAreEqual(&(x[0])+1,2,&(zero[0]),1,::_EqualAbsTol,2));
   }

};
