#include <grab_sample_location/SampleLocationOptions.hpp>
#include <grab_sample_location/SampleNetworkSimulator.hpp>
#include <grab_sample_location/SampleLocationDataWriter.hpp>
#include <grab_sample_location/Events.hpp>

#include <merlionUtils/TaskTimer.hpp>

#include <merlion/Merlion.hpp>
#include <merlion/Network.hpp>
#include <merlion/BlasWrapper.hpp>
#include <merlion/TriSolve.hpp>
#include <merlion/MerlionDefines.hpp>
#include <merlion/SparseMatrix.hpp>
#include <epanet2.h>
#include <epanetmsx.h>
extern "C" {
  #include <enl.h>
  #include <erd.h>
}
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


#define MIN(x,y) (((x)<=(y)) ? (x) : (y))     /* minimum of x and y    */
#define MAX(x,y) (((x)>=(y)) ? (x) : (y))     /* maximum of x and y    */
#define MOD(x,y) ((x)%(y))                    /* x modulus y           */
#define ERDCHECKCPP(x) if ((ERRNUM=x) > 0) {ERD_Error(ERRNUM);}
#define TEVAFILEOPEN(x,f) if ((x) == NULL) { TEVASimError(1,"Error opening file: %s",(f));}


bool run_simulations_merlion(SampleNetworkSimulator& net, const SampleLocationOptions& options);
int run_hydraulics_teva(PERD db, std::string prefix);
int run_quality_teva(PERD db, FILE *tsifile, PSourceData source, SampleNetworkSimulator& sample_net, const SampleLocationOptions& options);
void writeGreedySolution(SampleNetworkSimulator& net, const SampleLocationOptions& options, float obj);
void printErrorJson(std::string error_string, const SampleLocationOptions& options);
void TEVASimError(int exitCode,const char *errmsg, ...);

int main(int argc, char** argv)
{
   SampleLocationOptions options;
   options.parse_inputs(argc,argv);
   
   SampleNetworkSimulator net(options.disable_warnings);

   if (options.logging) {
      net.StartLogging(options.output_prefix+"samplelocation.log");
      options.print_summary(net.Log());
   }
  
   if ((options.inp_filename != "") && (options.wqm_filename != "")) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Both a wqm file and an inp file cannot be specified." << std::endl;
      std::cerr << std::endl;
      return 1;
   }

   if ((options.wqm_filename != "") && (!options.merlion)) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Merlion option must be true when using a WQM file" << std::endl;
      std::cerr << std::endl;
      return 1;
   }

   if ((options.greedy) && (!options.merlion)) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Merlion option must be true when using greedy algorithm." << std::endl;
      std::cerr << std::endl;
      return 1;
   }
   
   // ReadINP file and build merlion model if merlion option is used
   if (options.merlion) {
     if (options.inp_filename != "") {
       // Read an epanet input file
       merlionUtils::TaskTimer Run_Hydraulics;
       net.ReadINPFile(options.inp_filename,
		       options.sim_duration_min,
		       options.qual_step_min,
		       options.epanet_output_filename,
		       options.merlion_save_filename,
		       -1,
		       -1,
		       options.ignore_merlion_warnings,
		       options.decay_k);
       Run_Hydraulics.StopAndPrint(std::cout, "\n\t- Ran hydraulic model with Epanet ");     
     }
     
     else if (options.wqm_filename != "") {
       if (options.decay_k != -1.0f) {
         std::cerr << std::endl;
         std::cerr << "ERROR: A decay coefficient cannot be specified when using the --wqm option." << std::endl;
         std::cerr << std::endl;
         return 1;
       }
       merlionUtils::TaskTimer Read_WQM;
       // read a merlion water quality model file
       net.ReadWQMFile(options.wqm_filename);
       Read_WQM.StopAndPrint(std::cout, "\n\t- Read water quality data ");
     }
     else {
       std::cerr << std::endl;
       std::cerr << "ERROR: Missing network input file." << std::endl;
       std::cerr << std::endl;
       return 1;
     }

     if (options.tsg_filename != "") {
       merlionUtils::TaskTimer Read_tsg;
       // Read in the injection scenarios define by the *.tsg file
       //std::cout<< "before read tsg\n";
       net.ReadTSGFile(options.tsg_filename);
       //std::cout<< "after read tsg\n";
       net.writeEvents();
       if(net.NumberOfEvents()<2)
	 {
	   std::cerr << std::endl;
	   std::cerr << "ERROR: Missing event list. The event file should have at least two contamination events." << std::endl;
	   std::cerr << std::endl;
	   return 1;
	 }
       Read_tsg.StopAndPrint(std::cout, "\n\t- Read scenario data in .tsg file");
     }
     
     
     else if (options.tsi_filename != "") {
       merlionUtils::TaskTimer Read_tsi;
       // Read in the injection scenarios define by the *.tsi file
       net.ReadTSIFile(options.tsi_filename);
       net.writeEvents();
       if(net.NumberOfEvents()<2)
	 {
	   std::cerr << std::endl;
	   std::cerr << "ERROR: Missing event list. The event file should have at least two contamination events." << std::endl;
	   std::cerr << std::endl;
	   return 1;
	 }
       Read_tsi.StopAndPrint(std::cout, "\n\t- Read scenario data in .tsi file");
     }
     else if (options.scn_filename != "") {
       merlionUtils::TaskTimer Read_scn;
       // Read in the injection scenarios define by the *.scn file
       net.ReadSCNFile(options.scn_filename);
       net.writeEvents();
       if(net.NumberOfEvents()<2)
	 {
	   std::cerr << std::endl;
	   std::cerr << "ERROR: Missing event list, It should have at least two events." << std::endl;
	   std::cerr << std::endl;
	   return 1;
	 }
       Read_scn.StopAndPrint(std::cout, "\n\t- Read scenario data in .scn file");
     }
     else {
       std::cerr << std::endl;
       std::cerr << "ERROR: Missing scenario file." << std::endl;
       std::cerr << std::endl;
       return 1;
     }
     int scen_file_cnt = (options.tsg_filename != "")?(1):(0);
     scen_file_cnt += (options.tsi_filename != "")?(1):(0);
     scen_file_cnt += (options.scn_filename != "")?(1):(0);
     if (scen_file_cnt > 1) {
       std::cerr << std::endl;
       std::cerr << "ERROR: Too many scenario options specified. Choose one: TSG, TSI, SCN" << std::endl;
       std::cerr << std::endl;
       return 1;
     }

     if(options.sample_time<0)
       {
	 std::cerr << std::endl;
	 std::cerr << "ERROR: Wrong sampling time." << std::endl;
	 std::cerr << std::endl;
	 return 1;
       }
 
     if (options.fixed_sensors_filename != ""){
       net.readFixedSensors(options.fixed_sensors_filename, options.merlion);
     }
     if(options.allowedNodes_filename!="")
       {
	 std::cout<<"Using Allowed Nodes Cut ... " ;
	 net.readAllowedNodes(options.allowedNodes_filename, options.merlion);
       }   

     if(options.weights){
       net.with_weights = options.weights;
     }
     merlionUtils::TaskTimer run_sims;
     bool return_value = run_simulations_merlion(net,options);
     run_sims.StopAndPrint(std::cout, "\n\t- Ran all event simulations");
     merlionUtils::TaskTimer build_sets;
     net.BuildSets(options.greedy);
     //net.NumberOfUncertainNodes();
     build_sets.StopAndPrint(std::cout, "\n\t- Built Sets");
     if(options.greedy) {
       float objective = net.GreedySelection(options.nSamples);
       std::cout<<"Done with greedy selection\n";
       if(objective == -1.0)  {
	 std::cerr << std::endl;
	 std::cerr << "ERROR: Events provided are indistinguishable." << std::endl;
	 std::cerr << std::endl;
	 printErrorJson("Events are indistinguishable", options);
	 return 1;
       }
       writeGreedySolution(net, options, objective);
     }
     else{
       //merlionUtils::TaskTimer write_files;
       SampleLocationDataWriterMerlion writer(&net, &options); 
       writer.WriteProblem();
       //write_files.StopAndPrint(std::cout, "\n\t- Wrote Files");
     }
     return return_value;     
   } // end if merlion

   /***********************************************/
   /*                                             */
   /*    USING TEVASIM TO PERFORM SIMULATIONS     */
   /*                                             */
   /***********************************************/
   
   else { // Else use tevasim

     if (options.inp_filename == "") {
       std::cerr << std::endl;
       std::cerr << "ERROR: Missing network input file." << std::endl;
       std::cerr << "Using EPANET to run contaminant simulations require EPANET INP file." << std::endl;
       std::cerr << std::endl;
       return 1;
     }
     
     if ((options.tsg_filename != "") && (options.tsi_filename != "")) {
       std::cerr << std::endl;
       std::cerr << "ERROR: Both TSG and TSI file cannot be specified. Choose either one." << std::endl;
       std::cerr << std::endl;
       return 1;
     }
     
     if (options.scn_filename != "") {
       std::cerr << std::endl;
       std::cerr << "ERROR: SCN Files are only supported by Merlion." << std::endl;
       std::cerr << std::endl;
       return 1;
     }
     
     if ((options.tsg_filename == "") && (options.tsi_filename == "")) {
       std::cerr << std::endl;
       std::cerr << "ERROR: Missing scenario file." << std::endl;
       std::cerr << std::endl;
       return 1;
     }
       
     if(options.sample_time<0) {
       std::cerr << std::endl;
       std::cerr << "ERROR: Wrong sampling time." << std::endl;
       std::cerr << std::endl;
       return 1;
     }
     
     
     // Define input variables needed by tevasim run_flushsim
     char* tsgfname = (char*)options.tsg_filename.c_str();
     char* tsifname = (char*)options.tsi_filename.c_str();
     std::string erd_filename = options.output_prefix;
     erd_filename += "samplelocation_db.erd";
     char* erdName = (char*)erd_filename.c_str();
     std::string epanetout_filename = options.output_prefix;
     epanetout_filename += "samplelocation_epanet.out";
     char* epanetoutfname = (char*)epanetout_filename.c_str();
     char* epanetinpfname = (char*)options.inp_filename.c_str();
     char* msxfname = NULL;
     char* msx_species = NULL;
     bool isRLE = options.RLE;

     // original tevasim variables
     int version;
     int storageMethod=-1, runensemble=0;
     time_t cpustart, cpustop;
     double cpuelapsed;
     //	PNetInfo net;
     PERD db;
     //	PNodeInfo nodes;
     //	PLinkInfo links;
     PSourceData source=(PSourceData)calloc(1,sizeof(SourceData));
     int isMSX;
     FILE *tsgfile=NULL, *tsifile=NULL;
     //FILE *decfile=NULL;
     char tTSIFname[32]="";
     
     double **initQual;  // species,node: initial quality from msx input file
     source->source=(PSource)calloc(MAXSOURCES,sizeof(Source));
     source->nsources=0;
     #ifdef WIN32
       startMemoryMonitorThread("memlog.txt",1000);
     #endif

     if (tsgfname) {
       TEVAFILEOPEN(tsgfile = fopen(tsgfname, "r"),tsgfname);
       runensemble=1;
     }
     //std::cout<< "AFTER open tsg file\n";
     else if (tsifname) {
       tsifile = fopen(tsifname, "r+t");
       if (!tsifile)
	 TEVAFILEOPEN(tsifile = fopen(tsifname,"w+t"),tsifname);
     }

     std::string epanet_rpt_filename = options.output_prefix;
     epanet_rpt_filename += "tmp.rpt";

     ENCHECK( (ENopen((char*)epanetinpfname,(char*)epanetoutfname, (char*)epanet_rpt_filename.c_str())) );
     
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
     /*****************************************************/
     /*             Hydraulic Simulation                  */
     /*****************************************************/
     int errNum = run_hydraulics_teva(db, options.output_prefix);

     if(errNum != 0) {	// check error
       printf(":: Problem getting Epanet hydraulic results. \n\n");
       exit(1);
     }
     
     // create EPANET Label Map
     net.createEpanetLabelMap(db);

     // read fixed sensor locations
     if (options.fixed_sensors_filename != ""){
       net.readFixedSensors(options.fixed_sensors_filename, options.merlion);
     }
     
     // read Allowed node locations
     if(options.allowedNodes_filename!="") {
       std::cout<<"Using Allowed Nodes Cut ... " ;
       net.readAllowedNodes(options.allowedNodes_filename, options.merlion);
     }   

     /************************************************************/
     /*             Water Quality Simulations                    */
     /************************************************************/
     int return_val = run_quality_teva(db,tsifile,source, net, options);

     std::cout << "Number of Events : " << net.NumberOfEvents() << std::endl;

     free(source->source); /* I'd rather this be part of ReleaseNetworkData... */
     free(source); /* I'd rather this be part of ReleaseNetworkData... */
     /* close any files that have been opened */

     if(tsgfile) {fclose(tsgfile);}
     
     if(tsifile) {fclose(tsifile);}
     if(strlen(tTSIFname)!=0) {
       std::remove(tTSIFname);
     }

     // Build Sets
     net.BuildSets_teva(db);
     //net.NumberOfUncertainNodes();
     // write data files 
     SampleLocationDataWriterEpanet writer(&net, &options, db); 
     writer.WriteProblem();
     
     // close database
     ERD_close(&db);
     /* release EPANET memory */
     ENclose();
     
     return return_val;

   } // End calculations with tevasim  

   return 1;
}

bool run_simulations_merlion(SampleNetworkSimulator& net, const SampleLocationOptions& options)
{
   const merlionUtils::MerlionModelContainer& model(net.Model());
   EventList& Events=net.Events();

   std::cout << "\nNetwork Stats:\n";
   std::cout << "\tNumber of Junctions                  - " << model.junctions.size() << "\n";
   std::cout << "\tNumber of Nonzero Demand Junctions   - " << model.nzd_junctions.size() << "\n";
   std::cout << "\tNumber of Tanks                      - " << model.tanks.size() << "\n";
   std::cout << "\tNumber of Reservoirs                 - " << model.reservoirs.size() << "\n";
   std::cout << "\tWater Quality Timestep (minutes)     - " << model.qual_step_minutes << "\n";
   std::cout << "\tNumber of events                     - " << Events.size() <<"\n";
   std::cout << std::endl;
   
   float *tox_mass_inj_gpmin(new float[model.N]);
   BlasInitZero(model.N, tox_mass_inj_gpmin);
   int t=options.sample_time/model.qual_step_minutes;
   if(t>model.n_steps)
   {
      std::cerr << std::endl;
      std::cerr << "ERROR: The sample time must be less than " <<model.n_steps*model.qual_step_minutes<<" minutes."<< std::endl;
      std::cerr << std::endl;
      return 1;
   }
   float tao=options.threshold;
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
        if ((options.allowedNodes_filename == "")||(std::count(net.allowed_node_ID.begin(), net.allowed_node_ID.end(), nodeID) != 0)) {  
          int ut_idx = model.perm_nt_to_upper[nodeID*model.n_steps + t];        
          if(tox_conc_gpm3[ut_idx]>tao)
          {
            event.addContaminatedNodeID(nodeID);
	        //net.event_node_map_[].push_back(nodeID);
	  }
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


int run_hydraulics_teva(PERD db, std::string output_prefix)
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
            else epanetError(status); 
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
			ENL_getHydResults(hsteps,astep,db);
            if ( rstep == 0 ) hsteps++; /* End of current averaging report interval */
        }

        if ( status = ENnextH(&tstep) )
        {
            if (status < 100) wrncnt++;
            else epanetError(status); 
        }
        printf("\b\b\b\b\b\b\b");
    } while (tstep > 0);
    ENcloseH();
    std::string hyd_filename = output_prefix;
    hyd_filename += "hydraulics.hyd";

    ENsavehydfile((char*)hyd_filename.c_str());
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

    ERDCHECKCPP(ERD_newHydFile(db));
    ERDCHECKCPP(ERD_writeHydResults(db));
    return wrncnt;
}

int run_quality_teva(PERD db, FILE *tsifile, PSourceData source, SampleNetworkSimulator& sample_net, const SampleLocationOptions& options)
{
    int  is, wqsteps, wrncnt, status, scenario;
    int  storageMethod=-1, runensemble=0;
    long entime, tstep, rtime, rstep, astep;
    char atime[10];
    PNetInfo net=db->network;
    PNodeInfo nodes=db->nodes;
    PLinkInfo links=db->links;
    
    if (net->simDuration < options.sample_time*60) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Sample time must be withing simulation duration." << std::endl;
      std::cerr << std::endl;
      exit(1);
    }
    ENCHECK( ENopenQ() );
    printf("Computing network water quality ...\n");
    scenario=0;
    while ( ENL_setSource(source, net, tsifile,0) ) /* Load the next scenario define in TSI file */
      {
    	PSource sources=source->source;
    	int prevEntime;
	PTEVAIndexData indexData;
	
    	int stopTime=options.sample_time*60;
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
	// analyze the solution , create event with contaminated nodes
        for (is=0; is < source->nsources; is++) {
	  PEvent newEvent(new Event);
	  //newEvent->Name() = sources[is].sourceNodeID;
	  std::stringstream ss;
	  ss << scenario;
	  newEvent->Name() = ss.str();
	  //std::cout << "Source Node : " << sources[is].sourceNodeID << std::endl;
	  for(int nodeID=0; nodeID < net->numNodes; nodeID++)
	    { //only add the allowed nodes
	      if ((options.allowedNodes_filename == "")||(std::count(sample_net.allowed_node_ID.begin(), sample_net.allowed_node_ID.end(), nodeID) != 0)) {  
		//std::cout << "\t Node quality : " << qual->nodeC[0][nodeID][wqsteps] << std::endl;
		if(qual->nodeC[0][nodeID][wqsteps-1] > options.threshold)
		  {
		    //std::cout << "\t \t Added node : " <<nodeID << std::endl;
		    newEvent->addContaminatedNodeID(nodeID);
		    //net.event_node_map_[].push_back(nodeID);
		  }
	      }
	    }
	  sample_net.events_.push_back(newEvent);
	}
        scenario++;	

        /* Write scenario results to TSO database */
	indexData=newTEVAIndexData(source->nsources,source->source);
	ERD_writeQualResults(db,indexData);
      } /* End of while */
    
    ENcloseQ();
    return wrncnt;
}

void writeGreedySolution(SampleNetworkSimulator& net, const SampleLocationOptions& options, float obj) {

  int num_samples = net.greedy_selected_nodes.size();
  std::vector<std::string> selected_nodes;
  std::copy(net.greedy_selected_nodes.begin(),net.greedy_selected_nodes.end(),std::back_inserter(selected_nodes));
  std::map<std::string, std::vector<std::string> > nodes_to_outlets;
  // Print json file containing outlet pipes of all grabsample nodes
      // But we first need to get all the outlet links for these nodes in a std map


  const merlionUtils::MerlionModelContainer& model(net.Model());
  const float* link_velocities = const_cast<float*>(model.merlionModel->GetVelocities_mpmin());
  int sample_timestep=options.sample_time/model.qual_step_minutes;

  
  for (int i = 0; i < num_samples; ++i)
  { 
    int NodeID = model.NodeID(selected_nodes[i]);
    std::vector<int> outlet_links = model.NodeOutlets(NodeID);
    for (std::vector<int>::iterator link_i = outlet_links.begin(); link_i != outlet_links.end(); link_i++) {
      int link_id = *link_i;
      if(link_id != -1) {
        std::string link_name = model.LinkName(link_id);
        int flowidx = link_id*model.n_steps+sample_timestep;
        float flow = link_velocities[flowidx];
        if (flow > 0) {
         // flow is going out the outlet node - therefore, add to outlet_link_map
          nodes_to_outlets[selected_nodes[i]].push_back(link_name);
        }
      }
    }
    std::vector<int> inlet_links = model.NodeInlets(NodeID);
    for (std::vector<int>::iterator link_i = inlet_links.begin(); link_i != inlet_links.end(); link_i++) {
      int link_id = *link_i;
      if(link_id != -1) {
        std::string link_name = model.LinkName(link_id);
        int flowidx = link_id*model.n_steps+sample_timestep;
        float flow = link_velocities[flowidx];
        if (flow < 0) {
         // flow is going out the inlet node - therefore, add to outlet_link_map
         nodes_to_outlets[selected_nodes[i]].push_back(link_name);
        }
      }

    }
  
  }

  // write the prefix_grabsample_links.json file 
  // print greedy results in a json file 
  std::ofstream out_json;

  std::string results_filename = options.output_prefix;
  results_filename += "grabsample.json";
  out_json.open(results_filename.c_str(), std::ios_base::out);
  out_json<< "{\"sampleCount\": " << options.nSamples << ", \"threshold\": " << options.threshold << ", \"objective\": " << obj <<", \"sampleTime\": " << options.sample_time << ", \"Nodes\": [" ; 

  for(int i=0; i<num_samples-1; i++) {
    std::string node_name = selected_nodes[i];
    int outlet_vector_size = nodes_to_outlets[node_name].size();
    out_json<< "{\"id\":\"" << node_name << "\", \"rank\":" << i+1 << ", \"downstreamPipeIds\": [";
    for (int j=0; j < outlet_vector_size-1; ++j) {
      out_json<< "\"" << nodes_to_outlets[node_name][j] << "\",";
    }
    if(outlet_vector_size !=0) {
      out_json<< "\"" << nodes_to_outlets[node_name][outlet_vector_size-1] << "\"]},";
    }
    else { 
     out_json<< "]},"; 
    }
  }
  std::string node_name = selected_nodes[num_samples-1];
  int outlet_vector_size = nodes_to_outlets[node_name].size();
    out_json<< "{\"id\":\"" << node_name << "\", \"rank\":" << num_samples << ", \"downstreamPipeIds\": [";
    for (int j=0; j < outlet_vector_size-1; ++j) {
      out_json<< "\"" << nodes_to_outlets[node_name][j] << "\",";
    }
    if(outlet_vector_size !=0) {
      out_json<< "\"" << nodes_to_outlets[node_name][outlet_vector_size-1] << "\"]}";
    }
    else { 
     out_json<< "]}"; 
    }
  out_json<< "]}";

  out_json.close();
  std::cout<<"DONE\n";

}

void printErrorJson(std::string errorString, const SampleLocationOptions& options) {
  std::ofstream out_json;

  std::string results_filename = options.output_prefix;
  results_filename += "grabsample.json";
  out_json.open(results_filename.c_str(), std::ios_base::out);
  out_json<< "{\"Error\":\"" << errorString <<"\", \"sampleCount\": " << options.nSamples << ", \"threshold\": " << options.threshold << ", \"objective\": " << 0 <<", \"sampleTime\": " << options.sample_time << ", \"Nodes\": []}" ; 
  out_json.close();
}

void TEVASimError(int exitCode,const char *errmsg, ...)
{
        va_list ap;
 
        va_start(ap,errmsg);
        vfprintf(stderr,errmsg,ap);
        va_end(ap);
 
        exit(exitCode);
}

