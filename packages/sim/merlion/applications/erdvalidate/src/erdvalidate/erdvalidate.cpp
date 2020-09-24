/*
   * Copyright � 2008 UChicago Argonne, LLC
   * NOTICE: This computer software, TEVA-SPOT, was prepared for UChicago Argonne, LLC
   * as the operator of Argonne National Laboratory under Contract No. DE-AC02-06CH11357
   * with the Department of Energy (DOE). All rights in the computer software are reserved
   * by DOE on behalf of the United States Government and the Contractor as provided in
   * the Contract.
   * NEITHER THE GOVERNMENT NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
   * ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
   *
   * This software is distributed under the BSD License.
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string>
#include <iostream>
#include <vector>
extern "C" {
#include "tevautil.h"
}
const float conc_tol = 1e-8;

int compareInt(int valref, int val1, int val2, std::string prop_name);
int compareLong(long valref, long val1, long val2, std::string prop_name);
int compareFloat(float valref, float val1, float val2, std::string prop_name);
int sourcesSame(PSourceData source1, PSourceData source2, float strengthTol);

int compareString(char* valref, char* val1, char* val2, std::string prop_name);
int compare2DFloat(float **f1, float **f2, int nrows, int ncols, float tol, std::string prop_name);

float max(float a, float b) {
   return (a > b) ? a : b;
}
// Defining a cpp compatable version of ERDCHECK. The 'else' value
// is not needed since ERD_Error causes program to exit anyway.
#define ERDCHECKCPP(x) if ((ERRNUM=x) > 0) {ERD_Error(ERRNUM);}

int main(int argc, char **argv)
{
   char *fileref, *file1, *file2;  
   PHydData hydDataref = NULL;
   PHydData hydData1 = NULL;
   PHydData hydData2 = NULL;
   PQualData qualDataref = NULL;
   PQualData qualData1 = NULL;
   PQualData qualData2 = NULL;
   PNodeInfo nodesref = NULL;
   PNodeInfo nodes1 = NULL;
   PNodeInfo nodes2 = NULL;
   PLinkInfo linksref = NULL;
   PLinkInfo links1 = NULL;
   PLinkInfo links2 = NULL;
   PNetInfo netref = NULL;
   PNetInfo net1 = NULL;
   PNetInfo net2 = NULL;
   PSourceData sourceref = NULL;
   PSourceData source1 = NULL;
   PSourceData source2 = NULL;
   PERD erdref = NULL;
   PERD erd1 = NULL;
   PERD erd2 = NULL;
   PTSO tsoref = NULL;
   PTSO tso1 = NULL;
   PTSO tso2 = NULL;
   float hydraulic_tol = 1e-6;
   float quality_tol = 1e-6;

   if(argc != 4 && argc != 5) {
      printf("usage: erdval reference_erd file1_erd file2_erd\n");
      printf("       optional fourth argument: \"--full-report\"\n");
      exit(9);
   }
   bool full_report = false;
   if (argc == 5){
      std::string cmd = argv[4];
      if (cmd == "--full-report"){
         full_report = true;
      }
      if (cmd != "--full-report"){
         printf("usage: erdval reference_erd file1_erd file2_erd\n");
	 printf("       optional fourth argument: \"--full-report\"\n");
	 exit(9);
      }
   }

   // read the filenames from the command line
   fileref = argv[1];
   file1 = argv[2];
   file2 = argv[3];

   if (!ERD_isERD(fileref)) {
      printf("Error: File: %s is not an ERD file.\n", fileref);
      return 1;
   }
   else {
      ERDCHECKCPP(ERD_open(&erdref, fileref, READ_IGNORE_ERROR | READ_QUALITY | READ_DEMANDS | READ_LINKFLOW | READ_LINKVEL));
      netref = erdref->network;
      nodesref = erdref->nodes;
      linksref = erdref->links;
   }

   if (!ERD_isERD(file1)) {
      printf("Error: File: %s is not an ERD file.\n", file1);
      return 1;
   }
   else {
      ERDCHECKCPP(ERD_open(&erd1, file1, READ_IGNORE_ERROR | READ_QUALITY | READ_DEMANDS | READ_LINKFLOW | READ_LINKVEL));
      net1 = erd1->network;
      nodes1 = erd1->nodes;
      links1 = erd1->links;
   }

   if (!ERD_isERD(file2)) {
      printf("Error: File: %s is not an ERD file.\n", file2);
      return 1;
   }
   else {
      ERDCHECKCPP(ERD_open(&erd2, file2, READ_IGNORE_ERROR | READ_QUALITY | READ_DEMANDS | READ_LINKFLOW | READ_LINKVEL));
      net2 = erd2->network;
      nodes2 = erd2->nodes;
      links2 = erd2->links;
   }

   /* Check prologue size info */
   printf("ERD Validate\n\n");

   if (!compareInt(get_count(erdref,tsoref), get_count(erd1,tso1), get_count(erd2,tso2), "Difference in Prologue (Num Injections)")
      || !compareInt(netref->numJunctions, net1->numJunctions, net2->numJunctions, "Difference in Prologue (Num Junctions)")
      || !compareInt(netref->numLinks, net1->numLinks, net2->numLinks, "Difference in Prologue (Num Links)")
      || !compareInt(netref->numNodes, net1->numNodes, net2->numNodes, "Difference in Prologue (Num Nodes)")
      || !compareInt(net1->numSteps, net1->numSteps, net2->numSteps, "Difference in Prologue (Num Steps)")
      || !compareInt(netref->numTanks, net1->numTanks, net2->numTanks, "Difference in Prologue (Num Tanks)") 
      || !compareInt(netref->numSpecies, net1->numSpecies, net2->numSpecies, "Difference in Prologue (Num Species)")
      || !compareFloat(net1->stepSize, net1->stepSize, net2->stepSize, "Difference in Prologue (Stepsize)")
      || !compareInt(netref->qualCode, net1->qualCode, net2->qualCode, "Difference in Prologue (QualityCode)")
      || !compareLong(netref->reportStart, net1->reportStart, net2->reportStart, "Difference in Prologue (ReportStart)")
      || !compareLong(net1->reportStep, net1->reportStep, net2->reportStep, "Difference in Prologue (ReportStep)")
      || !compareLong(netref->simDuration, net1->simDuration, net2->simDuration, "Difference in Prologue (SimDuration)") ) {

      printf("Error in prologue. Comparison aborted.\n");
      return 9;
   }

   int stepsref_per_steps1 = net1->reportStep/netref->reportStep;
   if (net1->reportStep%netref->reportStep != 0 ) {
      printf("Reference's report stepsize is not a multiple of file1/file2 report stepsize. Comparison aborted.\n");
      return 1;
   }

   for(int i=0;i<net1->numSpecies;i++) {
      if (!compareString(netref->species[i]->id, net1->species[i]->id, net2->species[i]->id, "Difference in Species IDs")) { 
         return 1;
      }
   }

   hydDataref = netref->hydResults;
   hydData1=net1->hydResults;
   hydData2=net2->hydResults;

   /* Node Prologue info */
   for (int i=0; i<netref->numNodes; i++) {
      if (!compareString(nodesref[i].id, nodes1[i].id, nodes2[i].id, "Difference in Node IDs")) {
         return 1;
      }
   }

   if (!compare2DFloat(hydData1->demand, hydData2->demand, net1->numSteps, net1->numNodes, hydraulic_tol, "Error in demand (file1 vs file2)")
      || !compare2DFloat(hydData1->velocity, hydData2->velocity, net1->numSteps, net1->numNodes, hydraulic_tol, "Error in velocity (file1 vs file2)")
      || !compare2DFloat(hydData1->flow, hydData2->flow, net1->numSteps, net1->numNodes, hydraulic_tol, "Error in flow (file1 vs file2)")
      ) {
      return 1;
   }

   for (int i=0;i<net1->numLinks;i++) {
      if (!compareFloat(linksref[i].length, links1[i].length, links2[i].length, "Difference in link length")) {
         printf("Error occurred at link_id: %d\n", i);
         return 1;
      }
   }

   int numScenarios = get_count(erdref,tsoref);
   float* C_max = new float[numScenarios];
   memset(C_max, 0, sizeof(float)*numScenarios);
   float* total_abs_error1 = new float[numScenarios];
   memset(total_abs_error1, 0, sizeof(float)*numScenarios);
   float* total_rel_error1 = new float[numScenarios];
   memset(total_rel_error1, 0, sizeof(float)*numScenarios);
   float* total_rel_abs_error1 = new float[numScenarios];
   memset(total_rel_abs_error1, 0, sizeof(float)*numScenarios);
   float* max_rel_abs_err1 = new float[numScenarios];
   memset(max_rel_abs_err1, 0, sizeof(float)*numScenarios);
   int* node_rel_abs_err1 = new int[numScenarios];
   memset(node_rel_abs_err1, 0, sizeof(int)*numScenarios);
   float* total_abs_error2 = new float[numScenarios];
   memset(total_abs_error2, 0, sizeof(float)*numScenarios);
   float* total_rel_error2 = new float[numScenarios];
   memset(total_rel_error2, 0, sizeof(float)*numScenarios);
   float* total_rel_abs_error2 = new float[numScenarios];
   memset(total_rel_abs_error2, 0, sizeof(float)*numScenarios);
   float* max_rel_abs_err2 = new float[numScenarios];
   memset(max_rel_abs_err2, 0, sizeof(float)*numScenarios);
   int* node_rel_abs_err2 = new int[numScenarios];
   memset(node_rel_abs_err2, 0, sizeof(int)*numScenarios);
   float* abs_err1 = new float[net1->numNodes];
   float* abs_err2 = new float[net1->numNodes];
   float* rel_err1 = new float[net1->numNodes];
   float* rel_err2 = new float[net1->numNodes];
   float* rel_abs_err1 = new float[net1->numNodes];
   float* rel_abs_err2 = new float[net1->numNodes];
   
   float max_rel_abs_error1 = 0.0;
   float max_rel_abs_error2 = 0.0;
   int scen_id_max_rel_abs_error1 = 0;
   int node_id_max_rel_abs_error1 = 0;
   int scen_id_max_rel_abs_error2 = 0;
   int node_id_max_rel_abs_error2 = 0;
   int count1 = 0;
   int count2 = 0;
   
   for (int scen_id = 0; scen_id < numScenarios; ++scen_id) {
      // Try to load all three erd files
      int loadedref = loadSimulationResults(scen_id, erdref, tsoref, netref, nodesref, &sourceref);
      int loaded1 = loadSimulationResults(scen_id, erd1, tso1, net1, nodes1, &source1);
      int loaded2 =  loaded2=loadSimulationResults(scen_id, erd2, tso2, net2, nodes2, &source2);
      
      // Loop through the quality simulations
      if(loaded1 && loaded2 && loadedref) {
         qualDataref = erdref->network->qualResults;
         qualData1 = erd1->network->qualResults;
         qualData2 = erd2->network->qualResults;
         
         // do the comparison
         if (netref->numSpecies != 1 || net1->numSpecies != 1 || net2->numSpecies != 1) {
            printf("Currently erdval only supports single species erd files.\n");
            return 1;
         }
         
         int specidx = 0;
         float **cref = qualDataref->nodeC[specidx];
         float **c1 = qualData1->nodeC[specidx];
         float **c2 = qualData2->nodeC[specidx];
         
         if (full_report) {
            std::cout << "Scenario_id," << scen_id << std::endl;
            std::cout << "Injection_nodes" ;
            for (int i=0; i<sourceref->nsources; i++) {
               std::cout << "," << sourceref->source[i].sourceNodeID << std::endl;
            }
            std::cout << "Injection_type," << sourceref->source[0].sourceType << std::endl;
            std::cout << "Injection_start," << sourceref->source[0].sourceStart << std::endl;
            std::cout << "Injection_stop," << sourceref->source[0].sourceStop << std::endl;
            std::cout << "Injection_strength," << sourceref->source[0].sourceStrength << std::endl;
            std::cout << std::endl;
            std::cout << ",node_name,abs_error_1,abs_error_2,difference,rel_error_1,rel_error_2,difference,rel_abs_error_1,rel_abs_error_2,difference" << std::endl;
         }
         
         float max_conc = 0.0;
         for(int nn=0; nn<net1->numNodes; nn++) {       
            for(int tt=0; tt<net1->numSteps; tt++) {
              if (max_conc < cref[nn][tt*stepsref_per_steps1]) {
                 max_conc = cref[nn][tt*stepsref_per_steps1];
              }
            }
         }
         C_max[scen_id] = max_conc;
         float temp_max_rel_abs_error1 = 0.0;
         float temp_max_rel_abs_error2 = 0.0;
         int temp_node1 = 0;
         int temp_node2 = 0;
         
         for(int nn=0; nn<net1->numNodes; nn++) {       
            float abs_error_1 = 0.0;
            float abs_error_2 = 0.0;
            float rel_error_1 = 0.0;
            float rel_error_2 = 0.0;
            float rel_abs_error_1 = 0.0;
            float rel_abs_error_2 = 0.0;
            bool found1 = false;
            bool found2 = false;
            
            for(int tt=0; tt<net1->numSteps; tt++) {
               float cvref = cref[nn][tt*stepsref_per_steps1];
               float cv1 = c1[nn][tt];
               float cv2 = c2[nn][tt];
               if (cvref > conc_tol || cv1 > conc_tol) {
                  found1 = true;
               }
               if (cvref > conc_tol || cv2 > conc_tol) {
                  found2 = true;
               }
               
               abs_error_1 += fabs(cvref - cv1);
               abs_error_2 += fabs(cvref - cv2);
               rel_error_1 += fabs(cvref - cv1)/max(cvref,1.0);
               rel_error_2 += fabs(cvref - cv2)/max(cvref,1.0);
               rel_abs_error_1 += fabs(cvref - cv1)/max_conc;
               rel_abs_error_2 += fabs(cvref - cv2)/max_conc;
               if (temp_max_rel_abs_error1 < fabs(cvref - cv1)/max_conc) {
                  temp_max_rel_abs_error1 = fabs(cvref - cv1)/max_conc;
                  temp_node1 = nn;
               }
               if (temp_max_rel_abs_error2 < fabs(cvref - cv2)/max_conc) {
                  temp_max_rel_abs_error2 = fabs(cvref - cv2)/max_conc;
                  temp_node2 = nn;
               }
               if (max_rel_abs_error1 < fabs(cvref - cv1)/max_conc) {
                  max_rel_abs_error1 = fabs(cvref - cv1)/max_conc;
                  scen_id_max_rel_abs_error1 = scen_id;
                  node_id_max_rel_abs_error1 = nn;
               }
               if (max_rel_abs_error2 < fabs(cvref - cv2)/max_conc) {
                  max_rel_abs_error2 = fabs(cvref - cv2)/max_conc;
                  scen_id_max_rel_abs_error2 = scen_id;
                  node_id_max_rel_abs_error2 = nn;
               }
            }
            
            if (found1) {
               count1++;
            }
            if (found2) {
               count2++;
            }
            
            abs_error_1 = abs_error_1*net1->reportStep;
            abs_error_2 = abs_error_2*net1->reportStep;
            rel_error_1 = rel_error_1*net1->reportStep;
            rel_error_2 = rel_error_2*net1->reportStep;
            total_abs_error1[scen_id] += abs_error_1;
            total_rel_error1[scen_id] += rel_error_1;
            total_rel_abs_error1[scen_id] += rel_abs_error_1;
            total_abs_error2[scen_id] += abs_error_2;
            total_rel_error2[scen_id] += rel_error_2;
            total_rel_abs_error2[scen_id] += rel_abs_error_2;
         
            if (full_report){
               abs_err1[nn] = abs_error_1;
               abs_err2[nn] = abs_error_2;
               rel_err1[nn] = rel_error_1;
               rel_err2[nn] = rel_error_2;
               rel_abs_err1[nn] = rel_abs_error_1;
               rel_abs_err2[nn] = rel_abs_error_2;
               std::cout << "," << nodesref[nn].id << "," << abs_error_1 << "," << abs_error_2 << "," << abs_error_1-abs_error_2 << ",";
               std::cout << rel_error_1 << "," << rel_error_2 << "," << rel_error_1-rel_error_2 << ",";
               std::cout << rel_abs_error_1 << "," << rel_abs_error_2 << "," << rel_abs_error_1-rel_abs_error_2 << std::endl;
            }
         }
         
         max_rel_abs_err1[scen_id] = temp_max_rel_abs_error1;
         node_rel_abs_err1[scen_id] = temp_node1;
         max_rel_abs_err2[scen_id] = temp_max_rel_abs_error2;
         node_rel_abs_err2[scen_id] = temp_node2;
         
         if (full_report){
            std::cout << std::endl;
            
            for(int n=0; n<net1->numNodes; n++) {  
               std::cout << ",erd_file,node_name,abs_error,rel_error,rel_abs_error,time_step";
               for(int t=0; t<net1->numSteps; t++) {
                  std::cout << "," << t;
               }
               std::cout << std::endl;
               
               std::cout << ",Reference," << nodesref[n].id << ",-,-,-,Conc.";
               for(int t=0; t<net1->numSteps; t++) {
                  std::cout << "," << cref[n][t*stepsref_per_steps1];
               }
               std::cout << std::endl;
               
               std::cout << ",Standard," << nodes1[n].id << "," << abs_err1[n] << "," << rel_err1[n] << "," << rel_abs_err1[n] << ",Conc.";
               for(int t=0; t<net1->numSteps; t++) {
                  std::cout << "," << c1[n][t];
               }
               std::cout << std::endl;
               
               std::cout << ",Candidate," << nodes2[n].id << "," << abs_err2[n] << "," << rel_err2[n] << "," << rel_abs_err2[n] << ",Conc.";
               for(int t=0; t<net1->numSteps; t++) {
                  std::cout << "," << c2[n][t];
               }
               std::cout << std::endl;
               std::cout << std::endl;
            }
            
            std::cout << std::endl;
            std::cout << std::endl;
            std::cout << std::endl;
         }
      }
   }
   
   float avg_rel_abs_error1 = 0.0;
   float avg_rel_abs_error2 = 0.0;
   
   std::cout << "Scen_id,Inj_node,Total_abs_error1,Total_abs_error2,Difference,Total_rel_error1,Total_rel_error2,Difference,Total_rel_abs_error1,Total_rel_abs_error2,Difference,Max_conc_ref,Max_rel_abs_error1,Node_id1,Max_rel_abs_error2,Node_id2" << std::endl;
   for (int scen_id = 0; scen_id < numScenarios; ++scen_id) {
      avg_rel_abs_error1 += total_rel_abs_error1[scen_id];
      avg_rel_abs_error2 += total_rel_abs_error2[scen_id];
      int loadedref = loadSimulationResults(scen_id, erdref, tsoref, netref, nodesref, &sourceref);
      std::cout << scen_id << "," << sourceref->source[0].sourceNodeID << "," << total_abs_error1[scen_id] << "," << total_abs_error2[scen_id] << "," << total_abs_error1[scen_id]-total_abs_error2[scen_id] << "," << total_rel_error1[scen_id] << "," << total_rel_error2[scen_id] << "," << total_rel_error1[scen_id]-total_rel_error2[scen_id] << "," << total_rel_abs_error1[scen_id] << "," << total_rel_abs_error2[scen_id] << "," << total_rel_abs_error1[scen_id]-total_rel_abs_error2[scen_id] << "," << C_max[scen_id] << "," << max_rel_abs_err1[scen_id] << "," << nodesref[node_rel_abs_err1[scen_id]].id << "," << max_rel_abs_err2[scen_id] << "," << nodesref[node_rel_abs_err2[scen_id]].id << std::endl;
   }
   
   std::cout << std::endl;
   std::cout << std::endl;
   std::cout << "Max_rel_abs_error1," << max_rel_abs_error1 << std::endl;
   int load = loadSimulationResults(scen_id_max_rel_abs_error1, erdref, tsoref, netref, nodesref, &sourceref);
   std::cout << "Scen_id_with_max_rel_abs_error," << scen_id_max_rel_abs_error1 << std::endl;
   std::cout << "Inj_node," << sourceref->source[0].sourceNodeID << std::endl; 
   std::cout << "Node_with_max_rel_abs_error," << nodesref[node_id_max_rel_abs_error1].id << std::endl; 
   std::cout << "Avg_rel_abs_error1," << avg_rel_abs_error1/count1/net1->numSteps << std::endl;
   std::cout << std::endl;
   
   std::cout << "Max_rel_abs_error2," << max_rel_abs_error2 << std::endl;
   load = loadSimulationResults(scen_id_max_rel_abs_error2, erdref, tsoref, netref, nodesref, &sourceref);
   std::cout << "Scen_id_with_max_rel_abs_error," << scen_id_max_rel_abs_error2 << std::endl;
   std::cout << "Inj_node," << sourceref->source[0].sourceNodeID << std::endl; 
   std::cout << "Node_with_max_rel_abs_error," << nodesref[node_id_max_rel_abs_error2].id << std::endl; 
   std::cout << "Avg_rel_abs_error2," << avg_rel_abs_error2/count2/net1->numSteps << std::endl;
   std::cout << std::endl;
   
   std::cout << "numScenarios*net1->numNodes," << numScenarios*net1->numNodes << std::endl;
   std::cout << "count1," << count1 << std::endl;
   std::cout << "count2," << count2 << std::endl;
   
   delete [] total_abs_error1;
   delete [] total_rel_error1;
   delete [] total_abs_error2;
   delete [] total_rel_error2;
   delete [] abs_err1;
   delete [] abs_err2;
   delete [] rel_err1;
   delete [] rel_err2;
   
   ERD_close(&erd1);
   ERD_close(&erd2);
   ERD_close(&erdref);
}

int sourcesSame(PSourceData source1, PSourceData source2, float strengthTol)
{
   int nsources1 = source1->nsources;
   int nsources2 = source2->nsources;
   PSource sources1 = source1->source;
   PSource sources2 = source2->source;

   int ret = 1;

   if (nsources1 != nsources2) {
      ret = 0;
   }
   else {
      for (int i=0; i<nsources1; ++i) {
         if (strcmp(sources1[i].sourceNodeID,sources2[i].sourceNodeID) != 0
            || sources1[i].sourceStart != sources2[i].sourceStart
            || sources1[i].sourceStop != sources2[i].sourceStop
            || fabs(sources1[i].sourceStrength - sources2[i].sourceStrength) > strengthTol
            || sources1[i].sourceType != sources2[i].sourceType
            || sources1[i].speciesIndex != sources2[i].speciesIndex) {
            ret = 0;
            break;
         }
      }
   }
   return ret;
}

int compareInt(int valref, int val1, int val2, std::string prop_name)
{
   if (valref != val1 || valref != val2) {
      printf("%s | ref: %d, val1: %d, val2: %d\n", prop_name.c_str(), valref, val1, val2);
      return 0;
   }
   return 1;
}

int compareLong(long valref, long val1, long val2, std::string prop_name)
{
   if (valref != val1 || valref != val2) {
      printf("%s | ref: %ld, val1: %ld, val2: %ld\n", prop_name.c_str(), valref, val1, val2);
      return 0;
   }
   return 1;
}

int compareFloat(float valref, float val1, float val2, std::string prop_name)
{
   if (valref != val1 || valref != val2) {
      printf("%s | ref: %f, val1: %f, val2: %f\n", prop_name.c_str(), valref, val1, val2);
      return 0;
   }
   return 1;
}

int compareString(char* valref, char* val1, char* val2, std::string prop_name)
{
   if (strcmp(valref, val1) != 0 || strcmp(valref, val2) != 0) {
      printf("%s | ref: %s, val1: %s, val2: %s\n", prop_name.c_str(), valref, val1, val2);
      return 0;
   }
   return 1;
}

int compare2DFloat(float **f1, float **f2, int nrows, int ncols, float tol, std::string prop_name)
{
   float max_error = 0;
   int max_row = -1;
   int max_col = -1;
   int i = 0;
   int j = 0;
   for(i=0;i<nrows;i++) {
      for(j=0;j<ncols;j++) {
         if(fabs(f1[i][j]-f2[i][j]) > max_error) {
            max_error = fabs(f1[i][j]-f2[i][j]);
            max_row = i;
            max_col = j;
         }
      }
   }

   if(max_error > tol) {
      printf("%s | max_error = %f @ item_idx:%d and time_idx:%d)\n", prop_name.c_str(), max_error, max_row, max_col);
      return 0;
   }

   return 1;
}

