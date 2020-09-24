#include <string.h>
#include <math.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>
extern "C" {
#include "tevautil.h"
}

#define ERDCHECKCPP(x) if ((ERRNUM=x) > 0) {ERD_Error(ERRNUM);}

// READ_QUALITY  - concentrations
// READ_DEMANDS  - demands
// READ_LINKFLOW - link flows
// READ_LINKVEL  - link velocities

int main(int argc, char *argv[])
{
	const char *erdName = "";
	if (argc > 1) {
		erdName = argv[1];
	} else {
		printf("Error: no erd file passed in!");
		return 1;
	}
	//
	const char *callType = "";
	if (argc > 2) {
		callType = argv[2];
	}

	if (strcmp(callType, "impact") == 0) {
		//
		int stopTime = -1;
		if (argc > 3) {
			stopTime = atoi(argv[3]);
		}
		//
		int sensorCount = 0;
		if (argc > 4) {
			sensorCount = atoi(argv[4]);
		}
		//
		std::vector< std::string > sensors(sensorCount);
		for (int iSensor = 0; iSensor < sensorCount; iSensor++) {
			int iarg = iSensor + 5;
			if (argc > iarg) {
				sensors[iSensor] = argv[iarg];
			}
		}
		//
		int assetCount = 0;
		if (argc > 5 + sensorCount) {
			assetCount = atoi(argv[5 + sensorCount]);
		}
		//
		std::vector< std::string > assets(assetCount);
		for (int iAsset = 0; iAsset < assetCount; iAsset++) {
			int iarg = iAsset + (6 + sensorCount);
			if (argc > iarg) {
				assets[iAsset] = argv[iarg];
			}
		}
		//
		double dZero = 0.0;
		PERD erd = NULL;
		if(ERD_isERD(erdName)) {
			ERDCHECKCPP(ERD_open(&erd, erdName, READ_QUALITY | READ_DEMANDS | READ_LINKFLOW | READ_LINKVEL | READ_IGNORE_ERROR));
		//	ERDCHECKCPP(ERD_open(&erd, erdName, READ_QUALITY | READ_DEMANDS | READ_IGNORE_ERROR));
		} else {
			printf("{\"Error\": \"This is not an erd file: %s\"}", erdName);
			return 1;
		}
		//
		int nLinks    = erd->network->numLinks;
		int nNodes    = erd->network->numNodes;
		int nTimes    = erd->network->numSteps;
		int nDuration = erd->network->simDuration;
		int nStep0    = erd->network->stepSize;
		int nStep     = int(nDuration/(nTimes - 1));
		int nSim      = erd->qualSimCount;
		//
		int nStop = 1e9;
		if (stopTime > -1) {
			for (int itime = 0; itime < nTimes; itime++) {
				int nSeconds = itime * nStep;
				if (nSeconds > stopTime) {
					nStop = itime - 1;
					break;
				} 
			}
		}
//		printf("\nstopTime = %d\n", stopTime);
//		printf("\nnStop = %d\n", nStop);
//		printf("\nnStep = %d\n", nStep);
//		printf("\nnDuration = %d\n", nDuration);
//		printf("\nnSteps = %d\n", nTimes);
//		printf("\nnStepSize = %d\n", nStep);
		//
		// use these vectors to map each sensor to a particular node id and sim id
		//
		std::vector< int > sensorMapToNodeIds(sensorCount);
		int imap1 = 0;
		for (int inode = 0; inode < nNodes; inode++) {
			const char *id = erd->nodes[inode].id;
			for (int iSensor = 0; iSensor < sensorCount; iSensor++) {
				if (strcmp(id, sensors[iSensor].c_str()) == 0) {
					imap1++;
					sensorMapToNodeIds[iSensor] = inode;
				}
			}
		}
		//
		// use these vectors to map each sensor to a particular node id and sim id
		//
		std::vector< int > assetMapToNodeIds(assetCount);
		for (int inode = 0; inode < nNodes; inode++) {
			const char *id = erd->nodes[inode].id;
			for (int iAsset = 0; iAsset < assetCount; iAsset++) {
				if (strcmp(id, assets[iAsset].c_str()) == 0) {
					assetMapToNodeIds[iAsset] = inode;
				}
			}
		}
// this will cause a perfectly good case to fail if all the sensors are not in the assets list
		if (imap1 < sensorCount || imap1 > sensorCount) {
			printf("{\"Error\": \"imap for NodeIds not correct. Sensor count = %d, but imap count = %d.\"}", sensorCount, imap1);
			return 1;
		}
		//
		// aggregate the time data (concentrations/mass-consumed) into a matrix of nodes by simulations
		//
		int nDetection = 0;
		int sumDetection = 0;
		std::vector< std::vector<int  > > results_nc(nSim, std::vector<int  >(nNodes));
		std::vector< std::vector<float> > results_mc(nSim, std::vector<float>(nNodes));
		std::vector< int > results_sensors(sensorCount);
		
		double histogram_min = 1e38;
		double histogram_max = 0.0;
		int histogram_total = 0;
		int histogram_shift =  14;
		int histogram_count = 30;
		std::vector< int > histogram_results(histogram_count);
		
		float zero_flow_count = 0;
		
		for (int isim = 0; isim < nSim; isim++) {
			std::vector< int > detectionResults(sensorCount);
			PSourceData source = (PTEVAIndexData) erd->qualSim[isim]->appData;
			if (loadSimulationResults(isim, erd, NULL, NULL, NULL, &source)) {

				for (int ilink = 0; ilink < nLinks; ilink++) {
					for (int itime = 0; itime < nTimes; itime++) {
						float fVal = erd->network->hydResults->flow[itime][ilink];
						if (fVal == 0.0) zero_flow_count += 1.0 / float(nTimes) / float(nSim);
					}
				}

				for (int inode = 0; inode < nNodes; inode++) {
					for (int itime = 0; itime < nTimes; itime++) {
						double fConcen = erd->network->qualResults->nodeC[0][inode][itime];
						bool bZero = (fConcen == 0);
						float fLog = bZero ? -histogram_shift : log10(fConcen);
						int index = int(fLog + histogram_shift);
						histogram_results[index]++;
						if (!bZero) {
							histogram_min = (fConcen < histogram_min) ? fConcen : histogram_min;
							histogram_max = (fConcen > histogram_max) ? fConcen : histogram_max;
						}
						histogram_total++;
					}
				}


				int iDetection = 1e9;
				for (int iSensor = 0; iSensor < sensorCount; iSensor++) {
					detectionResults[iSensor] = 2e9;
					for (int itime = 0; itime < nTimes; itime++) {
						int inode = sensorMapToNodeIds[iSensor];
						float fConcen = erd->network->qualResults->nodeC[0][inode][itime];
						if (fConcen > dZero) {
							detectionResults[iSensor] = itime;
							if (itime < iDetection) {
								iDetection = itime;
							}
							break;
						}
					}
				}
				// sum all the sensor detection results
				for (int iSensor = 0; iSensor < sensorCount; iSensor++) {
					if (detectionResults[iSensor] == iDetection) results_sensors[iSensor]++;
				}
				//
				int nTime = erd->network->numSteps;
				if (iDetection < nTime) { 
					nTime = iDetection + 1;
					nDetection++;
					sumDetection += iDetection;
				}
				int nCount = nNodes;
				if (assetCount > 0) nCount = assetCount;
				for (int iAsset = 0; iAsset < nCount; iAsset++) {
					int inode = iAsset;
					if (assetCount > 0) inode = assetMapToNodeIds[iAsset];
					bool bFound = false;
					float fSum = 0.0;
					for (int itime = 0; itime < nTime; itime++) {
						float fConcen = erd->network->qualResults->nodeC[0][inode][itime];
						float fDemand = erd->network->hydResults->demand[itime][inode];
						if (itime > nStop) continue;
						if (fDemand > 0 && fConcen > dZero) {
							fSum += fConcen * fDemand;
						}
						if (bFound == false && fConcen > dZero) {
							bFound = true;
						}
					}
					results_nc[isim][inode] = (bFound) ? 1 : 0;
					results_mc[isim][inode] = fSum;
				}
			}
		}
		//
		printf("{");
		printf("\"Impact\": ");
		printf("{");
		printf("\"PercentDetection\": ");
		//printf("%d", nSim);
		printf("%f", float(nDetection)/float(nSim)*100.0);
		printf(",");
		printf("\"AverageDetectionTime\": ");
		if (nDetection == 0) printf("%d", erd->network->numSteps);
		else                 printf("%f", float(sumDetection)/float(nDetection));
		printf(",");
		printf("\"Sensors\": ");
		printf("[");
		for (int i = 0; i < sensorCount; i++) {
			if (i > 0) printf(",");
			printf("\"%s\"", sensors[i].c_str());
		}
		printf("]");
		printf(",");
		printf("\"Assets\": ");
		printf("{");
		printf("\"values\": ");
		printf("[");
		int max_nc = 0;
		float max_mc = 0.0;
		int max_inc = -1;
		int max_imc = -1;
		int nCount = nNodes;
		if (assetCount > 0) nCount = assetCount;
		for (int iAsset = 0; iAsset < nCount; iAsset++) {
			int inode = iAsset;
			if (assetCount > 0) inode = assetMapToNodeIds[iAsset];
			int sum_nc = 0;
			float sum_mc = 0.0;
			for (int isim = 0; isim < nSim; isim++) {
				int iVal = results_nc[isim][inode];
				sum_nc += iVal;
				float fVal = results_mc[isim][inode];
				sum_mc += fVal;
			}
			if (max_nc < sum_nc) max_inc = inode;
			max_nc = (sum_nc > max_nc) ? sum_nc : max_nc;
			if (max_mc < sum_mc) max_imc = inode;
			max_mc = (sum_mc > max_mc) ? sum_mc : max_mc;
			if (iAsset > 0) printf(",");
			printf("{");
			printf("\"count\":");
			printf("%d", sum_nc);
			printf(",");
			printf("\"mass\":");
			printf("%f", sum_mc);
			printf("}");
		}
		printf("]");
		printf(",");
		printf("\"maxCount\": ");
		printf("%d", max_nc);
		printf(",");
		printf("\"maxCountIndex\": ");
		printf("%d", max_inc);
		printf(",");
		printf("\"maxMass\": ");
		printf("%f", max_mc);
		printf(",");
		printf("\"maxMassIndex\": ");
		printf("%d", max_imc);
		printf("}");
		printf(",");
		printf("\"Injections\": ");
		printf("{");
		printf("\"values\": ");
		printf("[");
		max_mc = 0.0;
		max_nc = 0;
		max_imc = -1;
		max_inc = -1;
		for (int isim = 0; isim < nSim; isim++) {
			int sum_nc = 0;
			float sum_mc = 0.0;
			for (int inode = 0; inode < nNodes; inode++) {
				int iVal = results_nc[isim][inode];
				sum_nc += iVal;
				float fVal = results_mc[isim][inode];
				sum_mc += fVal;
			}
			if (max_nc < sum_nc) max_inc = isim;
			max_nc = (sum_nc > max_nc) ? sum_nc : max_nc;
			if (max_mc < sum_mc) max_imc = isim;
			max_mc = (sum_mc > max_mc) ? sum_mc : max_mc;
			if (isim > 0) printf(",");
			printf("{");
			printf("\"count\":");
			printf("%d", sum_nc);
			printf(",");
			printf("\"mass\":");
			printf("%f", sum_mc);
			printf("}");
		}
		printf("]");
		printf(",");
		printf("\"maxCount\": ");
		printf("%d", max_nc);
		printf(",");
		printf("\"maxCountIndex\": ");
		printf("%d", max_inc);
		printf(",");
		printf("\"maxMass\": ");
		printf("%f", max_mc);
		printf(",");
		printf("\"maxMassIndex\": ");
		printf("%d", max_imc);
		printf("}");
		printf(",");
		printf("\"Sensors\": ");
		printf("{");
		printf("\"values\": ");
		printf("[");
		for (int iSensor = 0; iSensor < sensorCount; iSensor++) {
			if (iSensor > 0) printf(",");
			printf("%d", results_sensors[iSensor]);
		}
		printf("]");
		printf(",");///////////////////////////////////////////////////////////////////
		printf("\"maxInjections\": ");
		printf("%d", nSim);
		printf("}");
		printf(",");///////////////////////////////////////////////////////////////////
		printf("\"SensorIds\": ");
		printf("{");
		for (int iSensor = 0; iSensor < sensorCount; iSensor++) {
			if (iSensor > 0) printf(",");
			int inode = sensorMapToNodeIds[iSensor];
			printf("\"%s\": %d", erd->nodes[inode].id, iSensor);
		}
		printf("}");
		printf(",");///////////////////////////////////////////////////////////////////
		printf("\"NodeIds\": ");
		printf("{");
		nCount = nNodes;
		if (assetCount > 0) nCount = assetCount;
		for (int iAsset = 0; iAsset < nCount; iAsset++) {
			int inode = iAsset;
			if (assetCount > 0) inode = assetMapToNodeIds[iAsset];
			if (iAsset > 0) printf(",");
			printf("\"%s\": %d", erd->nodes[inode].id, iAsset);
		}
		printf("}");
		printf(",");///////////////////////////////////////////////////////////////////
		printf("\"SimIds\": ");
		printf("{");
		for (int isim = 0; isim < nSim; isim++) {
			PSourceData source = (PTEVAIndexData) erd->qualSim[isim]->appData;
			const char *sourceId = erd->nodes[source->source->sourceNodeIndex - 1].id;
			if (isim > 0) printf(",");
			printf("\"%s\": %d", sourceId, isim);
		}
		printf("}");
		printf(",");///////////////////////////////////////////////////////////////////
		printf("\"zero_flow\": %f", zero_flow_count);
		printf(",");///////////////////////////////////////////////////////////////////
		printf("\"Histogram\": ");
		printf("{");
		printf("\"total\":%d", histogram_total);
		printf(",");
		if (histogram_min < 1) printf("\"min\":%3.1e", histogram_min);
		else                   printf("\"min\":%f",    histogram_min);
		printf(",");
		if (histogram_max < 1) printf("\"max\":%3.1e", histogram_max);
		else                   printf("\"max\":%f",    histogram_max);
		printf(",");
		for (int i = 0; i < histogram_count; i++) {
			if (i > 0) printf(",");
			if (i == 0) printf("\"0_zero\":%d", histogram_results[i]);
			else printf("\"%d_greater_than_1e%d\":%d", i, i-histogram_shift, histogram_results[i]);
		}
		printf("}");
		printf("}");
		printf("}");
		//
		if(erd!=NULL) {
			ERD_close(&erd);
		}
		return 0;
	}
	//
	
	//
	
	//
	
	//

	//
	
	//
	if (strcmp(callType, "inversion") == 0) {
		FILE * pFile = fopen("/Users/smcgee/test.txt", "w");
//		fprintf(pFile, "123");
		PERD erd = NULL;
		if(ERD_isERD(erdName)) {
			ERDCHECKCPP(ERD_open(&erd, erdName, READ_QUALITY | READ_DEMANDS | READ_LINKFLOW | READ_LINKVEL | READ_IGNORE_ERROR));
		//	ERDCHECKCPP(ERD_open(&erd, erdName, READ_QUALITY | READ_DEMANDS | READ_IGNORE_ERROR));
		} else {
			printf("{\"Error\": \"This is not an erd file: %s\"}", erdName);
			return 1;
		}
		//
		int nLinks = erd->network->numLinks;
		int nNodes = erd->network->numNodes;
		int nTimes = erd->network->numSteps;
		int nSim = erd->qualSimCount;
		fprintf(pFile, "{");
		fprintf(pFile, "\n");
		fprintf(pFile, "\t");
		fprintf(pFile, "\"indexOf\":{");
		for (int inode = 0; inode < nNodes; inode++) {
			if (inode > 0) fprintf(pFile, ",");
			fprintf(pFile, "\"%s\":%d", erd->nodes[inode].id, inode);
		}
		fprintf(pFile, "}");
		fprintf(pFile, "\n");
		fprintf(pFile, ",");
		fprintf(pFile, "\n");
		fprintf(pFile, "\t");
		fprintf(pFile, "\"id\":[");
		for (int inode = 0; inode < nNodes; inode++) {
			if (inode > 0) fprintf(pFile, ",");
			fprintf(pFile, "\"%s\"", erd->nodes[inode].id);
		}
		fprintf(pFile, "]");
		fprintf(pFile, "\n");

		double dZero = 0.0;
		std::vector< std::vector<int> > sumVector(nNodes, std::vector<int>(nNodes));
		for (int isim = 0; isim < nSim; isim++) {
			PSourceData source = (PTEVAIndexData) erd->qualSim[isim]->appData;
			if (loadSimulationResults(isim, erd, NULL, NULL, NULL, &source)) {
				int sourceIdNumber = source->source->sourceNodeIndex;
				for (int inode = 0; inode < nNodes; inode++) {
					for (int itime = 0; itime < nTimes; itime++) {
						float fConcen = erd->network->qualResults->nodeC[0][inode][itime];
						if (fConcen > dZero) {
							sumVector[inode][isim] = 1;
							break;
						}
					}
				}
			}
		}

		fprintf(pFile, ",");
		fprintf(pFile, "\n");
		fprintf(pFile, "\t");
		fprintf(pFile, "\"data\":");
		fprintf(pFile, "[");

		// no compression
		if (false) {
			for (int inode1 = 0; inode1 < nNodes; inode1++) {
				int irepeat = -1;
				int icount = 0;
				if (inode1 > 0) fprintf(pFile, ",");
				fprintf(pFile, "[");
				for (int inode2 = 0; inode2 < nNodes; inode2++) {
					if (inode2 > 0) fprintf(pFile, ",");
					int icurrent = sumVector[inode1][inode2];
					fprintf(pFile, "%d", icurrent);
				}
				fprintf(pFile, "]");
				fprintf(pFile, "\n");
			}
		}
		// [count,value] compression
		if (false) {
			for (int inode1 = 0; inode1 < nNodes; inode1++) {
				//
				int irepeat;
				int icount;
				bool bFirst = true;
				if (inode1 > 0) fprintf(pFile, ",");
				fprintf(pFile, "[");
				//
				for (int inode2 = 0; inode2 < nNodes; inode2++) {
					int icurrent = sumVector[inode1][inode2];
					if (inode2 == 0) {
						irepeat = icurrent;
						icount = 0;
					}
					if (icurrent == irepeat) {
						icount++;
					} else {
						if (!bFirst) fprintf(pFile, ",");
						fprintf(pFile, "[");
						fprintf(pFile, "%d", icount);
						fprintf(pFile, ",");
						fprintf(pFile, "%d", irepeat);
						fprintf(pFile, "]");
						bFirst = false;
						irepeat = icurrent;
						icount = 1;
					}
				}
				//
				if (!bFirst) fprintf(pFile, ",");
				fprintf(pFile, "[");
				fprintf(pFile, "%d", icount);
				fprintf(pFile, ",");
				fprintf(pFile, "%d", irepeat);
				fprintf(pFile, "]");
				//
				fprintf(pFile, "]");
				fprintf(pFile, "\n");
			}
		}
		// only count, starting with zeros
		if (true) {
			for (int inode1 = 0; inode1 < nNodes; inode1++) {
				//
				int irepeat = 0;
				int icount = 0;
				bool bFirst = true;
				if (inode1 > 0) fprintf(pFile, ",");
				fprintf(pFile, "[");
				//
				for (int inode2 = 0; inode2 < nNodes; inode2++) {
					int icurrent = sumVector[inode1][inode2];
					if (icurrent == irepeat) {
						icount++;
					} else {
						if (!bFirst) fprintf(pFile, ",");
						bFirst = false;
						fprintf(pFile, "%d", icount);
						irepeat = icurrent;
						icount = 1;
					}
				}
				//
				if (!bFirst) fprintf(pFile, ",");
				fprintf(pFile, "%d", icount);
				//
				fprintf(pFile, "]");
				fprintf(pFile, "\n");
			}
		}

		fprintf(pFile, "]");
		fprintf(pFile, "\n");
		fprintf(pFile, "}");
		fprintf(pFile, "\n");
		fprintf(pFile, "\n");
		printf("{}");
		fclose(pFile);
		return 0;
	}
	if (strcmp(callType, "inversion1") == 0) {
		FILE * pFile = fopen("test.txt", "w");
		fclose(pFile);
		PERD erd = NULL;
		if (ERD_isERD(erdName)) {
			ERDCHECKCPP(ERD_open(&erd, erdName, READ_QUALITY | READ_DEMANDS | READ_LINKFLOW | READ_LINKVEL | READ_IGNORE_ERROR));
		} else {
			printf("{\"Error\": \"This is not an erd file: %s\"}", erdName);
			return 1;
		}
		PHydData hyd = erd->network->hydResults;
		int nTimes = erd->network->numSteps;
		int nLinks = erd->network->numLinks;
		int num_total      = 0;
		int num_below_zero = 0;
		int num_above_zero = 0;
		int num_zero       = 0;
		int num_change_dir = 0;
		int num_all_zero   = 0;
		for (int ilink = 0; ilink < nLinks; ilink++) {
			bool bAbove = false;
			bool bBelow = false;
			for (int itime = 0; itime < nTimes; itime++) {
				num_total++;
				float flow = hyd->flow[itime][ilink];
				if (flow > 0.0) {
					bAbove = true;
					num_above_zero++;
				} else if (flow < 0.0) {
					bBelow = true;
					num_below_zero++;
				} else {
					num_zero++;
				}
			}
			if (bAbove && bBelow) {
				num_change_dir++;
			} else if (!bAbove && !bBelow) {
				num_all_zero++;
			}
		}
		printf("{");
		printf("\"num_links\"     : %d", nLinks        );
		printf(",");
		printf("\"num_times\"     : %d", nTimes        );
		printf(",");
		printf("\"num_total\"     : %d", num_total     );
		printf(",");
		printf("\"num_above_zero\": %d", num_above_zero);
		printf(",");
		printf("\"num_below_zero\": %d", num_below_zero);
		printf(",");
		printf("\"num_zero\"      : %d", num_zero      );
		printf(",");
		printf("\"num_change_dir\": %d", num_change_dir);
		printf(",");
		printf("\"num_all_zero\"  : %d", num_all_zero  );
		printf("}");
		//int nSim = erd->qualSimCount;
		//for (int isim = 0; isim < nSim; isim++) {
		//	PSourceData source = (PTEVAIndexData) erd->qualSim[isim]->appData;
		//	const char *sourceId = erd->nodes[source->source->sourceNodeIndex - 1].id;
		//}
		return 0;
	}
	//
	
	//
	
	//
	
	//

	//
	
	//
	const char *attributeRequested = "";
	if (argc > 3) {
		attributeRequested = argv[3];
	}
	//
	const char *nodeId = "";
	if (argc > 4) {
		nodeId = argv[4];
	}
	const char *simId = "";
	if (argc > 5) {
		simId = argv[5];
	}
	int t0 = -1;
	if (argc > 6) {
		t0 = atoi(argv[6]);
	}
	int t1 = -1;
	if (argc > 7) {
		t1 = atoi(argv[7]);
	}
	float fZero = 0.0;
	if (argc > 8) {
		fZero = atof(argv[8]);
	}
	float fMax = 0.0;
	if (argc > 9) {
		fMax = atof(argv[9]);
	}
	int nBands = 9;
	if (argc > 10) {
		nBands = atof(argv[10]);
	}
	//
	PERD erd = NULL;
	if(ERD_isERD(erdName)) {
		ERDCHECKCPP(ERD_open(&erd, erdName, READ_QUALITY | READ_DEMANDS | READ_LINKFLOW | READ_LINKVEL | READ_IGNORE_ERROR));
	} else {
		printf("{\"Error\": \"This is not an erd file: %s\"}", erdName);
		return 1;
	}
	//
	//PHydData hyd = erd->network->hydResults;
	//PQualData qual = erd->network->qualResults;
	if (false) {
		printf("Number of nodes = %d\n" 		,erd->network->numNodes			);
		printf("Number of junctions = %d\n"	,erd->network->numJunctions		);
		printf("Number of tanks = %d\n" 		,erd->network->numTanks			);
		printf("Number of links = %d\n" 		,erd->network->numLinks			);
		printf("Controlled links = %d\n" 		,erd->network->numControlLinks	);
		printf("Number of steps = %d\n" 		,erd->network->numSteps			);
		printf("Step size = %lf\n"				,erd->network->stepSize			);
		printf("Sim Start = %ld\n"				,erd->network->simStart			);
		printf("Sim Duration = %ld\n"			,erd->network->simDuration		);
		printf("Number of species = %d\n" 		,erd->network->numSpecies		);
		printf("Report start = %ld\n"			,erd->network->reportStart		);
		printf("Report step = %ld\n"			,erd->network->reportStep		);
		printf("Quality code = %d\n" 			,erd->network->qualCode			);
		printf("Simulation count = %d\n"		,erd->qualSimCount				);
	}
	//
	int nSim = erd->qualSimCount;
	if (strcmp(callType, "static") == 0) {
		printf("{\"SourceIds\":");
		printf("[");
		for (int isim = 0; isim < nSim; isim++) {
			PSourceData source = (PTEVAIndexData) erd->qualSim[isim]->appData;
			const char *sourceId = erd->nodes[source->source->sourceNodeIndex - 1].id;
			if (isim > 0) printf(",");
			printf("\"%s\"", sourceId);
		}
		printf("]");
		if (nSim > 0) printf(",");
	}
	int iSim = -1;
	PSourceData source;
	if (strcmp(simId, "") != 0) {
		for (int isim = 0; isim < nSim; isim++) {
			source = (PTEVAIndexData) erd->qualSim[isim]->appData;
			const char *sourceId = erd->nodes[source->source->sourceNodeIndex - 1].id;
			if (strcmp(simId, sourceId) == 0) {
				iSim = isim;
				break;
			}
		}
	}
	if (iSim == -1 && nSim > 0) {
		iSim = 0;
		source = (PTEVAIndexData) erd->qualSim[iSim]->appData;
		simId = erd->nodes[source->source->sourceNodeIndex - 1].id;
	}
	int iNode = -1;
	if (strcmp(nodeId, "") != 0) {
		for (int inode = 0; inode < erd->network->numNodes; inode++) {
			if (strcmp(nodeId, erd->nodes[inode].id) == 0) {
				iNode = inode;
				break;
			}
		}
	}
	if (nSim > 0) {
		if (loadSimulationResults(iSim, erd, NULL, NULL, NULL, &source)) {
			if (iNode == -1)
				iNode = source->source->sourceNodeIndex - 1;
			if (false) {
				printf("Number of sources = %d\n", source->nsources);
				printf("Source node (%d) = %s\n", source->source->sourceNodeIndex, source->source->sourceNodeID);
				printf("Source start = %ld\n", source->source->sourceStart);
				printf("Source stop = %ld\n", source->source->sourceStop);
				printf("Source strength = %f\n" , source->source->sourceStrength);
			}
			if (strcmp(callType, "static") == 0) {
				float fMax = 0.0;
				int imax = -1;
				for (int itime = 0; itime < erd->network->numSteps; itime++) {
					for (int inode = 0; inode < erd->network->numNodes; inode++) {
						float fVal = 0.0;
						if (strcmp(attributeRequested, "Concentration") == 0)
							fVal = erd->network->qualResults->nodeC[0][inode][itime];
						if (strcmp(attributeRequested, "Demand") == 0 && inode < erd->network->numJunctions)
							fVal = erd->network->hydResults->demand[itime][inode];
						if (fVal > fMax) {
							fMax = fVal;
							imax = inode;
						}
					}
				}
				printf("\"sourceId\":\"%s\"", simId);
				printf(",");
				printf("\"id\":\"%s\"", erd->nodes[iNode].id);
				printf(",");
				printf("\"%sMax\":%f", attributeRequested, fMax);
				printf(",");
				printf("\"maxId\":\"%s\"", erd->nodes[imax].id);
				printf(",");
				printf("\"numSteps\":%d", erd->network->numSteps);
				printf(",");
				printf("\"timeStep\":%ld", erd->network->reportStep);
				printf(",");
				printf("\"timeStart\":%ld", erd->network->reportStart);
				printf(",");
				printf("\"%ss\":", attributeRequested);
				printf("[");
				int istart = 0;
				for (int itime = istart; itime < erd->network->numSteps; itime++) {
					if (strcmp(attributeRequested, "Concentration") == 0) {
						if (itime > istart)	printf(",");
						printf("%f ", erd->network->qualResults->nodeC[0][iNode][itime]);
					}
					if (strcmp(attributeRequested, "Demand") == 0 && iNode < erd->network->numJunctions) {
						if (itime > istart)	printf(",");
						printf("%f ", erd->network->hydResults->demand[itime][iNode]);
					}
				}
				printf("]");


				printf(",");
				printf("\"Arguments\":");
				printf("{");
				for (int i = 0; i < argc; i++) {
					if (i > 0) printf(",");
					printf("\"arg%d\":\"%s\"", i, argv[i]);
				}
				printf("}");


				printf("}");
			}
			// 1. this is the original design. it just writes out all the data points.
			if (strcmp(callType, "dynamic1") == 0) {
				printf("{");
				printf("\"Dynamic\":");
				printf("{");
				printf("\"numSteps\":%d", erd->network->numSteps);
				printf(",");
				printf("\"timeStep\":%ld", erd->network->reportStep);
				printf(",");
				printf("\"timeStart\":%ld", erd->network->reportStart);
				printf(",");
				printf("\"%ss\":",attributeRequested);
				printf("{");
				for (int inode = 0; inode < erd->network->numNodes; inode++) {
					int istart = 0;
					int iend = erd->network->numSteps;
					if (t0 > -1) istart = t0;
					if (t1 > -1 && t1 < iend) iend = t1 + 1;
					if (inode > 0) printf(",");
					printf("\"%s\":",erd->nodes[inode].id);
					printf("[");
					for (int itime = istart; itime < iend; itime++) {
						if (strcmp(attributeRequested, "Concentration") == 0) {
							if (itime > istart)	printf(",");
							printf("%f", erd->network->qualResults->nodeC[0][inode][itime]);
						}
						if (strcmp(attributeRequested, "Demand") == 0 && inode < erd->network->numJunctions) {
							if (itime > istart)	printf(",");
							printf("%f", erd->network->hydResults->demand[itime][inode]);
						}
					}
					printf("]");
				}
				printf("}");
				printf("}");
				printf("}");
			}
			// 2. this tweek doesnt print a node if all its values are zero.
			//    it also implements a threshold for determining what is a zero.
			if (strcmp(callType, "dynamic2") == 0) {
				printf("{");
				printf("\"Dynamic\":");
				printf("{");
				printf("\"numSteps\":%d", erd->network->numSteps);
				printf(",");
				printf("\"timeStep\":%ld", erd->network->reportStep);
				printf(",");
				printf("\"timeStart\":%ld", erd->network->reportStart);
				printf(",");
				printf("\"%ss\":",attributeRequested);
				printf("{");
				bool bFirst = true;
				for (int inode = 0; inode < erd->network->numNodes; inode++) {
					int istart = 0;
					int iend = erd->network->numSteps;
					if (t0 > -1) istart = t0;
					if (t1 > -1 && t1 < iend) iend = t1 + 1;
					bool bPrint = false;
					for (int itime = istart; itime < iend; itime++) {
						if (strcmp(attributeRequested, "Concentration") == 0) {
							float nodeC = erd->network->qualResults->nodeC[0][inode][itime];
							if (nodeC > fZero) {
								bPrint = true;
								break;
							}
						}
						if (strcmp(attributeRequested, "Demand") == 0 && inode < erd->network->numJunctions) {
							float demand = erd->network->hydResults->demand[itime][inode];
							if (demand > 0.0) {
								bPrint = true;
								break;
							}
						}
					}
					if (bPrint) {
						//if (inode > 0) printf(",");
						if (bFirst) {
							bFirst = false;
						} else {
							printf(",");
						}
						printf("\"%s\":",erd->nodes[inode].id);
						printf("[");
						for (int itime = istart; itime < iend; itime++) {
							if (strcmp(attributeRequested, "Concentration") == 0) {
								if (itime > istart)	printf(",");
								float nodeC = erd->network->qualResults->nodeC[0][inode][itime];
								if (nodeC > fZero) {
									printf("%f", nodeC);
								} else {
									printf("0", nodeC);
								}
							}
							if (strcmp(attributeRequested, "Demand") == 0 && inode < erd->network->numJunctions) {
								if (itime > istart)	printf(",");
								printf("%f", erd->network->hydResults->demand[itime][inode]);
							}
						}
						printf("]");
					}
				}
				printf("}");
				printf("}");
				printf("}");
			}
			// 3. his tweek converts the values to bands
			if (strcmp(callType, "dynamic3") == 0) {
				printf("{");
				printf("\"Dynamic\":");
				printf("{");
				printf("\"numSteps\":%d", erd->network->numSteps);
				printf(",");
				printf("\"timeStep\":%ld", erd->network->reportStep);
				printf(",");
				printf("\"timeStart\":%ld", erd->network->reportStart);
				printf(",");
				printf("\"%ss\":",attributeRequested);
				printf("{");
				bool bFirst = true;
				float fBand = (fMax - fZero) / nBands;
				for (int inode = 0; inode < erd->network->numNodes; inode++) {
					if (inode > 0) printf(",");
					printf("\"%s\":",erd->nodes[inode].id);
					printf("[");
					int istart = 0;
					int iend = erd->network->numSteps;
					if (t0 > -1) istart = t0;
					if (t1 > -1 && t1 < iend) iend = t1 + 1;
					for (int itime = istart; itime < iend; itime++) {
						if (itime > istart)	printf(",");
						if (strcmp(attributeRequested, "Concentration") == 0) {
							float nodeC = erd->network->qualResults->nodeC[0][inode][itime];
							for (int iBand = nBands; iBand > 0; iBand--) {
								if (nodeC > fZero + fBand * (iBand - 1)) {
									printf("%d", iBand);
									break;
								}
								if (iBand == 1) printf("0"); // it only came here because all others failed the test
							}
						}
						if (strcmp(attributeRequested, "Demand") == 0 && inode < erd->network->numJunctions) {
							float demand = erd->network->hydResults->demand[itime][inode];
							printf("%f", demand);
						}
					}
					printf("]");
				}
				printf("}");
				printf("}");
				printf("}");
			}
			// 4. this one combines 2 & 3 (skip zeros and use bands).
			if (strcmp(callType, "dynamic4") == 0) {
				printf("{");
				printf("\"Dynamic\":");
				printf("{");
				printf("\"numSteps\":%d", erd->network->numSteps);
				printf(",");
				printf("\"timeStep\":%ld", erd->network->reportStep);
				printf(",");
				printf("\"timeStart\":%ld", erd->network->reportStart);
				printf(",");
				printf("\"%ss\":",attributeRequested);
				printf("{");
				bool bFirst = true;
				float fBand = (fMax - fZero) / nBands;
				for (int inode = 0; inode < erd->network->numNodes; inode++) {
					int istart = 0;
					int iend = erd->network->numSteps;
					if (t0 > -1) istart = t0;
					if (t1 > -1 && t1 < iend) iend = t1 + 1;
					bool bPrint = false;
					for (int itime = istart; itime < iend; itime++) {
						if (strcmp(attributeRequested, "Concentration") == 0) {
							float nodeC = erd->network->qualResults->nodeC[0][inode][itime];
							if (nodeC > fZero) {
								bPrint = true;
								break;
							}
						}
						if (strcmp(attributeRequested, "Demand") == 0 && inode < erd->network->numJunctions) {
							float demand = erd->network->hydResults->demand[itime][inode];
							if (demand > 0.0) {
								bPrint = true;
								break;
							}
						}
					}
					if (bPrint) {
						if (bFirst) {
							bFirst = false;
						} else {
							printf(",");
						}
						printf("\"%s\":",erd->nodes[inode].id);
						printf("[");
						for (int itime = istart; itime < iend; itime++) {
							if (itime > istart)	printf(",");
							if (strcmp(attributeRequested, "Concentration") == 0) {
								float nodeC = erd->network->qualResults->nodeC[0][inode][itime];
								for (int iBand = nBands; iBand > 0; iBand--) {
									if (nodeC > fZero + fBand * (iBand - 1)) {
										printf("%d", iBand);
										break;
									}
									if (iBand == 1) printf("0"); // it only came here because all others failed the test
								}
							}
							if (strcmp(attributeRequested, "Demand") == 0 && inode < erd->network->numJunctions) {
								float demand = erd->network->hydResults->demand[itime][inode];
								printf("%f", demand);
							}
						}
						printf("]");
					}
				}
				printf("}");
				printf("}");
				printf("}");
			}
			// 5. this option compresses the data by returning arrays of consecutive bands [nCount,iBand]
			if (strcmp(callType, "dynamic5") == 0) {
				printf("{");
				printf("\"Dynamic\":");
				printf("{");
				printf("\"numSteps\":%d", erd->network->numSteps);
				printf(",");
				printf("\"timeStep\":%ld", erd->network->reportStep);
				printf(",");
				printf("\"timeStart\":%ld", erd->network->reportStart);
				printf(",");
				printf("\"%ss\":",attributeRequested);
				printf("{");
				bool bFirstId = true;
				float fBand = (fMax - fZero) / nBands;
				for (int inode = 0; inode < erd->network->numNodes; inode++) {
					int iold = 0;
					int inew = 0;
					int nCount = 0;
					bool bFirstC = true;
					int istart = 0;
					int iend = erd->network->numSteps;
					if (t0 > -1) istart = t0;
					if (t1 > -1 && t1 < iend) iend = t1 + 1;
//					if (bFirstId) {
//						bFirstId = false;
//					} else {
//						printf(",");
//					}
//					printf("\"%s\":",erd->nodes[inode].id);
//					printf("[");
					for (int itime = istart; itime < iend; itime++) {
						float fVal = 0;
						if (strcmp(attributeRequested, "Concentration") == 0) {
							fVal = erd->network->qualResults->nodeC[0][inode][itime];
						}
						if (strcmp(attributeRequested, "Demand") == 0 && inode < erd->network->numJunctions) {
							fVal = erd->network->hydResults->demand[itime][inode];
						}
						bool bFound = false;
						for (int iBand = nBands; iBand > 0; iBand--) {
							if (fVal > fZero + fBand * (iBand - 1)) {
								bFound = true;
								inew = iBand;
								break;
							}
						}
						if (!bFound) inew = 0;
						if (inew == iold) nCount++;
						if (inew != iold || itime == iend - 1) {
//**/					if (inew != iold) {
							bool bSkip = bFirstC && (inew == 0) && (itime == iend - 1); // skip all zeros
							bool bPrint = ((itime > istart) && !bSkip);
							//if (itime > istart) {
							if (bPrint) {
								if (bFirstC) {
									if (bFirstId) {
										bFirstId = false;
									} else {
										printf(",");
									}
/**/								printf("\"%s\":",erd->nodes[inode].id);
/**/								printf("[");
									bFirstC = false;
								} else {
									printf(",");
								}
								printf("[%d,%d]", nCount, iold);
								if ((itime == iend - 1) && (inew != iold)) printf(",[%d,%d]", 1, inew);
							}
							nCount = 1;
							iold = inew;
							inew = 0;
						}
					}
//					printf("]");

/**/				if (!bFirstC) printf("]");

//**/				if (!bFirstC) {
//**/					printf("]");
//**/				} else {
//**/					bFirstId = true;
//**/				}
				}
				printf("}");
				printf("}");
				printf("}");
			}
		} else {
			printf("{\"Error\": \"Failed to load simulation results!\"}", nSim);
		}		
	} else {
		printf("{\"Error\": \"The number of simulations is equal to %d\"}", nSim);
	}

	//
	if(erd!=NULL) {
		ERD_close(&erd);
	}
	return 0;
}
