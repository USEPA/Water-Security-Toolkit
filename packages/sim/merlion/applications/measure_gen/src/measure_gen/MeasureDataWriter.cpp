#include <measure_gen/MeasureDataWriter.hpp>
#include <merlionUtils/ModelWriter.hpp>

MeasureGenDataWriter::MeasureGenDataWriter(MeasureGenNetworkSimulator *Net, MeasureGenOptions *Opts)
:
   net_(Net),
   model_(&(Net->Model())),
   options_(Opts)
{
   // set the output precision
   int precision_number = 8;
   out_WQM.setf(std::ios::scientific,std::ios::floatfield);
   out_WQM.precision(precision_number);
   out_CONC.setf(std::ios::scientific,std::ios::floatfield);
   out_CONC.precision(precision_number);

   // define the data file names
   out_WQM_fname = options_->output_prefix + "WQM.dat";
   out_NMAP_fname = options_->output_prefix + "MERLION_LABEL_MAP.txt";
   out_CONC_fname = options_->output_prefix + "CONC.dat";


   // create the modifed node map taking
   // merlion id -> epanet name -> custom label
   std::map<std::string,std::string> name_map;
   std::map<std::string,std::string> label_map;
   if (options_->nodemap_filename != "") {
      // use custom labels in data files
      std::ifstream in;
      in.open(options_->nodemap_filename.c_str(), std::ios_base::in);
      if (in.is_open()) {
         std::string in_name, in_label;
         std::vector<std::string>::iterator findpos;
         for (int i = 0, stop = model_->n_nodes; i < stop; ++i) {
            in >> in_name;
            int merlion_node_id = model_->NodeID(in_name);
            in >> in_label;
            if (label_map.find(in_label) != label_map.end()) {
               std::cerr << "Duplicate labels found in node map file: " << in_label << std::endl; 
            } 
            else {
               label_map[in_label] = in_name;
               name_map[in_name] = in_label;
            }
         }
      }
      else {
         std::cerr << "Failed to open custom node map file: " << options_->nodemap_filename << std::endl;
         exit(1); 
      }
      in.close();
   }
   else if (options_->output_merlion_labels) {
      // use merlion integer ids in data files
      typedef std::map<std::string, int> map_string_to_int;
      for (int n = 0; n < model_->n_nodes; ++n) {
         std::ostringstream stream;
         stream << n;
         name_map[model_->NodeName(n)] = stream.str();
      }
   }
   else {
      // use epanet names in data files
      typedef std::map<std::string, int> map_string_to_int;
      for (int n = 0; n < model_->n_nodes; ++n) {
         name_map[model_->NodeName(n)] = model_->NodeName(n);
      }
   }
   for (int n = 0; n < model_->n_nodes; ++n) {
      node_map_[n] = name_map[model_->NodeName(n)]; 
   }

   //write the map from node names to 
   //merlion labels if this option is requested
   if (options_->output_merlion_labels) {
      out_NMAP.open(out_NMAP_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
      for (int n = 0; n < model_->n_nodes; ++n) {
         out_NMAP << model_->NodeName(n) << " " << n << "\n"; 
      }
      out_NMAP.close();
   }  
}

void MeasureGenDataWriter::clear()
{
   net_ = NULL;
   model_ = NULL;
   options_ = NULL;
   node_map_.clear();
}

MeasureGenDataWriter::~MeasureGenDataWriter()
{
   clear(); 
}

void MeasureGenDataWriter::WriteProblem(float *tox_conc_gpm3) 
{
   out_WQM.open(out_WQM_fname.c_str(),std::ios_base::out | std::ios_base::trunc);
   merlionUtils::PrintWQMHeadersToAMPLFormat(out_WQM, model_->merlionModel, node_map_,0,model_->n_steps-1);
   merlionUtils::PrintWQMToAMPLFormat(out_WQM, model_->merlionModel, node_map_,0,model_->n_steps-1);
   out_WQM.close();

   const std::vector<int>& sensors = net_->SensorNodeIDS();
   out_CONC.open(out_CONC_fname.c_str(), std::ios_base::out | std::ios_base::trunc);
   out_CONC << "set S_SENSORS :=";
   for (int i = 0, i_stop = sensors.size(); i < i_stop; ++i) {
      out_CONC << " " << node_map_[sensors[i]];
   }
   out_CONC << ";\n\n";
   out_CONC << "param P_SENSOR_CONC_gpm3 := \n";
   for (int i = 0, i_stop = sensors.size(); i < i_stop; ++i) {
      int n = sensors[i];
      for (int t = 0, n_steps = model_->n_steps; t < n_steps; ++t) {
         int ut_idx = model_->perm_nt_to_upper[n*n_steps + t];
         if (tox_conc_gpm3[ut_idx] > merlionUtils::ZERO_CONC_GPM3) {
            out_CONC << node_map_[n] << " " << t << " " << tox_conc_gpm3[ut_idx] << "\n";
         }
      }
   }
   out_CONC << ";\n";
   out_CONC.close();
}

void MeasureGenDataWriter::WriteBinaryMeasurements(float *tox_conc_gpm3)
{
   int initialTime, stopTime;
   int numMeasuresHour;
   if(options_->start_time_sensing != -1) {
      initialTime = int(options_->start_time_sensing / model_->qual_step_minutes);
      if (initialTime < 0) {
         std::cerr << std::endl;
         std::cerr << "ERROR: Start-sensing-time must be greater then zero" << std::endl;
         std::cerr << std::endl;
         exit(0);
      }
   }
   else {
      initialTime = 0;
   }
   if (options_->measurements_per_hour != -1) {
      numMeasuresHour = options_->measurements_per_hour;
   }
   else {
      numMeasuresHour = 60;
   }

   bool first_detect = false;
   if (options_->stop_time_sensing != "") {
     if (options_->stop_time_sensing == "first_detect") {
       first_detect = true;
       stopTime = model_->n_steps;
       std::cout << "Priniting measurements until first detection time." << std::endl;
     }
     else {
       int stop_sensing_time_;
       stop_sensing_time_ = atoi(options_->stop_time_sensing.c_str()) + int(60/numMeasuresHour);
       // Need better error catch here 
       if (stop_sensing_time_ <=0) {std::cout << "ERROR: Converting stop sensing time to int: " << options_->stop_time_sensing;}

       stopTime = int(stop_sensing_time_ / model_->qual_step_minutes);
       if (stopTime > model_->n_steps){
         std::cerr << std::endl;
         std::cerr << "ERROR: Stop-sensing-time must be less than " << model_->n_steps * model_->qual_step_minutes << " minutes" << std::endl;
         std::cerr << std::endl;
         exit(0);
       }
     }
   }
   else {stopTime = model_->n_steps;}

   // Set the positive and negative node ids vector
   set_sensor_grabsample_ids(tox_conc_gpm3, initialTime, stopTime, numMeasuresHour);
   
   if ((options_->FNR != 0.0f)||(options_->FPR != 0.0f)) {
      addMeasurementError();
   }
   // Start Printing the json and dat files
   std::string datFileName, jsonFileName;
   std::ofstream out, out2;
   if (options_->output_prefix != ""){
      datFileName = options_->output_prefix + "_MEASURES.dat";
      jsonFileName = options_->output_prefix + "_MEASURES.json";
   }
   else{
     datFileName = "MEASURES.dat";
     jsonFileName = "MEASURES.json";
   }

   std::string datFile(datFileName);
   std::string jsonFile(jsonFileName);
   out.open(datFile.c_str(), std::ios::out | std::ios::trunc);
   out2.open(jsonFile.c_str(), std::ios::out | std::ios::trunc);
   //
   int    nQualStepSeconds = model_->qual_step_minutes * 60;
   double dTimeStep = 3600 / numMeasuresHour;
   int    nDuration = (model_->n_steps - 1) * nQualStepSeconds;
   int    nTimeFirst = initialTime * nQualStepSeconds;
   double dTimeLast = nTimeFirst;
   int    nTimeSteps = 1;
   while (dTimeLast + dTimeStep < stopTime * nQualStepSeconds) {
      dTimeLast = nTimeFirst + nTimeSteps * dTimeStep;
      nTimeSteps++;
   }
   //
   out << "# Node_name\t time \t Cij\n";
   out2 << "{";
   out2 << "\"TimeFirst\":"             << nTimeFirst       << ",";
   out2 << "\"TimeLast\":"              << dTimeLast        << ",";
   out2 << "\"TimeStep\":"              << dTimeStep        << ",";
   out2 << "\"TimeSteps\":"             << nTimeSteps       << ",";
   out2 << "\"WaterQualityTimeStep\":"  << nQualStepSeconds << ",";
   out2 << "\"WaterQualityTimeSteps\":" << model_->n_steps  << ",";
   out2 << "\"WaterQualityDuration\":"  << nDuration        << ",";
   out2 << "\"TimeUnits\":\"seconds\",";
   out2 << "\"Concentrations\":{";
   bool bFirstNode = true;

   for (std::vector<std::pair<int,int> >::const_iterator pos_s = net_->GrabSampleIDS().begin(), s_end = net_->GrabSampleIDS().end(); pos_s != s_end; ++pos_s) {
      int s = pos_s->first;
      int t = pos_s->second;
      int ut_idx = model_->perm_nt_to_upper[s*model_->n_steps + t];
      std::vector<int>::iterator p_it = std::find(positive_utidx.begin(),positive_utidx.end(),ut_idx);
      std::vector<int>::iterator n_it = std::find(negative_utidx.begin(),negative_utidx.end(),ut_idx);
      if (p_it!=positive_utidx.end()) {
         out << node_map_[s] << " " << t * model_->qual_step_minutes * 60 << " " << 1 << "\n";
         out2 << 1;
      }
      else if (n_it!=negative_utidx.end()) {
         out << node_map_[s] << " " << t * model_->qual_step_minutes * 60 << " " << 0 << "\n";
         out2 << 0;
      }
   }

   out << "\n\n";

   for (std::vector<int>::const_iterator pos_s = net_->SensorNodeIDS().begin(), s_end = net_->SensorNodeIDS().end(); pos_s != s_end; ++pos_s) {
      int s = *pos_s;
      if (bFirstNode) bFirstNode = false;
      else out2 << "],";
      bool bFirstPoint = true;
      out2 << "\""<< node_map_[s] << "\":[";

      if (first_detect) {
	bool detected = false;
	for (int t = initialTime; t <= stopTime; t++) {
	  if (int(t * model_->qual_step_minutes) % (int(60 / numMeasuresHour)) == 0) {
            int ut_idx = model_->perm_nt_to_upper[s * model_->n_steps + t];
            std::vector<int>::iterator p_it = std::find(positive_utidx.begin(),positive_utidx.end(),ut_idx);
            std::vector<int>::iterator n_it = std::find(negative_utidx.begin(),negative_utidx.end(),ut_idx);
            if (p_it!=positive_utidx.end()) {
	      detected = true;
	      if (stopTime > t){ 
		stopTime = t;
	      }
            }
	  }
	  if (detected){break;}
	}
      }
   }

   for (std::vector<int>::const_iterator pos_s = net_->SensorNodeIDS().begin(), s_end = net_->SensorNodeIDS().end(); pos_s != s_end; ++pos_s) {
      int s = *pos_s;
      if (bFirstNode) bFirstNode = false;
      else out2 << "],";
      bool bFirstPoint = true;
      out2 << "\""<< node_map_[s] << "\":[";
      
      
      for (int t = initialTime; t <= stopTime; t++) {
	if (int(t * model_->qual_step_minutes) % (int(60 / numMeasuresHour)) == 0) {
	  if (bFirstPoint) bFirstPoint = false;
	  else out2 << ",";
	  int ut_idx = model_->perm_nt_to_upper[s * model_->n_steps + t];
	  std::vector<int>::iterator p_it = std::find(positive_utidx.begin(),positive_utidx.end(),ut_idx);
	  std::vector<int>::iterator n_it = std::find(negative_utidx.begin(),negative_utidx.end(),ut_idx);
	  if (p_it!=positive_utidx.end()) {
	    out << node_map_[s] << " " << t * model_->qual_step_minutes * 60 << " " << 1 << "\n";
	    out2 << 1;
	  }
	  else if (n_it!=negative_utidx.end()) {
	    out << node_map_[s] << " " << t * model_->qual_step_minutes * 60 << " " << 0 << "\n";
	    out2 << 0;
	  }
	}
      }
      out<<"\n\n";
   }
   out2 << "]}}";
   //
   out << "\n\n";
   
}


void MeasureGenDataWriter::WriteConcentrations(float *tox_conc_gpm3)
{
   int initialTime, stopTime;
   int numMeasuresHour;
   if(options_->start_time_sensing != -1) {
      initialTime = int(options_->start_time_sensing / model_->qual_step_minutes);
      if (initialTime < 0) {
         std::cerr << std::endl;
         std::cerr << "ERROR: Start-sensing-time must be greater then zero" << std::endl;
         std::cerr << std::endl;
         exit(0);
      }
   }
   else {
      initialTime = 0;
   }
   if (options_->measurements_per_hour != -1) {
      numMeasuresHour = options_->measurements_per_hour;
   }
   else {
      numMeasuresHour = 60;
   }
   if (options_->stop_time_sensing != "") {
     int stop_sensing_time_;
       try{
	 stop_sensing_time_ = atoi(options_->stop_time_sensing.c_str()) + int(60/numMeasuresHour);
       }
       catch (int err) {std::cout << "ERROR: Converting stop sensing time to int. first_detect not supported for concentrations. : " << options_->stop_time_sensing;}

     stopTime = int(stop_sensing_time_ / model_->qual_step_minutes);
      if (stopTime > model_->n_steps){
         std::cerr << std::endl;
         std::cerr << "ERROR: Stop-sensing-time must be less than " << model_->n_steps * model_->qual_step_minutes << " minutes" << std::endl;
         std::cerr << std::endl;
         exit(0);
      }
   } else {
      stopTime = model_->n_steps;
   }
   // Start Printing the json and dat files
   std::string datFileName, jsonFileName;
   std::ofstream out, out2;
   if (options_->output_prefix != ""){
      datFileName = options_->output_prefix + "_MEASURES.dat";
      jsonFileName = options_->output_prefix + "_MEASURES.json";
   }
   else{
     datFileName = "MEASURES.dat";
     jsonFileName = "MEASURES.json";
   }

   std::string datFile(datFileName);
   std::string jsonFile(jsonFileName);
   out.open(datFile.c_str(), std::ios::out | std::ios::trunc);
   out2.open(jsonFile.c_str(), std::ios::out | std::ios::trunc);
   out << "# Node_name\t time \t Cij\n";
   out2 << "{";
   out2 << "\"TimeFirst\":" << initialTime * model_->qual_step_minutes * 60 << ",";
   out2 << "\"TimeLast\":" << stopTime * model_->qual_step_minutes * 60 - (3600 / numMeasuresHour)<< ",";
   out2 << "\"TimeStep\":" << 3600 / numMeasuresHour << ",";
   out2 << "\"TimeSteps\":" << (stopTime - initialTime) * model_->qual_step_minutes * 60 / (3600 / numMeasuresHour) << ",";
   out2 << "\"WaterQualityTimeStep\":" << model_->qual_step_minutes * 60 << ",";
   out2 << "\"WaterQualityTimeSteps\":" << model_->n_steps << ",";
   out2 << "\"WaterQualityDuration\":" << model_->n_steps * model_->qual_step_minutes * 60 << ",";
   out2 << "\"TimeUnits\":\"seconds\",";
   out2 << "\"Concentrations\":{";
   bool bFirstNode = true;

   for (std::vector<std::pair<int,int> >::const_iterator pos_s = net_->GrabSampleIDS().begin(), s_end = net_->GrabSampleIDS().end(); pos_s != s_end; ++pos_s) {
      int s = pos_s->first;
      int t = pos_s->second;
      bool found = false;
      for (int i = 0; i < net_->SensorNodeIDS().size() && !found; i++) {
         if (s == net_->SensorNodeIDS()[i]) found = true;
      }
      if (!found) {
         int ut_idx = model_->perm_nt_to_upper[s*model_->n_steps + t];
         out << node_map_[s] << " " << t * model_->qual_step_minutes * 60 << " " << tox_conc_gpm3[ut_idx] << "\n";
         out2 << tox_conc_gpm3[ut_idx];
      }      

   }

   out << "\n\n";

   for (std::vector<int>::const_iterator pos_s = net_->SensorNodeIDS().begin(), s_end = net_->SensorNodeIDS().end(); pos_s != s_end; ++pos_s) {
      int s = *pos_s;
      if (bFirstNode) bFirstNode = false;
      else out2 << "],";
      bool bFirstPoint = true;
      out2 << "\""<< node_map_[s] << "\":[";
      for (int t = initialTime; t <= stopTime; t++) {
         if (int(t * model_->qual_step_minutes) % (int(60 / numMeasuresHour)) == 0) {
            if (bFirstPoint) bFirstPoint = false;
            else out2 << ",";
            int ut_idx = model_->perm_nt_to_upper[s * model_->n_steps + t];
               out << node_map_[s] << " " << t * model_->qual_step_minutes * 60 << " " << tox_conc_gpm3[ut_idx] << "\n";
               out2 << tox_conc_gpm3[ut_idx];
         }
      }
      out<<"\n\n";
   }
   out2 << "]}}";
   //
   out << "\n\n";

}


void MeasureGenDataWriter::set_sensor_grabsample_ids(float *tox_conc_gpm3, int initialTime, int stopTime, int numMeasuresHour) {
   // Get the ut_idx of positive grabsample measurements
   for (std::vector<std::pair<int,int> >::const_iterator pos_s = net_->GrabSampleIDS().begin(), s_end = net_->GrabSampleIDS().end(); pos_s != s_end; ++pos_s) {
      int s = pos_s->first;
      int t = pos_s->second;
      bool found = false;
      for (int i = 0; i < net_->SensorNodeIDS().size() && !found; i++) {
         if (s == net_->SensorNodeIDS()[i]) found = true;
      }
      if (!found) {
         int ut_idx = model_->perm_nt_to_upper[s*model_->n_steps + t];
         if (tox_conc_gpm3[ut_idx] > options_->threshold) { 
            positive_utidx.push_back(ut_idx);
         } 
         else { 
           negative_utidx.push_back(ut_idx);
         }
      }
   }
   // Get the ut_idx of positive sensor measurements
   for (std::vector<int>::const_iterator pos_s = net_->SensorNodeIDS().begin(), s_end = net_->SensorNodeIDS().end(); pos_s != s_end; ++pos_s) {
      int s = *pos_s;
      for (int t = initialTime; t < stopTime; t++) {
         if (int(t * model_->qual_step_minutes) % (int(60 / numMeasuresHour)) == 0) {
            int ut_idx = model_->perm_nt_to_upper[s * model_->n_steps + t];
               if (tox_conc_gpm3[ut_idx] > options_->threshold) {
                  positive_utidx.push_back(ut_idx);
               } else {
                  negative_utidx.push_back(ut_idx);
               }
         }
      }
   }
}

void MeasureGenDataWriter::addMeasurementError() {

int num_pos = positive_utidx.size();
int num_neg = negative_utidx.size();
std::vector<int> append_to_positive;
std::vector<int> append_to_negative;

// Round down the number of positives to switch
int num_pos_switch = (int)(num_pos*options_->FPR); // this gives a float that is rounded down by conversing to int
int num_neg_switch = (int)(num_neg*options_->FNR); // same as above

// randomly select number of positives and negative to switch 
   if (options_->seed != -1.0f){
      srand(options_->seed); 
   }
   else {
      srand(time(NULL)); 
   }

   for (int i = 0; i < num_pos_switch; ++i)
   {
      //get a random index to remove
      int remove_position = rand()%positive_utidx.size();
      int ut_idx = positive_utidx[remove_position];
      positive_utidx.erase(positive_utidx.begin() + remove_position); //remove from positive ids list
      append_to_negative.push_back(ut_idx); //add to negative ids list
   }

   for (int i = 0; i < num_neg_switch; ++i)
   {
      // get a random index to remove
      int remove_position = rand()%negative_utidx.size();
      int ut_idx = negative_utidx[remove_position];
      negative_utidx.erase(negative_utidx.begin() + remove_position);
      append_to_positive.push_back(ut_idx);
   }
   //append positive with ids removed from negative and vice-versa
   positive_utidx.insert(positive_utidx.end(), append_to_positive.begin(), append_to_positive.end());
   negative_utidx.insert(negative_utidx.end(), append_to_negative.begin(), append_to_negative.end());

}

void MeasureGenDataWriter::printDemands(const merlionUtils::MerlionModelContainer& model_not_noise)
{
   std::ofstream out_test;
   //out_test.setf(std::ios::scientific,std::ios::floatfield);
   out_test.precision(4);
   out_test.open("demands_changes.dat",std::ios::out|std::ios::trunc);
   out_test <<"# timestep\t demand \t demand_noise \t Change_%";
   out_test << "\n";
   //double suma=0;
   for(int i=0;i<model_->n_nodes;i++){
      int nodeid = i;
      float dem_noise = model_->demand_m3pmin[nodeid*model_->n_steps];
      float dem_not_noise = model_not_noise.demand_m3pmin[nodeid*model_->n_steps];
      float percentage = 0;
      if(dem_not_noise!=0){
         percentage = (dem_noise-dem_not_noise)/dem_not_noise;
      }
      out_test <<"node_" <<node_map_[i]<<"\t\t"<<dem_not_noise<<"\t\t"<<dem_noise<<"\t\t"<< percentage*100<<"\n";
      //suma+=abs(percentage);
   }
   
   out_test << "\n";
   //out_test << "average: " << suma*100/model_->n_nodes;
   out_test.close();
}
