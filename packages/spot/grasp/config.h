// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

/****************************************************************
 *
 * PMConfig: reads and store the configuration parameters for
 *           program execution
 *
 * author: Renato Werneck (rwerneck@princeton.edu)
 * log: June 12, 2002 - file created
 *
 ****************************************************************/

#ifndef config_h
#define config_h

class PMConfig 
{
 public:
	
  int graspit;        //grasp iterations
  double alpha;       //randomization factor in greedy algorithm
  int elite;          //size of the pool of elite solutions
  bool grasp_fi;	    //execute fast interchange instead of local search?
  bool output;        //output file with solution? full-blown output, including assignment of customers to facilities
  bool outputFacilities; // output file with just the facility locations
  bool profile;       //output profile of the local search?
  char inputsol[256];   //input solution (file name)
  char outname[256]; //name of the output file
  char outFacilityName[256]; //name of the output file for just facility locations
  char ch [256];      //constructive heuristic
  bool run_grasp;     //run grasp?
  bool run_greedy;    //run greedy algorithm?
  bool run_pgreedy;   //run proportional greedy algorithm?
  bool run_random;    //run random algorithm?
  bool run_pworst;    //run smart random?
  bool run_rpg;       //run RPG (random plus greedy)?
  bool test_pr;       //test path-relinking?
  bool run_preverse;  //run randomized reverse greedy algorithm?
  bool use_mix;       //use mix of constructive algorithms?
  int tc;             //number of rounds in routine to test constr. heuristics (if positive)
  double rpg;         //factor to use in randomized greedy   
  double run_sample;      //run sample constructive algorithm?
  double sample_fraction; //fraction of the options to be sampled by the algorithm

  bool output_density; //print average density of the extra matrix --- for debug purposes only		
  bool partials;      //print partial results?
  bool postopt;       //execute post-optimization?
  bool tls;           //test local search?
  double tlstime;    //mintime for local search test

  double cf;  //cache factor; used in the local search
  //size of cache (for nearby facilities) will be ceil (cf * m / p)
  //if cf < 0, will use m

  char ls [256];  //local search method
  char cmg [256]; //combination meathod used by GRASP
  char cmp [256]; //combination method used in the post-optimization phase				

  double rpvalue;     //command-line specified of p (as fraction of m)
  int pvalue;         //command-line specified value for p (takes precedence over rpvalue)
  int seed;           //random seed
		
  PMConfig ();
  bool readParameters (int argc, char *argv[]);


  char **argv;
  int argc;
  int arg; //current argument being read

  bool readBool (const char *field, bool &variable, bool value = true) {
    if (strcmp (argv[arg], field)==0) {
      variable = value;
      arg++;
      return true;
    }
    return false;
  }

  bool readInt (const char *field, int &variable) {
    if (strcmp (argv[arg], field)==0) {
      arg++;
      if (arg==argc) {
	fprintf (stderr, "WARNING: Option %s requires an input parameter.\n", field);
      } else {
	variable = atoi (argv[arg]);
	arg++;
      }
      return true;
    }
    return false;
  }				

  bool readChar (const char *field, char &variable) {
    if (strcmp (argv[arg], field)==0) {
      arg++;
      if (arg==argc) {
	fprintf (stderr, "WARNING: Option %s requires a char parameter.\n", field);
      } else {
	variable = argv[arg][0];
	arg++;
      }
      return true;
    }
    return false;
  }

  bool readDouble (const char *field, double &variable) {
    if (strcmp (argv[arg], field)==0) {
      arg++;
      if (arg==argc) {
	fprintf (stderr, "WARNING: Option %s requires a double value.\n", field);
      } else {
	variable = atof (argv[arg]);
	arg++;
      }
      return true;
    }
    return false;
  }

  bool readString (const char *field, char *variable) {
    if (strcmp (argv[arg], field)==0) {
      arg++;
      if (arg==argc) {
	fprintf (stderr, "WARNING: Options %s requires a string value.\n", field);
	return false;
      }
      strcpy (variable, argv[arg]);
      arg++;
      return true;
    }
    return false;
  }
};


/*-----------------------------------------
  | constructor: initializes all parameters
  | to default values
  *----------------------------------------*/

PMConfig::PMConfig(void) 
{
  graspit = 32;
  elite = 10;
  grasp_fi = false;
  output = false;
  outputFacilities = false;
  strcpy(outname, "pmedian.out");
  inputsol[0] = 0; //no input solution
  run_grasp      = true;
  run_greedy     = false;
  output_density = false;
  run_random     = false;
  run_pworst     = false;
  run_pgreedy    = false;
  run_rpg        = false;
  run_preverse   = false;
  test_pr        = false;
  partials       = false;
  postopt        = true;
  use_mix        = false;
  profile        = false;
  tc = 0;
  cf = -1;

  strcpy(ch,"sample:2");
  strcpy(ls,"s");

  pvalue = -1;
  rpvalue = -1.0; //invalid value to start with
  seed = 1;
  rpg = -1;
  alpha = 0.0;
  run_sample = 0;
  sample_fraction = 2;

  tlstime = 5;
  tls = false;

  strcpy (cmg, "r:s"); //strict (new to old, in this case)
  strcpy (cmp, "r:u"); //up (best to worst)
}


/*---------------------------------------
  | read parameters from strings (in a 
  | command line-like format)
  *--------------------------------------*/

bool PMConfig::readParameters(int _argc, char *_argv[]) 
{
  argc = _argc;
  argv = _argv;
  arg = 2;
	
  while (arg < argc) 
    {
    if (strcmp(argv[arg], "-output") == 0) 
      {
      arg++; 
      if (arg==argc) return false;
      output=true;
      strcpy(outname,argv[arg]);
      arg++;
      continue;
    }
    if (strcmp(argv[arg], "-outputFacilities") == 0) 
      {
      arg++; 
      if (arg==argc) return false;
      outputFacilities=true;
      strcpy(outFacilityName,argv[arg]);
      arg++;
      continue;
    }
    if (strcmp(argv[arg], "-sample")==0) 
      {
      arg++; if (arg==argc) return false;
      sample_fraction = atof (argv[arg]);
      run_sample = true;
      arg++;
      continue;
    }
    if (strcmp(argv[arg], "-rpg")==0) 
      {
      arg++; if (arg==argc) return false;
      rpg = atof(argv[arg]);
      run_rpg = true;
      arg++;
      continue;
    }

    if (readString ("-inputsol",  inputsol)) continue;
    if (readString ("-ch",        ch)) continue;
    if (readBool   ("-grasp_fi",  grasp_fi)) continue;
    if (readBool   ("-profile",   profile)) continue;
    if (readInt    ("-seed",      seed)) continue;
    if (readBool   ("-partials",  partials)) continue;
    if (readBool   ("-mix",       use_mix)) continue;
    if (readBool   ("-nopostopt", postopt, false)) continue;
    if (readBool   ("-nograsp",   run_grasp, false)) continue;
    if (readInt    ("-tc",        tc)) continue;
    if (readInt    ("-graspit",   graspit)) continue;
    if (readInt    ("-p",         pvalue)) continue;
    if (readDouble ("-rp",        rpvalue)) continue;
    if (readDouble ("-alpha",     alpha)) continue;    
    if (readDouble ("-cf",        cf)) continue;
    if (readBool   ("-grasp",     run_grasp)) continue;
    if (readBool   ("-density",   output_density)) continue;
    if (readInt    ("-elite",     elite)) continue;
    if (readBool   ("-greedy",    run_greedy)) continue;
    if (readBool   ("-pgreedy",   run_pgreedy)) continue;
    if (readBool   ("-random",    run_random)) continue;
    if (readBool   ("-pworst",    run_pworst)) continue;
    if (readBool   ("-preverse",  run_preverse)) continue;
    if (readBool   ("-test_pr",   test_pr)) continue;
    if (readString ("-ls",        ls)) continue;
    if (readString ("-cmg",       cmg)) continue;
    if (readString ("-cmp",       cmp)) continue;
    if (readBool   ("-tls",       tls)) continue;
    if (readDouble ("-tlstime",   tlstime)) continue;

    fprintf (stderr, "WARNING: Unrecognized option %s. Ignored.\n", argv[arg]);
    arg++;
  }
  return true;
}

#endif
		
