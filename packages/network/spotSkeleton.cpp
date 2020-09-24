#include <iostream>
#include <fstream>
#include <map>
#include <list>
#include <vector>
#include <string>
#include <math.h>
#include <stdlib.h>


using namespace std;

#define STRMAX 512
#define max(a,b) ((a) > (b))? a : b;

char emsg[STRMAX];
                                                  
#define ENCHECK(x) (((x) > 0) ? (ENgeterror(x, emsg, STRMAX), \
                      printf("%s\n",emsg)) : (0))

extern  "C" {
//#include "toolkit.h"
#include "epanet2.h"
}

bool isDegree1(int theNode, vector<vector<int> >&neighbors,
               vector<vector<int> >& neighborEdge, int* nodeType, int* linkType);
bool isDegree2(int theNode, vector<vector<int> >&neighbors,
               vector<vector<int> >& neighborEdge, int* nodeType, int* linkType);
bool isParalNode(int theNode, vector<vector<int> >&neighbors,
                 vector<vector<int> >& neighborEdge, int* nodeType, int* linkType, double* headLoss);
bool isParalExit(int theNode, vector<int> paralNode);
bool isSamePattern(int node1, int node2); 

void getNumFromString(string rowstring, char* num);
void getParal(int theNode, int &neib, int& loc1, int &loc2, vector<vector<int> >&neighbors,
              vector<vector<int> >& neighborEdge, int* nodeType, int* linkType, double* headLoss);

void setEdge(int nlinks, int* startp, int* endp);
void setLinkType(int nlinks, double threshvalue, int* linkType);
void setHeadLoss(int nlinks, double* headLoss);
void setNodeType(int nnodes, int* nodeType);
void setNeighbors(int nlinks, vector<vector<int> > & neighbors, vector<vector<int> > & neighborEdge);
void setMap(int nnodes, vector<vector<int> > & maps);
void setDemandPattern(int nnodes, vector<vector<float> > & demand,  vector<vector<float> > & pattern);
void setDegreeList(int nnodes, vector<vector<int> > &neighbors, 
                   vector<vector<int> >& neighborEdge, int* nodeType, int* linkType, double* headLoss,
                   vector<int> & degree1, vector<int> & degree2);

void updateMaps(int theNext, int theNode, vector<vector<int> >& maps);
void updateDemand(int theNode, int theNext);
void updateDemand(int theNode, int theNext1, int theNext2);
void updateDemandPattern(int theNode, int theNext, vector<vector<float> >& demand, vector<vector<float> >& pattern);
void updateDiameterSeries(int theLink, int theLinkr);
void updateDiameterParallel(int theLink, int theLinkr);
void updateNeib(int theNext1, int theNode, int theNext2, int theLink, 
                vector<vector<int> >& neighbors, vector<vector<int> >& neighborEdge);

bool removeDegree2(vector<int>&degree2, int theNode);
bool removePar(vector<int>&paralNode, int theNode);
void removeNeib(int theNext, int theNode, vector<vector<int> >& neighbors, vector<vector<int> >& neighborEdge);
void removeLink(int link, vector<int>& neighbors, vector<int>& neighborEdge);

void branchTrim(vector<int> &degree1, vector<int> &degree2, int* nodeType, int* linkType, 
             vector<vector<int> >& neighbors, vector<vector<int> >& neighborEdge,
             vector<vector<int> >& maps, vector<vector<float> >& demand, vector<vector<float> >& pattern);
void mergeSeries(vector<int> &degree2, int* startp, int* endp, int* nodeType, int* linkType,double* headLoss,
               vector<vector<int> >& neighbors, vector<vector<int> >& neighborEdge,
               vector<vector<int> >& maps, vector<int>& paralNode, vector<vector<float> >& demand, vector<vector<float> >& pattern);
void mergeParallel(vector<int> &degree1, vector<int> &degree2, vector<int> &paralNode,
             int* startp, int* endp, int* nodeType, int* linkType,double* headLoss, 
             vector<vector<int> >& neighbors, vector<vector<int> >& neighborEdge,
             vector<vector<int> >& maps);

void printMaps(int nnodes, int* nodeType, char* outname, vector<vector<int> >& maps);
void printOutFile(int nnodes, int nlinks, int* startp, int* endp,
                  int* nodeType, int* linkType, vector<vector<float> >& demand, 
									vector<vector<float> >& pattern, char* inname, char* outname); 







void printMaps(int nnodes, int* nodeType, char* outname, vector<vector<int> >& maps){
	char *id = new char[256];
	char *id1 = new char[256];
	ofstream outfile(outname);
	for (int i=1; i<=nnodes; i++) {
		ENCHECK(ENgetnodeid(i, id));
		if (nodeType[i] != 10) {
      outfile << id << " " << maps[i].size() << " ";
      for (int j=0; j<maps[i].size(); j++) {
        ENCHECK(ENgetnodeid(maps[i][j], id1));
        outfile << id1 << " ";
      }
      outfile << '\n';
    }
  }
}

void printOutFile(int nnodes, int nlinks, int* startp, int* endp,
                  int* nodeType, int* linkType, vector<vector<float> >& demand, 
									vector<vector<float> >& pattern, char* inname, char* outname) {
	ofstream outfile(outname); 
  string str;
  ifstream infile(inname);
  getline(infile, str, '\n');
  while(!infile.eof()) {
		
		if (str.compare(0,10,"[JUNCTIONS]",0,10) == 0) {
      outfile << str << '\n';
			// output: ID Elevation
      getline(infile, str,'\n');
      while (str[0] != '[') 
				getline(infile, str,'\n');  
      float value1, value2, value3;
			int type;
      char *id = new char[256];
      char *id2 = new char[256];
      for (int i=1; i<=nnodes; i++) {
        ENCHECK(ENgetnodetype(i, &type));
        if ((type == 0) && (nodeType[i] != 10)) {
          ENgetnodevalue(i, EN_ELEVATION, &value1);
          ENCHECK(ENgetnodeid(i, id));
					outfile << id << "    " << value1 << "    " << '\n';
        }
      }
      outfile << '\n';
    }
	else if (str.compare(0,6,"[PIPES]",0,6) == 0) {
		outfile << str << '\n';
		// output: ID Node1 Node2 Length Diameter Roughness MinorLoss Status
	  getline(infile, str,'\n');
      while (str[0] != '[') 
        getline(infile, str,'\n');  
      float value1, value2, value3, value4, value5;
      int type;
      char *id = new char[256];
      char *id1 = new char[256];
      char *id2 = new char[256];
      for (int i=1; i<=nlinks; i++) {
				ENCHECK(ENgetlinktype(i, &type));
        if ((type == 1) && (linkType[i] != 10)) {
          ENgetlinkvalue(i, EN_LENGTH, &value1);
          ENgetlinkvalue(i, EN_DIAMETER, &value2);
          ENgetlinkvalue(i, EN_ROUGHNESS, &value3);
          ENgetlinkvalue(i, EN_MINORLOSS, &value4);
          ENgetlinkvalue(i, EN_INITSTATUS, &value5);
          ENCHECK(ENgetlinkid(i, id));
          ENCHECK(ENgetnodeid(startp[i], id1));
          ENCHECK(ENgetnodeid(endp[i], id2));
          if (value5==1) 
						outfile << id << "    " << id1 << "     "  << id2 <<  "       " << value1 << "    " 
                    << value2 << "     "  << value3 << "           " << value4 
                    << "           Open\n";
					else 
						outfile << id << "    " << id1 << "     "  << id2 <<  "       " << value1 << "    " 
                    << value2 << "     "  << value3 << "           " << value4 
                    << "           Closed\n";
        }
      }
      outfile << '\n';
    }
	else if (str.compare(0,5,"[DEMAND]",0,5) == 0) {
      outfile << str << '\n';
			//output: ID Demand Pattern
      getline(infile, str,'\n');
      while (str[0] != '[') 
        getline(infile, str,'\n');  
			float value1, value2;
      int type;
      char *id = new char[256];
      char *id2 = new char[256];
      for (int i=1; i<=nnodes; i++) {
        ENCHECK(ENgetnodetype(i, &type));
        if ((type == 0) && (nodeType[i] != 10)) {
					for (int j=0; j<demand[i].size(); j++) {
						value1 = demand[i][j];
						value2 = pattern[i][j];
						ENCHECK(ENgetnodeid(i, id));
						if (value1 > 0) {
							ENCHECK(ENgetpatternid(int(value2), id2));
							outfile << id << "    " << value1 << "     "  << id2 << '\n';
						}
					}
					/*  USE THIS TO CALL DEMAND AND PATTERN FROM INP FILE, 1 PER NODE
          ENgetnodevalue(i, EN_BASEDEMAND, &value1);
          ENgetnodevalue(i, EN_PATTERN, &value2);
          ENCHECK(ENgetnodeid(i, id));
					if (value1 > 0) {
						ENCHECK(ENgetpatternid(int(value2), id2));
            outfile << id << "    " << value1 << "     "  << id2 << '\n';
          }
					*/
        }
      }
			outfile << '\n';
    }
    else if (str.compare(0,5,"[STATUS]",0,5) == 0) {
      outfile << str << '\n';
      getline(infile, str,'\n');
      char* num = new char[256];
      while (str[0] != '[') {
        if ((str.length() < 3) || (str[0] == ';')) 
          outfile << str << '\n';
				else {
					getNumFromString(str, num);
          int id;
          ENCHECK(ENgetlinkindex(num, &id));
          if (linkType[id] != 10)
						outfile << str << '\n' << endl; 
        }
        getline(infile, str,'\n');
      }      
    }
	else if (str.compare(0,5,"[EMITTERS]",0,5) == 0) {
      outfile << str << '\n';
      getline(infile, str,'\n');
      char* num = new char[256];
      while (str[0] != '[') {
        if ((str.length() < 3) || (str[0] == ';')) 
          outfile << str << '\n';
        else {
          getNumFromString(str, num);
          int id;
          ENCHECK(ENgetnodeindex(num, &id));
          if (nodeType[id] != 10)
            outfile << str << '\n' << endl; 
        }
        getline(infile, str,'\n');
      }      
    }
    else if (str.compare(0,5,"[QUALITY]",0,5) == 0) {
      outfile << str << '\n';
      getline(infile, str,'\n');
      char* num = new char[256];
      while (str[0] != '[') {
        if ((str.length() < 3) || (str[0] == ';')) 
          outfile << str << '\n';
        else {
          getNumFromString(str, num);
          int id;
          ENCHECK(ENgetnodeindex(num, &id));
          if (nodeType[id] != 10)
            outfile << str << '\n' << endl; 
        }
        getline(infile, str,'\n');
      }      
    }
    else if (str.compare(0,5,"[SOURCES]",0,5) == 0) {
      outfile << str << '\n';
      getline(infile, str,'\n');
      char* num = new char[256];
      while (str[0] != '[') {
        if ((str.length() < 3) || (str[0] == ';')) 
          outfile << str << '\n';
        else {
          getNumFromString(str, num);
          int id;
          ENCHECK(ENgetnodeindex(num, &id));
          if (nodeType[id] != 10)
            outfile << str << '\n' << endl; 
        }
        getline(infile, str,'\n');
      }      
    }
    else if (str.compare(0,10,"[COORDINATES]",0,10)==0){
      outfile << str << '\n';
      getline(infile, str,'\n');
      char* num = new char[256];
      while (str[0] != '[') {
        if ((str.length() < 3) || (str[0] == ';')) 
          outfile << str << '\n';
        else {
          getNumFromString(str, num);
          int id;
          ENCHECK(ENgetnodeindex(num, &id));
          if (nodeType[id] != 10)
            outfile << str << '\n' << endl; 
        }
        getline(infile, str,'\n');
      }
    }
		else if (str.compare(0,10,"[VERTICES]",0,10)==0){
			outfile << str << '\n';
			// vertices are eliminated in this version of TEVASPOT Skeleton
			getline(infile, str,'\n');
			while (str[0] != '[') 
				getline(infile, str,'\n');  
		}
    else {
      outfile << str << '\n';
      getline(infile, str, '\n');
    }
  }
  infile.close();
  outfile.close();
}

void setEdge(int nlinks, int* startp, int* endp){
	for (int i=1; i<= nlinks; i++)  
		ENCHECK(ENgetlinknodes(i, &startp[i], &endp[i]));
}

void setHeadLoss(int nlinks, double* headLoss){
  float value1, value2;
  int node1, node2;
  for (int i=1; i<= nlinks; i++){
    ENCHECK(ENgetlinknodes(i, &node1, &node2));
    ENgetnodevalue(node1, EN_HEAD, &value1);
    ENgetnodevalue(node2, EN_HEAD, &value2);
    headLoss[i] = fabs(value1-value2);
  }
}

void setLinkType(int nlinks, double threshvalue, int* linkType){
  float value, valued;
  int type;
  for (int i=1; i<= nlinks; i++){
		ENCHECK(ENgetlinktype(i, &type));
    if (type == 1) { // pipe
			ENCHECK(ENgetlinkvalue(i, EN_INITSTATUS, &value));
      if (value == 1){ // open
        ENCHECK(ENgetlinkvalue(i, EN_DIAMETER, &valued));
        if (valued <= threshvalue)
          linkType[i] = 1; // 1 = open pipe below threshold
        else
          linkType[i] = 15; // 15 = open pipe above threshold
      }
      else
        linkType[i] = 11; // 11 = closed pipe
    }
    else
			linkType[i] = type; // pump or valve    
  }
}

void setNodeType(int nnodes, int* nodeType){
  float value;
  int type;
  for (int i=1; i<= nnodes; i++){ 
    ENCHECK(ENgetnodetype(i, &type));
    if (type == 0)  // junction
      nodeType[i] = 0;
    else
      nodeType[i] = type; // reservoir or tank   
  }
}

void setNeighbors(int nlinks, vector<vector<int> > & neighbors,
                  vector<vector<int> > & neighborEdge){
  int startp, endp;
  for (int i=1; i<= nlinks; i++) {
    ENCHECK(ENgetlinknodes(i, &startp, &endp));
    neighbors[startp].push_back(endp);
    neighbors[endp].push_back(startp);
    neighborEdge[startp].push_back(i);
    neighborEdge[endp].push_back(i);
  }
}

void setMap(int nnodes, vector<vector<int> > & maps){
	for (int i=1; i<= nnodes; i++) 
		maps[i].push_back(i); 
}

void setDemandPattern(int nnodes, vector<vector<float> > & demand, vector<vector<float> > & pattern){ 
	// this assumes that the original inp file has one demand and demand pattern per node
	// Modify to read in [DEMAND] section if this is not the case.
	// Update this function with EPANET toolkit function when available
	float value1, value2;
	for (int i=1; i<= nnodes; i++) {
		ENCHECK(ENgetnodevalue(i, EN_BASEDEMAND, &value1));
		ENCHECK(ENgetnodevalue(i, EN_PATTERN, &value2));
		demand[i].push_back(value1); 
		pattern[i].push_back(value2);
	}
}

void getNumFromString(string rowstring, char* num){
  int index1 = 0;
  while (rowstring[index1] == ' ')
    index1++;
  int index2 = 0;
  while ((rowstring[index1]!= ' ') && (rowstring[index1]!= '\t') && (rowstring[index1]!= '\n')) {
    num[index2] = rowstring[index1];
    index1++;
    index2++;
  }
  num[index2] = '\0';
}

void getParal(int theNode, int &neib, int& loc1, int &loc2, vector<vector<int> >&neighbors,
              vector<vector<int> >& neighborEdge, int* nodeType, int* linkType, double* headLoss){
  int num = neighbors[theNode].size();
  int neib1, neib2;
  for (int i=0; i<(num-1); i++)
    for (int j=(i+1); j<num; j++) {
      neib1 = neighbors[theNode][i];
      neib2 = neighbors[theNode][j];
      int link1 = neighborEdge[theNode][i];
      int link2 = neighborEdge[theNode][j];
      bool isDiameter;
      if ((linkType[link1] ==1)&&(linkType[link2]==1)) isDiameter = true;
      //else if ((linkType[link1] ==1)&&(linkType[link2]==15)) isDiameter = true; // Comment out so both pipes are under threshold
      //else if ((linkType[link1] ==15)&&(linkType[link2]==1)) isDiameter = true; // Comment out so both pipes are under threshold
      else isDiameter = false; 
      if ((neib1 == neib2) && (nodeType[neib1] <10) && (nodeType[neib2] <10) 
				&& (isDiameter) && (headLoss[link1]==headLoss[link2])) {
				neib = neib1;
        loc1 = i;
        loc2 = j;
        return ;
      }
    }
}

bool isParalExit(int theNode, vector<int> paralNode){
  bool isThere = false;
  for (int i=0; i<paralNode.size(); i++) {
    if (theNode == paralNode[i])
      isThere = true;
  }
  return isThere;
}

bool isSamePattern(int node1, int node2) {
  float value1, value2;
  ENCHECK(ENgetnodevalue(node1, EN_PATTERN, &value1));
  ENCHECK(ENgetnodevalue(node2, EN_PATTERN, &value2));
  if (value1==value2) return true;
  else return false;
}

bool isParalNode(int theNode, vector<vector<int> >&neighbors,
                 vector<vector<int> >& neighborEdge, int* nodeType, int* linkType, double* headLoss){
  bool isPar = false;
  int num = neighbors[theNode].size();
  if (num >= 2){
    int neib1, neib2;
    for (int i=0; i<(num-1); i++)
      for (int j=(i+1); j<num; j++) {
        neib1 = neighbors[theNode][i];
        neib2 = neighbors[theNode][j];
        int link1 = neighborEdge[theNode][i];
        int link2 = neighborEdge[theNode][j];
        bool isDiameter;
        if ((linkType[link1] ==1)&&(linkType[link2]==1)) isDiameter = true;
        //else if ((linkType[link1] ==1)&&(linkType[link2]==15)) isDiameter = true; // Comment out so both pipes must be below threshold
        //else if ((linkType[link1] ==15)&&(linkType[link2]==1)) isDiameter = true; // Comment out so both pipes must be below threshold
        else isDiameter = false; 
        if ((neib1 == neib2) && (nodeType[neib1] <10) && (nodeType[neib2] <10)
           && (isDiameter) && (headLoss[link1]==headLoss[link2]))
          isPar = true;
       }
  }
  return isPar;
}

bool isDegree1(int theNode, vector<vector<int> >&neighbors,
               vector<vector<int> >& neighborEdge, int* nodeType, int* linkType){
  int theNext = neighbors[theNode][0];
  // Remove isSamePattern criteria
  //if ((neighbors[theNode].size()==1)&&(nodeType[theNode]==0)&&isSamePattern(theNode, theNext)
  if ((neighbors[theNode].size()==1)&&(nodeType[theNode]==0)
     && (linkType[neighborEdge[theNode][0]]==1)&&(nodeType[theNext]==0)) 
    return true;
  else
    return false;
}

bool isDegree2(int theNode, vector<vector<int> >&neighbors,
               vector<vector<int> >& neighborEdge, int* nodeType, int* linkType){
  if ((neighbors[theNode].size() == 2)&&(nodeType[theNode]==0)){
    int neib1 = neighbors[theNode][0];
    int neib2 = neighbors[theNode][1];
    if ((linkType[neighborEdge[theNode][0]]==1)&&(nodeType[neib1]==0)
       && (linkType[neighborEdge[theNode][1]]==1)&&(nodeType[neib2]==0)
	   // Remove isSamePattern criteria
	   //&& (neib1 != neib2) && isSamePattern(theNode, neib1) && (isSamePattern(theNode, neib2)))
       && (neib1 != neib2))
      return true;
    else
      return false;
  }
  else
    return false;
}

void setDegreeList(int nnodes, vector<vector<int> > &neighbors, 
                   vector<vector<int> >& neighborEdge, int* nodeType, int* linkType, double* headLoss,
                   vector<int> & degree1, vector<int> & degree2, vector<int>& paralNode){
  for (int i=1; i<= nnodes; i++) {
    if (isDegree1(i, neighbors, neighborEdge, nodeType, linkType))
      degree1.push_back(i);
    if (isDegree2(i, neighbors, neighborEdge, nodeType, linkType))
      degree2.push_back(i);
    if (isParalNode(i, neighbors, neighborEdge, nodeType, linkType, headLoss))
      paralNode.push_back(i);
  }
}

void setSimpleControl(int ncontrols, int* nodeType, int* linkType){
  char *id1 = new char[256];
  char *id2 = new char[256];
  for (int i=1; i<= ncontrols; i++) {
    int ctype, lindex, nindex;
    float setting, level;
    ENCHECK(ENgetcontrol(i, &ctype, &lindex, &setting, &nindex, &level));
    linkType[lindex] = 11;
    ENCHECK(ENgetlinkid(lindex, id1));
    if (ctype<=1){
      nodeType[nindex] = 11;   
      ENCHECK(ENgetnodeid(nindex, id2));
    }
  }        
}

void setRuleControl(char* inname, int* nodeType, int* linkType){
  // Read in [RULES] section of the inp file
	// Replace this function with EPANET toolkit function when available
	string str;
  ifstream infile(inname);
  getline(infile, str, '\n');
  while(!infile.eof()) {

	  if (str.compare(0,10,"[RULES]",0,10) == 0) {
		  getline(infile, str,'\n');
		  char* num = new char[256];
		  int id;
			while (str[0] != '[') {
				if (int(str.find("JUNCTION")) >= 0) {
				  int start = int(str.find("JUNCTION")+9);
				  str = str.substr(start,str.size());
				  int end;
				  if (int(str.find(" ")) == -1) 
					  end = int(str.find("\t"));
				  else
					  end = min(int(str.find(" ")),int(str.find("\t")));
				  string nodeName = str.substr(0, end+1);
				  getNumFromString(nodeName, num);
				  ENCHECK(ENgetnodeindex(num, &id));
				  nodeType[id] = 11;   
			  }
			  else if (int(str.find("PIPE")) >= 0) {
				  int start = int(str.find("PIPE")+5);
				  str = str.substr(start,str.size());
				  int end;
				  if (int(str.find(" ")) == -1) 
					  end = int(str.find("\t"));
				  else
					  end = min(int(str.find(" ")),int(str.find("\t")));
				  string pipeName = str.substr(0, end+1);
				  getNumFromString(pipeName, num);
				  ENCHECK(ENgetlinkindex(num, &id));
				  linkType[id] = 11;
			  }
				getline(infile, str,'\n'); 
			}
			break; // after [RULES] section, stop
		}
	  else {
      getline(infile, str, '\n');
    }
  }
  infile.close();
}

void updateDemand(int theNode, int theNext){
  float value1, value2, newvalue;
  ENCHECK(ENgetnodevalue(theNode, EN_BASEDEMAND, &value1));
  ENCHECK(ENgetnodevalue(theNext, EN_BASEDEMAND, &value2));
  newvalue = value1+value2;
  ENCHECK(ENsetnodevalue(theNext, EN_BASEDEMAND, newvalue));
}

void updateDemand(int theNode, int theNext1, int theNext2){
  float value, value1, value2, newvalue1, newvalue2;
  ENCHECK(ENgetnodevalue(theNode, EN_BASEDEMAND, &value));
  ENCHECK(ENgetnodevalue(theNext1, EN_BASEDEMAND, &value1));
  ENCHECK(ENgetnodevalue(theNext2, EN_BASEDEMAND, &value2));
  newvalue1 = value/2.0+value1;
  newvalue2 = value/2.0+value2;
  ENCHECK(ENsetnodevalue(theNext1, EN_BASEDEMAND, newvalue1));
  ENCHECK(ENsetnodevalue(theNext2, EN_BASEDEMAND, newvalue2));
}

void updateDemandPattern(int theNode, int theNext, vector<vector<float> >& demand, vector<vector<float> >& pattern){
	for (int i=0; i<demand[theNode].size(); i++) {
		int match = 0;
		for (int j=0; j<demand[theNext].size(); j++) {
			if (pattern[theNode][i] == pattern[theNext][j]){
				demand[theNext][j] = demand[theNext][j] + demand[theNode][i];
				match = 1;
				break;
			}
		}
		if (match == 0) {
			float temp;
			temp = demand[theNode][i];
			demand[theNext].push_back(temp); 
			temp = pattern[theNode][i];
			pattern[theNext].push_back(temp);
		}
	}
}

void updateDiameterSeries(int theLink, int theLinkr){
	// Hydaulic equivalency calcualtion for series
  float dd1, dd2, ll1, ll2, rr1, rr2, newdd, newll, newrr;
  ENCHECK(ENgetlinkvalue(theLink, EN_DIAMETER, &dd1));
  ENCHECK(ENgetlinkvalue(theLinkr, EN_DIAMETER, &dd2));
  ENCHECK(ENgetlinkvalue(theLink, EN_LENGTH, &ll1));
  ENCHECK(ENgetlinkvalue(theLinkr, EN_LENGTH, &ll2));
  ENCHECK(ENgetlinkvalue(theLink, EN_ROUGHNESS, &rr1));
  ENCHECK(ENgetlinkvalue(theLinkr, EN_ROUGHNESS, &rr2));
  newll = ll1 + ll2;
  newrr = rr1;
  double tem = ll1 * pow(rr1, static_cast<float>(-1.852)) * pow(dd1, static_cast<float>(-4.871));
  tem += ll2 * pow(rr2, static_cast<float>(-1.852)) * pow(dd2, static_cast<float>(-4.871));
  tem = newll * pow(newrr, static_cast<float>(-1.852)) / tem;
  newdd = pow(tem , (1.0/4.871)); 
  ENCHECK(ENsetlinkvalue(theLink, EN_LENGTH, newll));
  ENCHECK(ENsetlinkvalue(theLink, EN_DIAMETER, newdd));
}

void updateDiameterParallel(int theLink, int theLinkr){
	// Hydaulic equivalency calcualtion for parallel
  float dd1, dd2, ll1, ll2, rr1, rr2, hh, newdd, newll, newrr;
  ENCHECK(ENgetlinkvalue(theLink, EN_DIAMETER, &dd1));
  ENCHECK(ENgetlinkvalue(theLinkr, EN_DIAMETER, &dd2));
  ENCHECK(ENgetlinkvalue(theLink, EN_LENGTH, &ll1));
  ENCHECK(ENgetlinkvalue(theLinkr, EN_LENGTH, &ll2));
  ENCHECK(ENgetlinkvalue(theLink, EN_ROUGHNESS, &rr1));
  ENCHECK(ENgetlinkvalue(theLinkr, EN_ROUGHNESS, &rr2));
  ENCHECK(ENgetlinkvalue(theLink, EN_HEADLOSS, &hh));
  newll = ll1;
  newrr = rr1;
  float tem = pow(ll1, static_cast<float>(-0.5)*hh) * rr1* pow(dd1, static_cast<float>(2.63));
  tem += rr2 * pow(ll2, static_cast<float>(-0.5)*hh) * pow(dd2, static_cast<float>(2.63));
  tem = pow(newll, static_cast<float>(0.5)*hh) * tem / newrr;
  newdd = pow(tem , static_cast<float>(1.0/2.63));
  ENCHECK(ENsetlinkvalue(theLink, EN_LENGTH, newll));
  ENCHECK(ENsetlinkvalue(theLink, EN_DIAMETER, newdd));
}

bool removePar(vector<int>&paralNode, int theNode){
  bool isThere = false;
  for (int i=0; i<paralNode.size(); i++){
    if (paralNode[i] == theNode) {
      paralNode[i] = paralNode.back();
      paralNode.pop_back();
      isThere = true;
    }  
  }
  return isThere;
}

void removeNeib(int theNext, int theNode, vector<vector<int> >& neighbors, 
				vector<vector<int> >& neighborEdge){
  for (int i=0; i<neighbors[theNext].size(); i++)
    if (neighbors[theNext][i] == theNode){
      neighbors[theNext][i] = neighbors[theNext].back();
      neighbors[theNext].pop_back();
      neighborEdge[theNext][i] = neighborEdge[theNext].back();
      neighborEdge[theNext].pop_back();
    }
}

bool removeDegree2(vector<int>&degree2, int theNode){
  bool isThere = false;
  for (int i=0; i<degree2.size(); i++){
    if (degree2[i] == theNode) {
      degree2[i] = degree2.back();
      degree2.pop_back();
      isThere = true;
    }  
  }
  return isThere;
}

void removeLink(int link, vector<int>& neighbors, vector<int>& neighborEdge){
  for (int i=0; i<neighborEdge.size(); i++)
    if (neighborEdge[i] == link) {
      neighborEdge[i] = neighborEdge.back();
      neighborEdge.pop_back();
      neighbors[i] = neighbors.back();
      neighbors.pop_back();
    }
}

void updateNeib(int theNext1, int theNode, int theNext2, int theLink, 
                vector<vector<int> >& neighbors, vector<vector<int> >& neighborEdge){
  for (int i=0; i<neighbors[theNext1].size(); i++)
    if (neighbors[theNext1][i] == theNode){
      neighbors[theNext1][i] = theNext2;
      neighborEdge[theNext1][i] = theLink;
    }
  for (int i=0; i<neighbors[theNext2].size(); i++)
    if (neighbors[theNext2][i] == theNode){
      neighbors[theNext2][i] = theNext1;
      neighborEdge[theNext2][i] = theLink;
    }
}

bool isExit(int theNode, vector<int>& nodeList) {
  bool isthere = false;
  for (int i=0; i<nodeList.size(); i++) {
    if (nodeList[i] == theNode)
      isthere = true;
  }
  return isthere;
}

void updateMaps(int theNext, int theNode, vector<vector<int> >& maps){
  for (int i=0; i<maps[theNode].size(); i++) {
    int tem = maps[theNode][i];
    if (!isExit(tem, maps[theNext]))
      maps[theNext].push_back(tem);
  }
}

void branchTrim(vector<int> &degree1, vector<int> &degree2, int* nodeType, int* linkType, 
             vector<vector<int> >& neighbors, vector<vector<int> >& neighborEdge,
             vector<vector<int> >& maps, vector<vector<float> >& demand, vector<vector<float> >& pattern){
	// Merge theNode with theNext, eliminate the link between them
	// Update degree1, degree2, neighbors, edges, map, demand, and pattern
  int theNode = degree1.back();
  int theNext = neighbors[theNode][0];
  degree1.pop_back();
  nodeType[theNode] = 10;
  int theLink = neighborEdge[theNode][0];
  linkType[theLink] = 10;
  removeNeib(theNext, theNode, neighbors, neighborEdge);
  updateMaps(theNext, theNode, maps);
  maps[theNode].clear();
  neighbors[theNode].clear();
  neighborEdge[theNode].clear();
  updateDemand(theNode, theNext); // reallocate demand
  updateDemandPattern(theNode, theNext, demand, pattern); // reallocate demand with pattern
  if (removeDegree2(degree2, theNext)){
    degree1.push_back(theNext);
  }
  if (isDegree2(theNext, neighbors, neighborEdge, nodeType, linkType))
    degree2.push_back(theNext);
  char* id = new char[256];
  ENCHECK(ENgetnodeid(theNode, id));
}

void mergeSeries(vector<int> &degree2, int* startp, int* endp, int* nodeType, int* linkType, double* headLoss, 
               vector<vector<int> >& neighbors, vector<vector<int> >& neighborEdge,
               vector<vector<int> >& maps, vector<int>& paralNode, vector<vector<float> >& demand, vector<vector<float> >& pattern){
  	// Merge theNode with theNext
	// Update degree2, paralNode, neighbors, edges, map, demand, and pattern
  int theNode = degree2.back();
  degree2.pop_back();
  nodeType[theNode] = 10;
  int theNext1 = neighbors[theNode][0];
  int theLink = neighborEdge[theNode][0];
  int theNext2 = neighbors[theNode][1];
  int theLinkr = neighborEdge[theNode][1];
  // Removal criteria = keep larger diameter pipe
  float removalCriteria1, removalCriteria2;
  ENCHECK(ENgetlinkvalue(theLink, EN_DIAMETER, &removalCriteria1)); 
  ENCHECK(ENgetlinkvalue(theLinkr, EN_DIAMETER, &removalCriteria2)); 
  removalCriteria1 = floor((removalCriteria1*100) + 0.5)/100;
  removalCriteria2 = floor((removalCriteria2*100) + 0.5)/100;
  if (removalCriteria2 == removalCriteria1) {
	  // Secondary removal criteria = keep longer length pipe
	  float secRemovalCriteria1, secRemovalCriteria2;
	  ENCHECK(ENgetlinkvalue(theLink, EN_LENGTH, &secRemovalCriteria1)); 
	  ENCHECK(ENgetlinkvalue(theLinkr, EN_LENGTH, &secRemovalCriteria2)); 
	  if (secRemovalCriteria2 > secRemovalCriteria1) {
		  int temp;
		  temp = theNext1; theNext1 = theNext2; theNext2 = temp;
		  temp = theLink; theLink = theLinkr; theLinkr = temp;
	  }
  }
  else if (removalCriteria2 > removalCriteria1) {
	  int temp;
	  temp = theNext1; theNext1 = theNext2; theNext2 = temp;
	  temp = theLink; theLink = theLinkr; theLinkr = temp;
  }
  linkType[theLinkr] = 10;
  startp[theLink] = theNext1;
  endp[theLink] = theNext2;
  updateNeib(theNext1, theNode, theNext2, theLink, neighbors, neighborEdge);
  updateNeib(theNext2, theNode, theNext1, theLink, neighbors, neighborEdge);
  // Reallocation criteria = nearest neighbor
  float reallocationCriteria1, reallocationCriteria2;
  ENCHECK(ENgetlinkvalue(theLink, EN_LENGTH, &reallocationCriteria1)); 
  ENCHECK(ENgetlinkvalue(theLinkr, EN_LENGTH, &reallocationCriteria2)); 
  if (reallocationCriteria2 < reallocationCriteria1) {
	  updateMaps(theNext2, theNode, maps);
	  updateDemand(theNode, theNext2); // reallocate demand
	  updateDemandPattern(theNode, theNext2, demand, pattern); // reallocate demand with pattern
  }
  else {
	  updateMaps(theNext1, theNode, maps);
	  updateDemand(theNode, theNext1); // reallocate demand
	  updateDemandPattern(theNode, theNext1, demand, pattern); // reallocate demand with pattern
  }
  maps[theNode].clear();
  neighbors[theNode].clear();
  neighborEdge[theNode].clear();
  updateDiameterSeries(theLink, theLinkr);
  float value1, value2;
  ENgetnodevalue(theNext1, EN_HEAD, &value1);
  ENgetnodevalue(theNext2, EN_HEAD, &value2);
  headLoss[theLink] = fabs(value1-value2);
  if (!isDegree2(theNext1, neighbors, neighborEdge, nodeType, linkType))
    removeDegree2(degree2, theNext1);
  if (!isDegree2(theNext2, neighbors, neighborEdge, nodeType, linkType))
    removeDegree2(degree2, theNext2);
  if (isParalNode(theNext2, neighbors, neighborEdge, nodeType, linkType,headLoss)
      && !isParalExit(theNext2, paralNode))
    paralNode.push_back(theNext2);
  if (isParalNode(theNext1, neighbors, neighborEdge, nodeType, linkType,headLoss)
      && !isParalExit(theNext1, paralNode))
    paralNode.push_back(theNext1);
  char* id1 = new char[256];
  ENCHECK(ENgetnodeid(theNode, id1));
  char* id2 = new char[256];
  ENCHECK(ENgetnodeid(theNext1, id2));
  char* id3 = new char[256];
  ENCHECK(ENgetnodeid(theNext2, id3));
  char* id = new char[256];
  ENCHECK(ENgetlinkid(theLinkr, id));
  char* id4 = new char[256];
  ENCHECK(ENgetlinkid(theLink, id4));
}

void mergeParallel(vector<int> &degree1, vector<int> &degree2, vector<int> &paralNode,
             int* startp, int* endp, int* nodeType, int* linkType,double*headLoss, 
             vector<vector<int> >& neighbors, vector<vector<int> >& neighborEdge,
             vector<vector<int> >& maps){
	// Eliminate one of the parallel pipes, keep both nodes
	// Update degree1, degree2, paralNode, neighbors, edges, and map
  int node1 = paralNode.back();
  paralNode.pop_back();
  int node2, link1, link2, loc1, loc2;
  getParal(node1, node2, loc1, loc2, neighbors, neighborEdge, nodeType, linkType, headLoss);
  link1 = neighborEdge[node1][loc1];
  link2 = neighborEdge[node1][loc2];
  // Removal criteria = keep larger diameter pipe
  float removalCriteria1, removalCriteria2;
  ENCHECK(ENgetlinkvalue(link1, EN_DIAMETER, &removalCriteria1)); 
  ENCHECK(ENgetlinkvalue(link2, EN_DIAMETER, &removalCriteria2));
  removalCriteria1 = floor((removalCriteria1*100) + 0.5)/100;
  removalCriteria2 = floor((removalCriteria2*100) + 0.5)/100;
  if (removalCriteria2 == removalCriteria1) {
	  // Secondary criteria = keep longer length pipe
	  float secRemovalCriteria1, secRemovalCriteria2;
	  ENCHECK(ENgetlinkvalue(link1, EN_LENGTH, &secRemovalCriteria1)); 
	  ENCHECK(ENgetlinkvalue(link2, EN_LENGTH, &secRemovalCriteria2)); 
	  if (secRemovalCriteria2 > secRemovalCriteria1) {
		  int temp;
		  temp = link1; link1 = link2; link2 = temp;
	  }
  }
  else if (removalCriteria2 > removalCriteria1) {
	  int temp;
	  temp = link1; link1 = link2; link2 = temp;
  }
  linkType[link2] = 10; 
  removeLink(link2, neighbors[node1], neighborEdge[node1]);
  removeLink(link2, neighbors[node2], neighborEdge[node2]);
  updateDiameterParallel(link1, link2);
  if (isDegree1(node1, neighbors, neighborEdge, nodeType, linkType))
    degree1.push_back(node1);
  if (isDegree1(node2, neighbors, neighborEdge, nodeType, linkType))
    degree1.push_back(node2);
  if (isDegree2(node1, neighbors, neighborEdge, nodeType, linkType))
    degree2.push_back(node1);
  if (isDegree2(node2, neighbors, neighborEdge, nodeType, linkType))
    degree2.push_back(node2);
  if (isParalNode(node1, neighbors, neighborEdge, nodeType, linkType,headLoss)
      && !isParalExit(node1, paralNode))
    paralNode.push_back(node1);
  if (!isParalNode(node2, neighbors, neighborEdge, nodeType, linkType,headLoss))
    removePar(paralNode, node2);
  char* id1 = new char[256];
  ENCHECK(ENgetlinkid(link1, id1));
  char* id2 = new char[256];
  ENCHECK(ENgetlinkid(link2, id2));
}








int main(int argc, char *argv[])
{
  int nnodes, nlinks, ncontrols;
  if (argc < 4) {
	  cerr << "usage: spotSkeleton <input inp file> <pipe diameter threshold> <output inp file> <output map file>"
	     << endl;
	exit(1);
  }
  char *epanet_inp = argv[1];
  double threshvalue = atof(argv[2]);
  char *outfile = argv[3];
  char *mapfile = argv[4];

  cout << "Read inp file... " << endl;
  ENCHECK(ENopen(epanet_inp,"rpt","d.out"));
  ENCHECK(ENgetcount(EN_NODECOUNT, &nnodes));
  ENCHECK(ENgetcount(EN_LINKCOUNT, &nlinks));
  ENCHECK(ENgetcount(EN_CONTROLCOUNT, &ncontrols));
  ENCHECK(ENsolveH());

	cout << "Setup network... " << endl;
  int* startp = new int[nlinks+1];
  int* endp = new int[nlinks+1];
  int* nodeType = new int[nnodes+1];
  int* linkType = new int[nlinks+1];
  double* headLoss = new double[nlinks+1];
  vector<vector<int> > neighbors(nnodes+1);
  vector<vector<int> > neighborEdge(nnodes+1);
  vector<vector<int> > maps(nnodes+1);
  vector<vector<float> > demand(nnodes+1);
  vector<vector<float> > pattern(nnodes+1);
  vector<int> degree1;
  vector<int> degree2;
  vector<int> paralNode;
  setEdge(nlinks, startp, endp);
  setLinkType(nlinks, threshvalue, linkType);
  setHeadLoss(nlinks, headLoss);
  setNodeType(nnodes, nodeType);
  setNeighbors(nlinks, neighbors, neighborEdge);
  setMap(nnodes, maps);
  setDemandPattern(nnodes, demand, pattern);
  setSimpleControl(ncontrols, nodeType, linkType);
  setRuleControl(epanet_inp, nodeType, linkType);
  setDegreeList(nnodes, neighbors, neighborEdge, nodeType, linkType, headLoss, degree1, degree2, paralNode);

	cout << "Skeletonize network...  " <<endl;
  do{
	  cout << "  Branch trim  " << degree1.size() << endl;
	  while (degree1.size()>0)
			branchTrim(degree1, degree2, nodeType, linkType, neighbors, neighborEdge, maps, demand, pattern);
	  cout << "  Series pipe merge  " << degree2.size() << endl;
	  while (degree2.size()>0)
		  mergeSeries(degree2, startp, endp, nodeType, linkType,headLoss, neighbors, neighborEdge, maps, paralNode, demand, pattern);
	  cout << "  Parallel pipe merge  " << paralNode.size() << endl;
	  while (paralNode.size()>0)
		  mergeParallel(degree1, degree2, paralNode, startp, endp, nodeType, linkType,headLoss, neighbors, neighborEdge, maps);
  
  } while ((degree1.size()>0)||(degree2.size()>0));

	cout << "Write inp and map file... " << endl;
  printOutFile(nnodes, nlinks, startp, endp, nodeType, linkType, demand, pattern, epanet_inp, outfile);
  printMaps(nnodes, nodeType, mapfile, maps);

  delete [] headLoss;
  delete [] startp;
  delete [] endp;
  delete [] nodeType;
  delete [] linkType;
  return 0;
}
