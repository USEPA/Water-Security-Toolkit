#include <source_inversion/InversionNetworkSimulator.hpp>
#include <merlionUtils/TaskTimer.hpp>
#include <merlion/TriSolve.hpp>
#include <merlion/BlasWrapper.hpp>
#include <merlion/SparseMatrix.hpp>
#include <merlion/Merlion.hpp>

#include <algorithm>
#include <cmath>
#include <set>
#include <vector>
#include <time.h>
#include <fstream>
#include <epanet2.h>
#include <epanetmsx.h>
//#include <tevautil.h>
extern "C" {
   #include <enl.h>
}
#include <erd.h>

#include <tevautil.h>
#include <iostream>

#ifdef WIN32
#include "mem_mon.h"
#endif

#ifdef WIN32
#include <fcntl.h>
int mkstemp(char *tmpl)
{
	int ret=-1;
	mktemp(tmpl);
	ret=open(tmpl,O_RDWR|O_BINARY|O_CREAT|O_EXCL|_O_SHORT_LIVED,_S_IREAD|_S_IWRITE);
	return ret;
}
#endif

// Define Tevasim helper functions
#define MIN(x,y) (((x)<=(y)) ? (x) : (y))     /* minimum of x and y    */
#define MAX(x,y) (((x)>=(y)) ? (x) : (y))     /* maximum of x and y    */
#define MOD(x,y) ((x)%(y))                    /* x modulus y           */
#define ERDCHECKCPP(x) if ((ERRNUM=x) > 0) {ERD_Error(ERRNUM);}
#define TEVAFILEOPEN(x,f) if ((x) == NULL) {TEVASimError(1,"Error opening file: %s",(f));}

 // forward declaration
std::vector<std::string> tokenize_measurement_line(const char* str, const char* delim);
int run_hydraulics_teva(PERD db);
//int run_quality_teva(PERD db, FILE *tsifile, PSourceData source, int stopTime, std::vector<std::pair<int,int> >& measurements, float threshold);
void TEVASimError(int exitCode,const char *errmsg, ...);
bool run_simulations_merlion(InversionNetworkSimulator& net, float threshold);

void InversionNetworkSimulator::clear()
{
  if (!MeasureList_.empty()) {
    for (std::vector<Measure*>::iterator pos = MeasureList_.begin(), stop = MeasureList_.end(); pos != stop; ++pos) {
      delete *pos;
    }
  }
  MeasureList_.clear();
  
  /* Base Class clear() */
   NetworkSimulator::clear();
}

void InversionNetworkSimulator::ReadMEASUREMENTS(std::string Measurements_filename)
{
  std::ifstream file;
  file.precision(8);
  file.open(Measurements_filename.c_str(), std::ios_base::in);
  
  if (!file.is_open()) 
    {
      std::cerr << std::endl;    
      std::cerr << "Failed to open Measurements file: " << Measurements_filename << std::endl;
      std::cerr << std::endl;
      exit(1);
    }
  const int max_chars_per_line = 512;
  char *buffer(new char[max_chars_per_line]);	
  //int line_cntr = 1;
  bool first_space_found = false;
  
  while (file.good())
    {  
      buffer[0] = '\0';
      file.getline(buffer, max_chars_per_line);
      std::vector<std::string> tokens = tokenize_measurement_line(buffer, " \t");
      int n_tokens = tokens.size();
      
      //std::cout << line_cntr++ << " " << n_tokens << std::endl;
      if (n_tokens == 0) {
	first_space_found = true;
	continue;
      }
      
       std::string value = tokens[0];

       if (value.compare("#") == 0) {
          continue;
       }

       if (n_tokens < 3) {
          std::cerr << std::endl;
          std::cerr << "Failed to read Measurements file: " << Measurements_filename << std::endl;
          std::cerr << std::endl;      
          exit(1);
       }
       //Value of the measure
       value = tokens[n_tokens -1];
       float Measure_Concentration = atof(value.c_str());
       if (Measure_Concentration < 0) {
          std::cerr << std::endl;      
          std::cerr << "Measurements File Error: measure must be a positive number.\n" << std::endl;
          std::cerr << std::endl;      
          exit(1);
       }
       //Time step
       value = tokens[n_tokens -2];
       int Measure_seconds = atof(value.c_str());
       int Measure_timestep = merlionUtils::SecondsToNearestTimestepBoundary(Measure_seconds, model_.qual_step_minutes);

       if(Measure_timestep < 0 || Measure_timestep > model_.n_steps)
       {
          std::cerr << std::endl;
          std::cerr << "Measurements File Error: Measure time does not take place within simulation period.\n" << std::endl;
          std::cerr << std::endl;
          exit(1);
       }
       //Measure_Node
       value = tokens[n_tokens -3];
       // check that the node name is in the merlion model
       int node_id = model_.NodeID(value);
       Measure* measureToAdd=new Measure(value.c_str(),Measure_Concentration,Measure_seconds);
       MeasureList_.push_back(measureToAdd);
       if(!first_space_found)
	 grabsample_ids_.insert(node_id);

    }
    file.close();
    delete [] buffer;
 }

std::vector<std::string> tokenize_measurement_line(const char* str, const char* delim)
{
  std::vector<std::string> tokens;
  char* token = strtok (const_cast<char*>(str), const_cast<char*>(delim));
  while (token != NULL) {
    tokens.push_back(token);
    token = strtok (NULL, delim);
  }
  return tokens;
}

int InversionNetworkSimulator::reduceMatrixNrows()
{
  wqm_inverse_.TransformToCSRMatrix();
  int nEmptyrows=0;
  for(int i=0;i<wqm_inverse_.NRows();i++)
    {
      int p_col_start = wqm_inverse_.pRows()[i];
      int p_col_stop = wqm_inverse_.pRows()[i+1];
      if(p_col_start==p_col_stop)
	{
          nEmptyrows++;                  
	}
    }
  return wqm_inverse_.NRows()-nEmptyrows;
}

//This must be changed! the impact set should be in networkSimulator and should be created when the R_ matrix is build
std::set<int>& InversionNetworkSimulator::ImpactNT_IDX(float wqm_tol, float meas_thresh)
 {
    impactNTidx_.clear();
    //std::cout<<"Inversion Meas thresh : "<<meas_thresh<<std::endl;
    std::map<int,std::string> node_name_map;
    for (int i = 0, stop = model_.n_nodes; i < stop; ++i) {
      node_name_map[i] = model_.NodeName(i);
    }

    int N_node_cut=allowedNodes_.size();
    //std::set<int> impact_nodes;

    for(std::vector<Measure*>::iterator pos_t=MeasureList_.begin(),l_end=MeasureList_.end();pos_t!=l_end;++pos_t)
      {
	Measure* meas = *pos_t;
	//std::cout << "meas " << meas->nodeName()<<"\n";
	if (meas->value() == 1.0) // If measurement is 1 add all events above the wqm_tol to the impact list
	{
	    int MeasID = model_.NodeID(meas->nodeName());
	    int MeasTimeStep = meas->Measure_timestep(model_.qual_step_minutes);
	    int Meas_nt_idx = MeasID*model_.n_steps + MeasTimeStep;
	    int u_row = model_.perm_nt_to_upper[Meas_nt_idx];
	    int p_row_start = wqm_inverse_.pRows()[u_row];
	    int p_row_stop = wqm_inverse_.pRows()[u_row+1]-1;
	    
	    for (int p = p_row_start; p <= p_row_stop; ++p)
	      {
		int ut_idx = wqm_inverse_.jCols()[p];
		if (wqm_inverse_.Values()[p] > wqm_tol)
		  {
		    int nt_idx = model_.perm_upper_to_nt[ut_idx];
		    int nodeID = nt_idx/model_.n_steps;
		    std::string nodeName = node_name_map[nodeID];
		    bool found=false; 

		    std::vector<std::string>::iterator it;     
		    // Find out if a list of allowed nodes is provided and the current injection is at one of the allowed nodes 
		    it = find (allowedNodes_.begin(), allowedNodes_.end(), nodeName);
		    if(N_node_cut>0 && it!=allowedNodes_.end()) {
		      found=true;
		    } 

		    //std::cout<<"Number of Allowed nodes : " << N_node_cut << std::endl;
	
		    if((N_node_cut==0)||found) // simulation is done only for the allowed nodes
		      {
			impactNTidx_.insert(nt_idx);
		      }
		    //impact_nodes.insert(nt_idx/model_.n_steps);
		  }  
	      }
	 }
	/*else // If measurement is 0 add only the events that will have a positive measure at the current sensor location
	  {
	    int MeasID = model_.NodeID(meas->nodeName());
	    int MeasTimeStep = meas->Measure_timestep(model_.qual_step_minutes);
	    int Meas_nt_idx = MeasID*model_.n_steps + MeasTimeStep;
	    int u_row = model_.perm_nt_to_upper[Meas_nt_idx];
	    int p_row_start = wqm_inverse_.pRows()[u_row];
	    int p_row_stop = wqm_inverse_.pRows()[u_row+1]-1;
	    float c = 0.0;

	    for (int p = p_row_start; p < p_row_stop; ++p)
	      {

		int ut_idx = wqm_inverse_.jCols()[p];
	       if (rhs] > 0.0) && (wqm_inverse_.Values()[p] > wqm_tol))
		  {
		    int nt_idx = model_.perm_upper_to_nt[ut_idx];
		    impactNTidx_.insert(nt_idx);
		  }  
	      }
	    

	  }
       */

      }
    //std::cout<<"Possible Impact Nodes : "<<impact_nodes.size()<<std::endl;
    return impactNTidx_;
 }

 void InversionNetworkSimulator::readAllowedNodes(std::string allowedFilename)
 {
    std::ifstream infile(allowedFilename.c_str());
    if(infile.is_open())
    {
       std::string reader;
       while(!infile.eof())
       {
          infile>>reader;
 	 allowedNodes_.push_back(reader);   
       }
    }
    infile.close();
 }

 int InversionNetworkSimulator::reduceMatrixNcols()
 {
    wqm_inverse_.TransformToCSCMatrix();
    int nEmptycols=0;
    for(int i=0;i<wqm_inverse_.NCols();i++)
    {
       int p_row_start = wqm_inverse_.pCols()[i];
       int p_row_stop = wqm_inverse_.pCols()[i+1];
       if(p_row_start==p_row_stop)
       {
          nEmptycols++;                  
       }
    }
    return wqm_inverse_.NCols()-nEmptycols;
 }


std::vector<std::string> InversionNetworkSimulator::EventProbabilities(float pf, 
								       float conf, 
								       std::string prefix,
								       float threshold,
								       float wqm_tol, 
								       bool merlion,
								       std::string inp_fname)
 {
    time_t start = time(NULL);
    std::cout<<"\nRunning Inversion:\n";                                                                                          
    std::map<int,std::string> node_name_map;
    for (int i = 0, stop = model_.n_nodes; i < stop; ++i) {
      node_name_map[i] = model_.NodeName(i);
    }

    
    //exit(1);

    //Find the impact tuples
    std::set<int> events=ImpactNT_IDX(wqm_tol, threshold);

    // Convert reduced Matrix to CSC
    wqm_inverse_.TransformToCSCMatrix();

    // Print Matrix stats 
    /*std::ofstream stats_file;
    stats_file.open("reduced_model_stats.txt", std::ios::out);
    wqm_inverse_.PrintStats(stats_file);
    stats_file.close();
    */

    time_t test_time = time(NULL) - start;
    //std::cout<<"What is taking so long: "<<test_time<<"\n";
    std::cout<<"\n Total number of simulations: "<<events.size()<<std::endl;
    int iter = 0;
    //std::vector<int> Prob_IDX;
    float strength=10;
    //std::vector<int> matches;
    std::set<std::string> node_names;
    //merlionUtils::Injection tmp_tox;
    
    //float *rhs(new float[model_.N]);	  
    int num_measures = MeasureList_.size();
    float *conc_short(new float[num_measures]);
    int* perm_nt_to_short(new int[model_.N]);
    BlasInitZero(num_measures, conc_short);
    /*for(int i = 0;i<578;i++){
      conc[i]= 0.0;
      }*/
    //std::cout<<"init started \n";
    //std::cout<<"init complete \n";
    //    std::cout<<"Num Measurements: "<<num_measures<<std::endl;
    //std::map<int,float> rhs;
    //Timing variables 
    time_t conc_calc_time = (time_t)0.0;
    time_t meas_loop_total = (time_t)0.0;
    time_t sim_start = time(NULL);
    //std::cout<<"Filtered size : " << Filtered_MeasureList_.size()<<"\n";
  // Loop through all the events and simulate an injection using the reduced model

    merlionUtils::TaskTimer all_events_sim_time;
    merlionUtils::TaskTimer c_calc_time(false);
    merlionUtils::TaskTimer meas_match(false);
    merlionUtils::TaskTimer blas_reset(false);

    std::vector<std::pair<int,int> > measurements;
    std::vector<std::vector<int> > contaminated_nodes;
    int short_id = 0;
    for(std::vector<Measure*>::iterator pos_t=MeasureList_.begin(),l_end=MeasureList_.end();pos_t!=l_end;++pos_t) {
      Measure* meas = *pos_t;
      int MeasID = model_.NodeID(meas->nodeName());
      int MeasTimeStep = meas->Measure_timestep(model_.qual_step_minutes);
      int Meas_nt_idx = MeasID*model_.n_steps + MeasTimeStep;
      perm_nt_to_short[Meas_nt_idx] = short_id++;
      if(meas->Type() == "READ_CONC") {
	if (meas->value() > threshold) {
	  meas->value() = 1.0;
	  measurements.push_back(std::pair<int,int>(Meas_nt_idx,1));
	}
	else {
	  meas->value() = 0.0;
	  measurements.push_back(std::pair<int,int>(Meas_nt_idx,0));
	}
      }
      else {
	if (meas->value() > 0.5) {
	  measurements.push_back(std::pair<int,int>(Meas_nt_idx,1));
	}
	else {
	  measurements.push_back(std::pair<int,int>(Meas_nt_idx,0));
	}
      }
    }
    
    //*********************** USE MERLION TO SIMULATE EVENTS *************************************   
    
    if(merlion) {


      for(std::set<int>::iterator pos = events.begin(), p_end = events.end(); pos != p_end; ++pos)
	{
	  
	  //std::cout<<"new event\n";
	  int nt_idx= *pos;
	  //Mass simulations for reservoirs or tanks since they don't support flowpaced
	  //bool mass_inj_only = false;
	  
	  /*	std::vector<int>::iterator itt;     
		itt = find(model_.reservoirs.begin(), model_.reservoirs.end(), nt_idx/model_.n_steps);
		if(itt != model_.reservoirs.end())
		mass_inj_only = true;
		for(std::vector<std::pair<int, int> >::iterator itt2 = model_.tanks.begin(),itt_end = model_.tanks.end();itt2 != itt_end; ++itt2)     
		{
		if(itt2->first == nt_idx/model_.n_steps)
		{mass_inj_only = true;
		break;
		}
		}
	  */
	  
	  iter++;
	  
	  int nodeID = nt_idx/model_.n_steps;
	  std::string nodeName = node_name_map[nodeID];
	  Prob_IDX.push_back(nt_idx);
	  int timestep = nt_idx%model_.n_steps;
	  node_names.insert(nodeName);

	  //create injection
	  
	  //tmp_tox.NodeName()= nodeName;
	  //if(mass_inj_only)
	  //tmp_tox.Type() = merlionUtils::StringToInjType("MASS");
	  //else
	  //tmp_tox.Type() = merlionUtils::StringToInjType("MASS");
	  //tmp_tox.Strength() = strength;
	  //tmp_tox.StartTimeSeconds() = timestep*model_.qual_step_minutes*60;
	  //tmp_tox.StopTimeSeconds() = (model_.n_steps-1)*model_.qual_step_minutes*60;
	  
	  //int u_idx_max;
	  //float tmp_tox_mass_injected_g;
	  //tmp_tox.SetArray(model_, rhs, tmp_tox_mass_injected_g, u_idx_max);
	  
	  //tmp_tox.SetArray(model_, rhs);
	  int n_matches=0;
	  // iterate through the list of measurements and calculate concentration for each corresponding node   
	  time_t meas_loop_start = time(NULL);
	  


	    c_calc_time.Start();
	    int nt_idx_start = nodeID*model_.n_steps + timestep;
	    int nt_idx_stop = nodeID*model_.n_steps + model_.n_steps;
	    
	    
	    for(int ntx_idx = nt_idx_start; ntx_idx < nt_idx_stop; ++ntx_idx)
	      {
		if(model_.D[ntx_idx] != 0)
		  {
		    int u_col = model_.perm_nt_to_upper[ntx_idx];
		    int pCol_start = wqm_inverse_.pCols()[u_col];
		    int pCol_stop = wqm_inverse_.pCols()[u_col+1]-1;
		    for(int i = pCol_start; i<= pCol_stop; i++) {
		      int meas_nt = model_.perm_upper_to_nt[wqm_inverse_.iRows()[i]];
		      //conc[meas_nt] += wqm_inverse_.Values()[i];
		      conc_short[perm_nt_to_short[meas_nt]] += wqm_inverse_.Values()[i];
		    }
		  }
	      }
	    
	    c_calc_time.Stop();
	    meas_match.Start();



	    //std::cout<<"threshold : " << threshold << std::endl;
	    //for(std::vector<Measure*>::iterator pos_t=MeasureList_.begin(),l_end=MeasureList_.end();pos_t!=l_end;++pos_t)
	    for(std::vector<std::pair<int,int> >::iterator pos_t=measurements.begin(),l_end=measurements.end();pos_t!=l_end;++pos_t)
	      {
		/*
		  Measure* meas = *pos_t;
		  meas_match.Start();
		  int MeasID = model_.NodeID(meas->nodeName());
		  meas_match.Stop();
		  int MeasTimeStep = meas->Measure_timestep(model_.qual_step_minutes);
		  int Meas_nt_idx = MeasID*model_.n_steps + MeasTimeStep;
		*/
		int Meas_nt_idx = pos_t->first;
		int Meas_upper_idx = model_.perm_nt_to_upper[Meas_nt_idx];
		//if ( Meas_upper_idx <= model_.perm_nt_to_upper[nt_idx]) { 
		//if (conc[Meas_nt_idx] > threshold) {
		if (conc_short[perm_nt_to_short[Meas_nt_idx]] > threshold) {
		  if (pos_t->second == 1) {
		    ++n_matches;
		}
	      }
	      else if (pos_t->second == 0) {
		++n_matches;
	      }
	      //}
	      
	      /*
	      //std::cout<< MeasID << "\t" << MeasTimeStep << "\t" << c << "\t" <<  meas->value() <<std::endl;
	      if(c > threshold){c=1.0;}
	      else{c=0.0;}
	      // Convert measurement to 1 or 0 
	      if(meas->Type() == "READ_CONC" && meas->value() > threshold){meas->value() = 1.0;}
	      else if(meas->Type()=="READ_CONC"){
	      meas->value() = 0.0;
	      }
	      
	      if(c == meas->value()){
	      n_matches++;
	      }
	      */
	    }
	  meas_match.Stop();
	  
	  //exit(1);
	  meas_loop_total += time(NULL) - meas_loop_start;
	  matches.push_back(n_matches);
	  //std::cout<<nodeName<<"\t"<<timestep<<"\t"<<n_matches<<"\n";
	  // Reset the rhs
	  //tmp_tox.ClearArray(model_, rhs);
	  //BlasResetZero(model_.N,rhs); 
	  // Time remaining calculation
	  //std::cout<<"Simulations completed... ";
	  //std::cout<<iter<<" of "<<events.size()<<"                       \r";
	  
	  
	  
	  //tmp_tox.ClearArray(model_, rhs);
	  //BlasResetZero(model_.N,rhs);
	  //std::cout<<"Number of meas: "<< num_measures <<" conc : "<< conc << std::endl;
	  blas_reset.Start();
	  //BlasResetZero(model_.N,conc);
	  BlasResetZero(num_measures,conc_short);
	  blas_reset.Stop();
	  //exit(1);
	  
	}
      
      //exit(1);
      
      //all_events_sim_time.StopAndPrint(std::cout, "\n\t - Ran all possible event simulations ");
      //c_calc_time.StopAndPrint(std::cout, "\n\t - Total concentration calc time ");
      //meas_match.StopAndPrint(std::cout, "\n Measurement match time ");
      //blas_reset.StopAndPrint(std::cout, "\n Blas reset time ");
      
      
      //exit(1);
      //delete [] conc;
      delete [] conc_short;
      delete [] perm_nt_to_short;


    } // End if (merlion)

    //*********************** USE EPANET TO SIMULATE EVENTS *************************************   

    else {
      // Define input variables needed by tevasim run_flushsim
      //char* tsgfname = (char*)"tmp_inversion_candidates.tsg";
      //char* tsifname = (char*)"tmp_inversion_candidates.tsi";
      char* erdIndexName = (char*)"tmp_inversion_db.index.erd";
      char* erdQualName = (char*)"tmp_inversion_db-1.qual.erd";
      char* hydFname = (char*)"hydraulics.hyd";
      char* erdName = (char*)"tmp_inversion_db.erd";
      char* epanetinpfname = (char*)inp_fname.c_str();
      char* epanetoutfname = (char*)"tmp_inversion_epanet.out";
      char* msxfname = NULL;
      char* msx_species = NULL;
      bool isRLE = false;
      int StopTimeSeconds = (model_.n_steps-1)*model_.qual_step_minutes*60;

      // original tevasim variables
      int version;
      int storageMethod=-1, runensemble=0;
      time_t cpustart, cpustop;
      double cpuelapsed;
      PERD db;
      PSourceData source = (PSourceData)calloc(1,sizeof(SourceData));
      int isMSX;
      FILE *tsgfile=NULL, *tsifile=NULL;
      char tTSIFname[32]="";
      char tTSGFname[32]="";
      
      double **initQual;  // species,node: initial quality from msx input file
      source->source=(PSource)calloc(MAXSOURCES,sizeof(Source));
      source->nsources=0;
#ifdef WIN32
      startMemoryMonitorThread("memlog.txt",1000);
#endif

      int tsgfd;
      strcpy(tTSGFname,"tsgXXXXXX");
      tsgfd=mkstemp(tTSGFname);
      if( (tsgfile=fdopen(tsgfd,"w+b") ) == NULL ) {
	TEVASimError(1,"Can not create temporary TSG file\n");
      }
      // write candidate injection events in a tsg file
      //std::ofstream temp_inv_cand_out;
      //temp_inv_cand_out = "tmp_inversion_candidates.tsg";
      //temp_inv_cand_out.open(tsgfname,std::ios_base::out | std::ios_base::trunc);
      for(std::set<int>::iterator pos = events.begin(), p_end = events.end(); pos != p_end; ++pos)
	{
	  int nt_idx= *pos;
	  //merlionUtils::PInjection tmp(new merlionUtils::Injection);
	  int nodeID = nt_idx/model_.n_steps;
	  std::string nodeName = node_name_map[nodeID];
	  Prob_IDX.push_back(nt_idx);
	  int timestep = nt_idx%model_.n_steps;
	  node_names.insert(nodeName);
	  int StartTimeSeconds = timestep*model_.qual_step_minutes*60;
	  
	  //temp_inv_cand_out<< nodeName << "\t" << "FLOWPACED\t1e+4\t" 
	  //	   <<  StartTimeSeconds << "\t" << StopTimeSeconds << "\n";
	  fprintf(tsgfile, "%s \t FLOWPACED \t 1e+4 \t %d \t %d \n", nodeName.c_str(), StartTimeSeconds, StopTimeSeconds);
	}
      //temp_inv_cand_out.close();
      fflush(tsgfile);
      close(tsgfd);
      
      TEVAFILEOPEN(tsgfile = fopen(tTSGFname, "r"),tTSGFname);
      runensemble=1;

      ENCHECK(ENopen((char*)epanetinpfname,(char*)epanetoutfname, (char*)""));
      
      if (isRLE) {ERDCHECKCPP(ERD_create(&db, erdName, teva, rle));}
      else {ERDCHECKCPP(ERD_create(&db, erdName, teva, lzma));}
      
      ERDCHECKCPP(ENL_getNetworkData(db, (char*)epanetinpfname, (char*)msxfname, (char*)msx_species));
      // set hydraulic storage flags
      ERDCHECKCPP(ERD_setHydStorage(db, TRUE, TRUE, TRUE, FALSE, TRUE));
      // ENTU_Initialize(epanetinpfname, tsofname, storageMethod, tsofileversion, &tso, &net, &nodes, &links, &sources);
      /* (Optionally) Process threat simulation generator file and write simulation input file */
      if(tsgfile) {
	if(!tsifile) { /* temporary tsi file */
	  int tsifd;
	  strcpy(tTSIFname,"tsiXXXXXX");
	  tsifd=mkstemp(tTSIFname);
	  if( (tsifile=fdopen(tsifd,"w+b") ) == NULL ) {
	    TEVASimError(1,"Can not create temporary TSI file\n");
	  }
	}
	ENL_writeTSI(db->network, db->nodes, source->source, tsgfile, tsifile);
      }
      
      // Run hydraulics with epanet - This has to be changed to load the hydraulics file generated before
      
      int errNum = run_hydraulics_teva(db);
      
      if(errNum != 0) {	// check error
	printf(":: Problem getting Epanet hydraulic results. \n\n");
	exit(1);
      }

      int return_val = run_quality_teva(db,tsifile,source, StopTimeSeconds, measurements, threshold);

     free(source->source); /* I'd rather this be part of ReleaseNetworkData... */
     free(source); /* I'd rather this be part of ReleaseNetworkData... */
     /* close any files that have been opened */
     
     // Close and delete temperory files
     if(tsgfile) {fclose(tsgfile);}
     if(tsifile) {fclose(tsifile);}
     if(strlen(tTSIFname)!=0) {
       std::remove(tTSIFname);
     }
     if(strlen(tTSGFname)!=0) {
       std::remove(tTSGFname);
     }
     std::remove(erdName);
     std::remove(epanetoutfname);
     std::remove(erdIndexName);
     std::remove(erdQualName);
     std::remove(hydFname);
    }

    ////////////////////// Calculate probabilities ////////////////
 
    time_t prob_start = time(NULL);

    //Calculating probabilities
    int N_measurements=MeasureList_.size();
    std::vector<double> raw_prob_e_m; 
    for(int i = 0;i< matches.size(); i++)
      {  
	double prob = pow(1-pf,matches[i])*pow(pf,N_measurements-matches[i]);
	raw_prob_e_m.push_back(prob);
      }
                                                                                                 
    if(matches.size() != Prob_IDX.size()){
      std::cerr << std::endl;           
      std::cerr << "Error: Measurement vector size not equal to Probability vector size" << std::endl;             
      std::cerr << std::endl;
      exit(1);
    }
   
   // Sorting raw probabilities (standard bubble sort is used)       
    
    int temp_prob_id = 0;  
    double temp_prob = 0.0; 
    for(int j = 0; j<raw_prob_e_m.size();j++)
      { for(int i = j+1;i< raw_prob_e_m.size(); i++)
	  if(raw_prob_e_m[i] > raw_prob_e_m[j])
	    { temp_prob = raw_prob_e_m[i];
	      raw_prob_e_m[i] = raw_prob_e_m[j];
	      raw_prob_e_m[j] = temp_prob;
	      temp_prob_id = Prob_IDX[i];
	      Prob_IDX[i] = Prob_IDX[j];
	      Prob_IDX[j] = temp_prob_id;
	    }    
      }

   // Filtering highest probability value for a particular node 
    std::vector<int> most_probable_times; //Most probable timestep for a particular node
    std::vector<int> filtered_IDX; 
    std::vector<double> filtered_raw_prob;
    double raw_prob_sum = 0.0f;
    for(int j = 0; j<Prob_IDX.size();j++){
      if(std::find(filtered_IDX.begin(), filtered_IDX.end(), Prob_IDX[j]/model_.n_steps) == filtered_IDX.end()){
	filtered_IDX.push_back(Prob_IDX[j]/model_.n_steps);
	filtered_raw_prob.push_back(raw_prob_e_m[j]);
	most_probable_times.push_back((Prob_IDX[j]%model_.n_steps)*model_.qual_step_minutes*60);
	raw_prob_sum += raw_prob_e_m[j];
      }
    }

    
    //std::cout<<"Raw Probability Sum : "<<raw_prob_sum<<"\n";
    if(raw_prob_sum == 0.0)
      {
	std::cerr << std::endl;           
	std::cerr << "ERROR:Cumulative probability is zero!! This happens when measurement failure rate is set to 0.0 and there is no perfect match." << std::endl;
	std::cerr << std::endl;
	exit(1);
      }  
    // Normalizing and adding top probable nodes to a vector
    double top_prob_sum = 0.0f;
    // adding the first value by default
    most_probable_nodes_.push_back(model_.NodeName(filtered_IDX[0]));
    top_prob_.push_back(filtered_raw_prob[0]/raw_prob_sum);
    top_prob_sum += filtered_raw_prob[0]/raw_prob_sum;
    
    if(filtered_IDX.size() > 1)
      {
	for(int j = 1; j<filtered_IDX.size();j++){ // Note this loop start at 1 
	  double tmp_norm_prob = filtered_raw_prob[j]/raw_prob_sum;
	  if (((top_prob_sum >= conf)&&(tmp_norm_prob != top_prob_[j-1])))	    
	    break;
	  else{
	    most_probable_nodes_.push_back(model_.NodeName(filtered_IDX[j]));
	    top_prob_.push_back(tmp_norm_prob);
	    top_prob_sum += tmp_norm_prob;
	  }
	}
      }


   //Printing Probability Calculation Time
  time_t prob_total_time = time(NULL) - prob_start; 
  //std::cout<<"\n prob calc Time : "<<prob_total_time<<std::endl;

  merlionUtils::TaskTimer  print_file_time;

  //######### Rest of the code should have a seperate function in Datawriter #########

   //Printing json and tsg file
  std::ofstream out_json;
  std::string inversion_resultfile_name = prefix;
  inversion_resultfile_name += "inversion.json";
  out_json.open(inversion_resultfile_name.c_str(), std::ios_base::out);
  std::ofstream out_tsg;
  std::string profile_name = prefix + "profile.tsg";
  out_tsg.open(profile_name.c_str(), std::ios_base::out);

  out_json<<"[";   
  for(int i = 0; i < most_probable_nodes_.size()-1;i++)
    {
      out_json<<"{\"Objective\": "<<top_prob_[i]<<",\"Nodes\":[{\"Name\": \""<<most_probable_nodes_[i]<<"\"}]},";
    }
  
  out_json<<"{\"Objective\": "<<top_prob_[most_probable_nodes_.size()-1]<<",\"Nodes\":[{\"Name\": \""<<most_probable_nodes_[most_probable_nodes_.size()-1]<<"\"}]}]";
  out_json.close();


  if (most_probable_nodes_.size()<=100)
    {
  
      for(int i = 0; i < most_probable_nodes_.size();i++){
	out_tsg<<most_probable_nodes_[i]<<"\t MASS \t 10000 \t"<<most_probable_times[i]<<"\t"<<(model_.n_steps-1)*model_.qual_step_minutes*60<<"\n";
      }
      out_tsg.close();
    }
  else
    {
      for(int i = 0; i < 100;i++){
	out_tsg<<most_probable_nodes_[i]<<"\t MASS \t 10000 \t"<<most_probable_times[i]<<"\t"<<(model_.n_steps-1)*model_.qual_step_minutes*60<<"\n";
      }
      out_tsg.close();
      
    }

  // This will count number of uncertain nodes
  if(snap_timestep_ != -1)
  {
    //First run sims of most probable events
    //EventList probable_events_;
    for(int i = 0; i < most_probable_nodes_.size() && i<100 ;i++){
      PEvent newEvent(new Event);
      std::stringstream scen_name;
      scen_name << i;
      newEvent->Name()= scen_name.str();
      merlionUtils::PInjection tmp(new merlionUtils::Injection);
      tmp->NodeName() = most_probable_nodes_[i];
      tmp->Type() = merlionUtils::StringToInjType("MASS");
      tmp->Strength() = 10000;
      tmp->StartTimeSeconds() = most_probable_times[i];
      tmp->StopTimeSeconds() = (model_.n_steps-1)*model_.qual_step_minutes*60;
      newEvent->Injections().push_back(tmp);
      probable_events_.push_back(newEvent);
    }
    run_simulations_merlion(*this,threshold);
    for(EventList::iterator event_pos_i=probable_events_.begin(),event_end=probable_events_.end();event_pos_i!=event_end;event_pos_i++)
    {
      	const Event& event = **event_pos_i;
	merlionUtils::InjectionList injections = event.Injections();
	for(merlionUtils::InjectionList::iterator it_inj = injections.begin(), it_inj_end=injections.end();it_inj!=it_inj_end;++it_inj){
	  //std::cout<<(*it_inj)->NodeName() << " " << (*it_inj)->Type() << " " << (*it_inj)->StartTimeSeconds() << " " <<(*it_inj)->StopTimeSeconds() <<"\n";
	}
        std::set<int> infectedNodes = event.infectedNodes();
	/*std::cout<< "Senario "<<event.Name() << " ";
	for(std::set<int>::iterator it_node = infectedNodes.begin(),it_node_end = infectedNodes.end();it_node!=it_node_end;++it_node)
	{
	    std::cout<< model_.NodeName(*it_node) << " ";
	}
	std::cout <<"\n";*/
    }
    
  } 
  
  print_file_time.StopAndPrint(std::cout, " Total file print time " );;
  // std::cout<<"Total File Print time : "<<print_total_time<<"\n";

  raw_prob_e_m.clear();
  //norm_prob_e_m.clear();
  //node_cum_prob.clear();
  //ordered_node_names.clear();
  node_names.clear();
  matches.clear();
  most_probable_times.clear(); //Most probable timestep for a particular node
  filtered_IDX.clear(); 
  filtered_raw_prob.clear();

  //  delete [] tox_mass_inj_gpmin;
  
  return most_probable_nodes_;             
}

void InversionNetworkSimulator::FilterMeasurements()
{
  std::set<int> positive_node_ids;
  for (std::vector<Measure*>::iterator pos = MeasureList_.begin(), stop = MeasureList_.end(); pos != stop; ++pos) {
    Measure* meas = *pos;
    int MeasID = model_.NodeID(meas->nodeName());
    if(meas->value() > 0.0)
      {positive_node_ids.insert(MeasID);}
  }
  
  for (std::vector<Measure*>::iterator pos = MeasureList_.begin(), stop = MeasureList_.end(); pos != stop; ++pos) {
    Measure* meas = *pos;
    int MeasID = model_.NodeID(meas->nodeName());
    int MeasTimeStep = meas->Measure_timestep(model_.qual_step_minutes);
    
    std::set<int>::iterator it = positive_node_ids.find(MeasID);
    std::set<int>::iterator itt = grabsample_ids_.find(MeasID);
    if ((it != positive_node_ids.end())|| (itt != grabsample_ids_.end()))
      {
	Filtered_MeasureList_.push_back(meas);
      }
  }
  
}

void TEVASimError(int exitCode,const char *errmsg, ...)
{
        va_list ap;
 
        va_start(ap,errmsg);
        vfprintf(stderr,errmsg,ap);
        va_end(ap);
 
        exit(exitCode);
}

int run_hydraulics_teva(PERD db)
{
    long entime, tstep, rtime, rstep, astep;
    int status,wrncnt,hsteps;
    char atime[10];
    PNetInfo net=db->network;
    PNodeInfo nodes=db->nodes;
    PLinkInfo links=db->links;
    int errcode;
    tstep = net->reportStep - net->reportStart; /* initial averaging step when reportstart < reportstep */
    hsteps = 0;
    wrncnt = 0;
    errcode = ENopenH();
    errcode = ENinitH(1);
    printf("Computing network hydraulics ... ");
    do {
        if ( status = ENrunH(&entime) )
        {
            if (status < 100) wrncnt++;
            //else epanetError(status); 
        }
        /* Compute time of, and step to, next reporting interval boundary */
        if (entime <= net->reportStart) rtime = net->reportStart;
        else if (MOD(entime - net->reportStart,net->reportStep) != 0) {
            rtime = net->reportStart + net->reportStep*(1 + (entime - net->reportStart)/net->reportStep);
        } 
        else rtime = entime;
        rstep = rtime - entime;

        //printf("%-7s", TEVAclocktime(atime, entime));
        if (rstep < net->reportStep && hsteps < net->numSteps ) {
            /* Accumulate time averaged node and link data in TSO data structures */
            astep = MIN(tstep,net->reportStep - rstep);
	    //ENL_getHydResults(hsteps,astep,db);
            if ( rstep == 0 ) hsteps++; /* End of current averaging report interval */
        }

        if ( status = ENnextH(&tstep) )
        {
            if (status < 100) wrncnt++;
            //else epanetError(status); 
        }
        printf("\b\b\b\b\b\b\b");
    } while (tstep > 0);
    ENcloseH();
    ENsavehydfile((char*)"hydraulics.hyd");
    //MSXusehydfile("hydraulics.hyd");

    /* Check for hydraulic simulation errors and issue warnings */
    printf("\n");
    if (wrncnt) {
        printf("One or more warnings occurred during the hydraulic\n");
        printf("analysis.  Check the EPANET output file.\n");
    }
    if ( hsteps != net->numSteps ) {
      std::cerr << std::endl;
      std::cerr << "Inconsistent calculated/actual hydraulic time steps" << std::endl;
      std::cerr << std::endl;
      return 1;
    }

    //ERDCHECKCPP(ERD_newHydFile(db));
    //ERDCHECKCPP(ERD_writeHydResults(db));
    return wrncnt;
}


int InversionNetworkSimulator::run_quality_teva(PERD db, FILE *tsifile, PSourceData source, int stopTime, std::vector<std::pair<int,int> >& measurements, float threshold)
{
    int  is, wqsteps, wrncnt, status, scenario;
    int  storageMethod=-1, runensemble=0;
    long entime, tstep, rtime, rstep, astep;
    char atime[10];
    PNetInfo net=db->network;
    PNodeInfo nodes=db->nodes;
    PLinkInfo links=db->links;
    
    ENCHECK(ENopenQ());
    printf("Computing network water quality ...\n");
    scenario=0;
    while ( ENL_setSource(source, net, tsifile,0) ) /* Load the next scenario define in TSI file */
      {
    	PSource sources=source->source;
    	int prevEntime;
	PTEVAIndexData indexData;
	
    	//int stopTime=options.sample_time*60;
	//std::cout << "Sim Duration: " << stopTime << std::endl; 
        tstep = net->reportStep - net->reportStart; /* initial averaging step when reportstart < reportstep */
        wrncnt = 0;
        wqsteps = 0;
        entime=0;

        //printf("Scenario %05d, injection nodes ",scenario);
        //for (is=0; is < source->nsources; is++) printf("%+15s, ",sources[is].sourceNodeID);
        //printf(" time");

	
	ERD_clearQualityData(db);
        ENCHECK( ENinitQ(0) );
        do {
	  prevEntime=entime;
	  if ( status = ENrunQ(&entime) ) {
	    if (status < 100)
	      wrncnt++;
	    else epanetError(status);
	  }
	  //printf("%-7s", TEVAclocktime(atime, entime));
	  
	  /* Compute time of, and step to, next reporting interval boundary */
	  if (entime <= net->reportStart) rtime = net->reportStart;
	  else if (MOD(entime - net->reportStart,net->reportStep) != 0) {
	    rtime = net->reportStart + net->reportStep*(1 + (entime - net->reportStart)/net->reportStep);
	  } 
	  else rtime = entime;
	  rstep = rtime - entime;
	  
	  /* Set source strengths and types - order is important if duplicate source nodes */
	  for (is=0; is < source->nsources; is++) {
	    if ( entime >= sources[is].sourceStart && entime < sources[is].sourceStop ) { /* Source is on */
	      ENCHECK( ENsetnodevalue(sources[is].sourceNodeIndex,EN_SOURCEQUAL,sources[is].sourceStrength) );
	      ENCHECK( ENsetnodevalue(sources[is].sourceNodeIndex,EN_SOURCETYPE,(float)sources[is].sourceType) );
	    } else { /* Source is off */
	      ENCHECK( ENsetnodevalue(sources[is].sourceNodeIndex,EN_SOURCEQUAL,0.0) );
	      ENCHECK( ENsetnodevalue(sources[is].sourceNodeIndex,EN_SOURCETYPE,EN_CONCEN) );
	    }
	  }
	  /* Save results */
	  if ( rstep < net->reportStep && wqsteps < net->numSteps ) {
	    
	    /* Accumulate time averaged node data in TSO data structures */
	    astep = MIN(tstep,net->reportStep - rstep);
	    astep=entime-prevEntime;
	    ENL_getQualResults(wqsteps,astep,db);
	    if ( rstep == 0 ) wqsteps++; /* End of current averaging report interval */
	  }
	  
	  if(tstep > 0) {
	    if ( status = ENstepQ(&tstep) ) {
	      if (status < 100)
		wrncnt++;
	      else epanetError(status);
	    }
	    //printf("\b\b\b\b\b\b\b");
	  }
        } while (entime < stopTime);
	
        /* Check for errors and issue warnings */
        //printf("\n");
        if (wrncnt) {
	  printf("\nOne or more warnings occurred during the water quality\n");
	  printf("analysis, check MS/EPANET output file\n");
        }
	/*        if ( wqsteps != net->numSteps )  {
	  std::cerr << std::endl;
	  std::cerr << "Inconsistent calculated/actual water quality time steps" << std::endl;
	  std::cerr << std::endl;
	  return 1;
	}
	*/

	/*        for (is=0; is < source->nsources; is++) {
	  PEvent newEvent(new Event);
	  newEvent->Name() = sources[is].sourceNodeID;
	  sample_net.events_.push_back(newEvent);
	}
	*/
	
	PQualData qual = net->qualResults;
	int n_matches = 0;
	// analyze the solution , calculate number of matches for each event
        for (is=0; is < source->nsources; is++) {
	  // Calculate Node-Time idx
	  //int node_id = sources[is].sourceNodeID;
	  //int inj_start_time_sec = sources[is].sourceStart;
	  //int inj_timestep = merlionUtils::SecondsToNearestTimestepBoundary(inj_start_time_sec, model_.qual_step_minutes);
	  //int nt_idx = node_ID*model_.n_steps + inj_timestep;
	  //std::cout << "Source Node : " << sources[is].sourceNodeID << std::endl;
	  for(std::vector<std::pair<int,int> >::iterator pos_t=measurements.begin(),l_end=measurements.end();pos_t!=l_end;++pos_t) {
	    int Meas_nt_idx = pos_t->first;
	    int MeasID = Meas_nt_idx/model_.n_steps;
	    int MeasTimeStep = Meas_nt_idx%model_.n_steps;
	    if(qual->nodeC[0][MeasID][MeasTimeStep+1] > threshold) {
	      if (pos_t->second == 1) {
		++n_matches;
	      }
	    }
	    else if (pos_t->second == 0) {
	      ++n_matches;
	    }
	  }
	  
	  
	  matches.push_back(n_matches);
	}
        scenario++;	

        /* Write scenario results to TSO database */
	indexData=newTEVAIndexData(source->nsources,source->source);
	ERD_writeQualResults(db,indexData);
      } /* End of while */
    
    ENcloseQ();
    return wrncnt;
}


bool run_simulations_merlion(InversionNetworkSimulator& net, float threshold)
{
   const merlionUtils::MerlionModelContainer& model(net.Model());
   EventList& Events=net.probableEvents();
   
   float *tox_mass_inj_gpmin(new float[model.N]);
   BlasInitZero(model.N, tox_mass_inj_gpmin);
   int t=net.getSnapTimeStep();
   if(t>model.n_steps)
   {
      std::cerr << std::endl;
      std::cerr << "ERROR: The snap time must be less than " <<model.n_steps*model.qual_step_minutes<<" minutes."<< std::endl;
      std::cerr << std::endl;
      return 1;
   }
   float tao=threshold;
   for(EventList::iterator event_pos=Events.begin(),event_end=Events.end();event_pos!=event_end;event_pos++)
   {
      Event& event=**event_pos;
   
      float mass_injected_g;
      int max_row_index;
      event.SetArray(model, tox_mass_inj_gpmin, mass_injected_g, max_row_index);
      
      // we only need the toxin concentrations at the sample time step 
      // int max_row = model.perm_nt_to_upper[t];
      int min_row = model.N-model.n_nodes*(t+1);
      // Solver the linear system
      usolve ( model.N,
        min_row,
        max_row_index,
        model.G->Values(),
        model.G->iRows(),
        model.G->pCols(),
        tox_mass_inj_gpmin );
   
      // Create a reference variable to make units make more obvious. The linear solver
      // converts the rhs vector (units gpmin) into the solution vector (gpm3). We will
      // use this reference when it make sense for units (e.g below the linear solve).
      float*& tox_conc_gpm3 = tox_mass_inj_gpmin;

       // analyze the solution
      for(int nodeID=0;nodeID<model.n_nodes;nodeID++)
      { //only add the allowed nodes
	int ut_idx = model.perm_nt_to_upper[nodeID*model.n_steps + t];        
	if(tox_conc_gpm3[ut_idx]>tao)
        {
	  event.addContaminatedNodeID(nodeID);
	  //net.event_node_map_[].push_back(nodeID);
	}
      }


      // reset the sparse set of locations where the injections modifed the array,
      // these may have been outside the region of interest in the linear solver,
      // in which case, the BlasResetZero code after this loop would not have
      // zero that region of the array
      event.ClearArray(model,tox_mass_inj_gpmin);
      // not really necessary unless more simulation will be performed
      // reset to zero only the values where chl_conc_gpm3
      // was possibly modified by linear solver, this saves time
      const int num_elements = max_row_index+1;
      BlasResetZero(num_elements, tox_conc_gpm3);

   }
   
   return 0;   
}
