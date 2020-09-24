#include <source_inversion/InversionDataWriter.hpp>
#include <source_inversion/InversionSimOptions.hpp>
#include <source_inversion/InversionNetworkSimulator.hpp>

#include <merlionUtils/TaskTimer.hpp>

#include <merlion/Merlion.hpp>
#include <merlion/BlasWrapper.hpp>
#include <merlion/TriSolve.hpp>
#include <merlion/MerlionDefines.hpp>
#include <merlion/SparseMatrix.hpp>

#include <time.h>
#include <set>
#include <vector>

int main(int argc, char** argv)
{
  time_t start_other_time = time(NULL);  
   // Handle the optional inputs to this executable
   InversionSimOptions options; 
   options.parse_inputs(argc, argv);

   // Helpful interface for setting up the refuce inverse matrix
   InversionNetworkSimulator net(options.disable_warnings);

   const merlionUtils::MerlionModelContainer& model(net.Model());

   if (options.logging) {
      net.StartLogging(options.output_prefix+"inversionsim.log");
      options.print_summary(net.Log());
   }

   if ((options.inp_filename != "") && (options.wqm_filename != "")) {
      std::cerr << std::endl;
      std::cerr << "ERROR: Both a wqm file and an inp file cannot be specified." << std::endl;
      std::cerr << std::endl;
      return 1;
   }
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
   
   //Read the measurements file
   if(options.measurement_filename != "")
   {
      net.ReadMEASUREMENTS(options.measurement_filename);
      //net.FilterMeasurements();
  }
   else {
      std::cerr << std::endl;
      std::cerr << "ERROR: Missing measurement file." << std::endl;
      std::cerr << std::endl;
      return 1;
   }

   //Build the inverse matrix
   std::set<int> rowIDs;
   const std::vector<Measure*>& measures=net.MeasureList();	
   //start time in seconds
   int start_inversion, end_inversion=0;
   if(options.start_time ==-1)
   {
      int last_measure_time_s=-1;
      for(std::vector<Measure*>::const_iterator s_pos=measures.begin(),e_pos=measures.end();s_pos!=e_pos;s_pos++)
      {
         if((*s_pos)->MeasureTimeSeconds()>last_measure_time_s){last_measure_time_s=(*s_pos)->MeasureTimeSeconds();}
      }
      start_inversion=last_measure_time_s;
   }
   else{start_inversion=options.start_time*60;}	
      
   std::cout<<"last_ts:   "<<merlionUtils::SecondsToNearestTimestepBoundary(start_inversion,model.qual_step_minutes)<<"\n";
   
   //end time in seconds
   if((options.inversion_horizon !=-1)/*&&(start_inversion > options.inversion_horizon*60)*/)
     end_inversion=start_inversion-options.inversion_horizon*60;
   else 
     end_inversion=0;
   
    if(end_inversion<0)
      end_inversion=0;

    // Sets the time (in seconds) to take a snap of uncertain nodes
    if(options.snap_time!=-1){
      int snap_timestep = merlionUtils::SecondsToNearestTimestepBoundary(options.snap_time*60,model.qual_step_minutes);
      net.setSnapTimeStep(snap_timestep);
    }
    else{
      net.setSnapTimeStep(-1);
    }
    
   /* if(end_inversion<0){
      std::cerr << std::endl;
      std::cerr << "ERROR: Bad horizon specification" << std::endl;
      std::cerr << std::endl;
      return 1;
    
      }*/

   

   std::cout<<"last_ts:   "<<merlionUtils::SecondsToNearestTimestepBoundary(end_inversion,model.qual_step_minutes)<<"\n";
   bool oneDeleted=false;
   for(std::vector<Measure*>::const_iterator s_pos=measures.begin(),e_pos=measures.end();s_pos!=e_pos;s_pos++)
   {
      if((*s_pos)->MeasureTimeSeconds()>start_inversion || (*s_pos)->MeasureTimeSeconds()<end_inversion)
      {
         oneDeleted=true;
      }
      else
      {
         rowIDs.insert(model.perm_nt_to_upper[model.NodeID((*s_pos)->nodeName())*model.n_steps + (*s_pos)->Measure_timestep(model.qual_step_minutes)]); 
      }
   }
   
   if(oneDeleted)
   {
      std::cerr << std::endl;
      std::cerr << "WARNING: There are measuremets taken that are not being consider in the source inversion." << std::endl;
      std::cerr << "         Consider change the horizon or the inversion start-time options." << std::endl;
      std::cerr << std::endl;
   }
   
   //Method that build the inverse matrix.
   net.GenerateReducedInverse(rowIDs,merlionUtils::SecondsToNearestTimestepBoundary(end_inversion,model.qual_step_minutes),1,false,options.wqm_zero_tol);
   

   // We instantiate the data writer with pointers to the following
   // objects so function calls can be less cluttered with arguments
   // The data writer will be used to write the resulting data files
   // for the optimization problem
    
    InversionDataWriter writer(&net, &options);
   
   if(options.probability)
   	  options.optimization = false;
	
   if(options.optimization)
     {
       options.probability = false; 

	std::cout << "\nNetwork Stats:\n";
	std::cout << "\tNumber of Junctions                  - " << model.junctions.size() << "\n";
	std::cout << "\tNumber of Nonzero Demand Junctions   - " << model.nzd_junctions.size() << "\n";
	std::cout << "\tNumber of Tanks                      - " << model.tanks.size() << "\n";
	std::cout << "\tNumber of Reservoirs                 - " << model.reservoirs.size() << "\n";
	std::cout << "\tWater Quality Timestep (minutes)     - " << model.qual_step_minutes << "\n";
	std::cout << "\tNumber of measurements	             - " << net.MeasureList().size()<< "\n";
	std::cout<<"\tInverse Matrix dimensions            - " << net.reduceMatrixNrows()<<"x"<<net.reduceMatrixNcols()<<"\n";
	std::cout << std::endl;

   //Write Data Files
   end_inversion=merlionUtils::SecondsToNearestTimestepBoundary(end_inversion,model.qual_step_minutes);
   start_inversion=merlionUtils::SecondsToNearestTimestepBoundary(start_inversion,model.qual_step_minutes);
   if(!options.reduceSystem)
   {
      writer.WriteReduceMAtrixGInverse();
      writer.WriteMeasurments(end_inversion,start_inversion);
   }
   else
   {		    
      writer.WriteGMAtrixToAMPL(end_inversion,start_inversion);
      writer.WriteMeasurments(end_inversion,start_inversion);
   }
   
     }

   time_t total_other_time = time(NULL) - start_other_time;
   std::cout<<"Total other time : "<<total_other_time<<"\n";
   // Probability calculations   
   if(options.probability)
   {
       
     float pf = options.meas_failure;
     float conf = options.confidence;
     
     std::vector<std::string> most_probable_nodes;
     if(options.allowedNodes_filename!="")
       {
	 std::cout<<"Using Allowed Nodes Cut ... " ;
	 net.readAllowedNodes(options.allowedNodes_filename);
       }   
     most_probable_nodes=net.EventProbabilities(pf, 
						conf, 
						options.output_prefix, 
						options.meas_threshold, 
						options.wqm_zero_tol, 
						options.merlion,
						options.inp_filename);
     if(options.output_impact_nodeNames)
       writer.WriteImpactNodeNames(most_probable_nodes);

     writer.WriteUncertainNodes();
   }
   //std::cout<<"@@@@@@@@@@@@@@@"<<options.sim_duration_min<<std::endl;  
   net.StopLogging();
   
   //Free all other memory being used
   writer.clear();
   net.clear();

   std::cout << "\nDONE" << std::endl;
   
   return 0;
}

