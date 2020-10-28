/**************************************
Initial code by Peter.Drexler@exp2.physik.uni-giessen.de and Markus.Moritz@exp2.physik.uni-giessen.de
Written by Lukas.Nies@physik.uni-giessen.de
modified for single board / self triggered readout 
**************************************/

// aw_lukas specific header

#include "aw_lukas.h"

using namespace RooFit;
using namespace std;
using namespace chrono;
using namespace RooFit;

  
int main(int argc, char *argv[])
{
  // Timer for program execution time measurement
  high_resolution_clock::time_point t_total_begin = high_resolution_clock::now();
  ///////////////////////////////////////////////////////////////
  // Read all existing command line argunments and file names
  ///////////////////////////////////////////////////////////////
  char inputfile[200];
  char outputfile[200];
  char outputfile_proto_trace[200];
  char outputfile_energy_calib[200];
  char configfile[200];
  unsigned int no_of_events=0, do_no_of_events=0;
  int realign_first_NOE=0;
  // Parse the input file
  if(argc<=1){ 
    printf("No datafile set!!!\n");
    print_usage();
    return(-1);
  }
  strcpy(inputfile, argv[1]);
  // Parse the output file
  if(argc<=2){ 
    printf("No outputfile declared.\n");
    sprintf(outputfile,"%s.root",argv[1]);
    printf("outputfile set to: %s\n",outputfile);
  }
  else strcpy(outputfile, argv[2]);
  // Parse the config file
  bool conf_exists = false;
  if(argc<=3){ 
    printf("No config file set, using standard or command line parameters.\n");
  }
  else {
    conf_exists = true;
    strcpy(configfile, argv[3]);    
    printf("Config file read in: %s\n",configfile);
  }
  // Now parse all command line options
  for(int n=0; n<argc; n++){
    if(strstr(argv[n],"-n")!=NULL){  // set stop after # of counts
      n++;  
      printf("Max count enabled!\n");
      if(n<argc){
        do_no_of_events=1;
        no_of_events=atoi(argv[n]);
      }
      else{
        printf("Missing max. number of events!\n");
        return(-1);
      }
    }
    // Reading of Digital threshold
    if(strstr(argv[n],"-t")!=NULL){  
      n++;
      if(n<argc){
        THRESHOLD_MULTIPLICY=atoi(argv[n]);
        printf("Software threshold multiplicy set to: %3.2f !\n", THRESHOLD_MULTIPLICY);
      }  
      else{
        printf("Missing treshold multiplicy!\n");
        return(-1);
      }
    }
    // Reading of glitch filter settings
    if(strstr(argv[n],"-g")!=NULL){  // set stop after # of counts
      n++;
      if(n<argc){
        GLITCH_FILTER_RANGE=atoi(argv[n]);
        GLITCH_FILTER = true;
        printf("Glitch filter range set to: %i !\n", GLITCH_FILTER_RANGE);
      }  
      else{
        printf("Missing glitch filter parameter!\n");
        return(-1);
      }
    }
    if(strstr(argv[n],"-L")!=NULL){  // set stop after # of counts
      n++;
      if(n<argc){
        L=atoi(argv[n]);
        printf("Moving average range set to: %i !\n", L);
      }  
      // Check if L is odd
      if (L%2 == 0){
        printf("SETTINGS ERROR: MA window must be odd!\n");
        return(-1);
      }

    }
    if(strstr(argv[n],"-d")!=NULL){  // set stop after # of counts
      n++;
      if(n<argc){
        DELAY=atoi(argv[n]);
        printf("DELAY for CFD set to: %i !\n", DELAY);
      }  
    }
    if(strstr(argv[n],"-f")!=NULL){  // set stop after # of counts
      n++;
      if(n<argc){
        CFD_fraction=atof(argv[n]);
        if ( CFD_fraction < 1. && CFD_fraction > 0. ){
          printf("Fraction for CFD set to: %f !\n", CFD_fraction);
        }
        else{
          printf("SETTINGS ERROR: CFD fraction must be set between 0 and 1!\n");
          return(-1);
        }
      }  
    }
    if(strstr(argv[n],"-M")!=NULL){  // set stop after # of counts
      n++;
      if(n<argc){
        M=atoi(argv[n]);
        printf("Window length for MWD set to: %i !\n", M);
      }  
      // Check if M is odd
      if (M%2 == 0){
        printf("SETTINGS ERROR: MWD window must be odd!\n");
        return(-1);
      }
    }
    if(strstr(argv[n],"-v")!=NULL){  // turn verbose output on
      n++;
      if(n<argc){
        strcpy(VERBOSE, argv[n]);
        printf("Verbose options set to: %s !\n", VERBOSE);
      }  
    }
    if(strstr(argv[n],"-I")!=NULL){  // set the number of inter samples
      // Check if MULTIS is 1, 2, or 4
      n++;
      if (atoi(argv[n]) == 1 ||  atoi(argv[n]) == 2 || atoi(argv[n]) == 4){
        if(n<argc){
          MULTIS=atoi(argv[n]);
          printf("Window length for MWD set to: %i !\n", M);
        }  
      }
      else{
        printf("SETTINGS ERROR: Intersampling number not 1,2, or 4!\n");
        return(-1);
      }
    }
    if(strstr(argv[n],"-r")!=NULL){  // realign_first_NOE
      realign_first_NOE=1;
    }
    if(strstr(argv[n],"-m")!=NULL){  // multiple files with startfile
      realign_first_NOE=42;
    }
    if(strstr(argv[n],"-e")!=NULL){  // turn verbose output on
      MULTIS_CALIB_MODE = true;
      printf("\n\n++++++++++++++++++++++++++++++++\n");
      printf("+++ ENERGY CALIBRATION MODE  +++\n");
      printf("++++++++++++++++++++++++++++++++\n");
    }
    if(strstr(argv[n],"-h")!=NULL){  // print help page
      print_usage();
      return(-1);
    }
  } 
  // Check if MA interval length is larger than sample of baseline cut 
  if (L > BASELINE_CUT){
    printf("SETTINGS ERROR: Moving average interval larger than baseline! Not allowed!\n");
    return(-1);
  }
  // Check if MWD interval length is larger than sample of baseline cut 
  if (M > BASELINE_CUT){
    printf("SETTINGS ERROR: MWD moving average interval larger than baseline! Not allowed!\n");
    return(-1);
  }

  //////////////////////////////////////////////////////////
  // Initialize detector readout
  //////////////////////////////////////////////////////////

  // Standard detector analysis mode is cosmics mode, is changed by config file
  sprintf(MODE, "COSMICS"); // standard setting

  // Read the config file if file is given
  if(conf_exists){ 
    bool conf_healthy = read_config(configfile);
    if (conf_healthy == false){
      printf("ERROR: Config file error!\n");
      return(-1);
    }
  }

  // Print mapping if verbose flag is set
  if (is_in_string(VERBOSE, "c")) print_detector_config();

  // Set up detector read out
  DETECTOR.init_readout(inputfile, realign_first_NOE);
  //TRACELEN=DETECTOR.get_maxevents(1);  


  randit(1);

  //////////////////////////////////////////////////////////
  // Construct TFile for storing all plot/histograms and define folder structure
  //////////////////////////////////////////////////////////

  //TFile hfile(outputfile,"RECREATE","NTEC analysis");
  hfile = new TFile(outputfile,"RECREATE","NTEC analysis");
  hfile->SetCompressionLevel(1);
  // Create folder structure
  build_structure();

  // Proto signal is to be extracted
  if (EXTRACT_PROTO == 1){
    sprintf(outputfile_proto_trace, "%s_proto_trace.dat", outputfile);
    proto_out = new ofstream(outputfile_proto_trace);
    if (proto_out->is_open()){
      printf("NOTICE(main): Opened proto trace output file %s.\n", outputfile_proto_trace);
    }
    else printf("WARNING(statistics): Unable to write proto trace file.\n");
  }

  // Open file for saving the energy calibration paramters from cosmics
  if (strcmp(MODE, "COSMICS")==0){
    sprintf(outputfile_energy_calib, "%s_energy_calib.txt", outputfile);
    energy_out = new ofstream(outputfile_energy_calib);
    if (energy_out->is_open()){
      printf("NOTICE(main): Opened energy calibration output file %s.\n", outputfile_energy_calib);
    }
    else printf("WARNING(statistics): Unable to write energy calibration file.\n");
  }

  // array_simulate_proto();

  // Recalculate the energy windows
  E_WINDOW_LENGTH = (E_WINDOW_RIGHT - E_WINDOW_LEFT)/N_E_WINDOW;


  ////////////////////////////////////////////////////////
  // BUILD HISTOGRAMS / LOAD PROTO TRACES FROM FILE
  ////////////////////////////////////////////////////////

  // Reset/initialize signal container
  // If in normal extraction mode, use CHANNELS_EFF
  if (MULTIS_CALIB_MODE == false){
    // Initialize the signal containers
    init_signal(RAW, CHANNELS, "true"); // Has to be CHANNELS 
    init_signal(RAW_CALIB, CHANNELS_EFF);
    init_signal(MA, CHANNELS_EFF);
    init_signal(MWD, CHANNELS_EFF);
    init_signal(TMAX, CHANNELS_EFF);
    init_signal(NMO, CHANNELS_EFF);
    // Init the Calorimeter sum (only one element per filter type in the vector)
    init_signal(ECAL25, 5);
    // Initialize histograms, done in extra functionS
    init_hists(CHANNELS_EFF);
  }
  else{
    init_signal(RAW, CHANNELS);
    // Initialize histograms, done in extra function  
    // Initialize the MULTIS container
    init_multis_norm(MULTIS_NORM, CHANNELS);
    init_hists(CHANNELS);
  }
  // Save the Proto Trace 
  if (EXTRACT_PROTO == 0){
    for (int i = 0; i<(int)RAW_CALIB.size(); i++){
      RAW_CALIB[i].proto_trace_fit = PROTO[i].proto_trace_fit;
    }
  }
  // Save costum threshold multiplicies
  if (TH_MULTIPLICITY.size() != RAW_CALIB.size() && THRESHOLD_MULTIPLICY == -1){
    printf("ERROR: Check your costum threshold multiplicies from the ini file! Must be exactly one row for every real channel!\n");
    printf("TH_MULTIPLICITY.size()=%d, RAW_CALIB.size()=%d\n", (int)TH_MULTIPLICITY.size(), (int)RAW_CALIB.size());
    return(-1);
  }
  if (THRESHOLD_MULTIPLICY == -1){
    for (int i = 0; i<(int)RAW_CALIB.size(); i++){
      RAW_CALIB[i].base.TH_multiplicity = TH_MULTIPLICITY[i];
    }
  }
  else{
    for (int i = 0; i<(int)RAW_CALIB.size(); i++){
      RAW_CALIB[i].base.TH_multiplicity = THRESHOLD_MULTIPLICY;
    }
  }


  printf("INITIALIZATION COMPLETED! BEGIN READOUT!\n");



  /////////////////////////////////////////////////////
  // BEGIN SIGNAL READOUT
  /////////////////////////////////////////////////////

  // Initialize the loop condition
  int m=1;
  // Reset NOE number of events counter
  // loop timer
  high_resolution_clock::time_point t_loop_begin = high_resolution_clock::now();
  NOE=0;
  do{
    // Keep reading as long there are unread events
    m=DETECTOR.read_one_event(NOE);
    // Print some verbose information
    if (is_in_string(VERBOSE, "p")){ // "p" for progress report
      if (no_of_events != 0) {
        if(NOE%((int)no_of_events/10)==0){
		      high_resolution_clock::time_point t_loop_end = high_resolution_clock::now();
		      // duration<double, milli> dur = ( t_loop_end - t_loop_begin );
		      auto duration = duration_cast<milliseconds>( t_loop_end - t_loop_begin ).count();
          cout << "Analysing event: " << NOE << " (" << duration/1000 << "s per cycle)" << endl;
        } 
      }
      else { 
        if(NOE%1000 ==0) {
		      high_resolution_clock::time_point t_loop_end = high_resolution_clock::now();
		      auto duration = duration_cast<milliseconds>( t_loop_end - t_loop_begin ).count();
        	cout << "Analysing event: " << NOE << " (" << duration/1000 << "s per cycle)" << endl;
        }
      }
      // reset timer 
      t_loop_begin = high_resolution_clock::now();
    }
    // Increase Global number of events counter
    NOE++;
    // If there is an event, do the extraction
    if(m==1){ // && t!=-13){  // not eof for either of these files
      // MULTISAMPLING CALIBRATION MODE
      // Depending on the program mode, do either calibration or final extraction
      //
      if(strcmp(MODE, "MULTIS") == 0 || MULTIS_CALIB_MODE == true) {
        // Extract calibration information
        multis_calib();
        // Fill histograms with result, respecting the correct program mode
        fill_hists();
      }
      // FEATURE EXTRACTION MODE
      // Depending on the program mode, do either calibration or final extraction
      //
      else{
        // Extract energy and timing information
        extraction();
        // Fill histograms with result, respecting the correct program mode
        fill_hists();
      }
      // Every 1000 events plot a random event
      if (NOE%1000==0){
        // Plot every 100th raw signal
        hfile->cd("WAVE_FORMS/RAW"); 
        plot_waves(RAW, "Signal_RAW", "TRACE");  
      }
    }
    if(do_no_of_events==1){
      if(NOE>=no_of_events) m=0;
    }
  }while(m==1);

  /////////////////////////////////////////////////////
  // END SIGNAL READOUT
  /////////////////////////////////////////////////////

  // Print final statistics

  if(strcmp(MODE, "MULTIS") == 0 || MULTIS_CALIB_MODE == true) {
    print_stat_multis_calib();
    plot_multis_hist();
  }
  else{
    print_final_statistics();
  }

  /////////////////////////////////////////////////////
  // END PHYSICS PROGRAM
  /////////////////////////////////////////////////////

  // End programm timer
  high_resolution_clock::time_point t_total_end = high_resolution_clock::now();

  auto duration = duration_cast<milliseconds>( t_total_end - t_total_begin ).count();
  if (NB_ACT_CHANNELS == 0) NB_ACT_CHANNELS++; // Avoid deviding by zero
  cout << endl << "Program exectuion time: " << (duration/1000) 
      << "s (" << duration/NB_ACT_CHANNELS/1000 << "s per active channel)" << endl << endl;

  // Write and save and delete the root file element in program
  printf("Writing root file...\n"); 
  hfile->Write();   
  // If put stuff after deleting hfile, software might crash. Beware!
  delete hfile;
  printf("\nRoot file written! Program ends here.\n");
  // If put stuff after deleting hfile, software might crash. Beware!
  
  /////////////////////////////////////////////////////
  // END OF PROGRAM
  /////////////////////////////////////////////////////
}

/////////////////////////////////////////////////////
// ENERGY AND TIMING EXTRACITON
/////////////////////////////////////////////////////
void extraction(){
  /////////////////////////////////////////////////////
  // INITIALIZE SIGNAL CONTAINERS
  /////////////////////////////////////////////////////
  reset_signal(RAW); // raw has to be of lentgh CHANNELS 
  reset_signal(RAW_CALIB);
  reset_signal(MA);
  reset_signal(MWD);
  reset_signal(TMAX);
  reset_signal(NMO);
  reset_signal(ECAL25);
  /////////////////////////////////////////////////////
  // FETCH RAW CONTENT FROM DATA
  /////////////////////////////////////////////////////
  // Create dummy container for fetching the signal
  int entry = 0;
  unsigned int dumm_cont[CHANNELS][TRACELEN];
  // Initialization of the boards (has to start with 1, since 0 is usually the IOL board)
  //    and fetching the ADC content
  for(int board=1; board<=BOARDS; board++){
    for(int channel=0; channel<8; channel++){
      // Convert the board channel to overall channel 
      entry = (board-1)*8 + channel;
      // Fetch the channels
      DETECTOR.get_trace(board, channel, dumm_cont[entry]);
      // Save the entries in the RAW container
      for(int n=0; n<TRACELEN; n++){
        double dumm = (double) dumm_cont[entry][n]; // Has no function but is needed to work (dunno why)
        RAW[entry].trace.push_back((double) dumm_cont[entry][n]);
        dumm++; // Has no function but is needed to work (dunno why) 
      }
      // Calculate the baseline information for the RAW and RAW_Calib
      RAW[entry].base.mean = array_mean(RAW[entry].trace, 0,BASELINE_CUT);
      // Already subtract the baseline from the signals
      for (int n = 0; n<TRACELEN; n++){
        RAW[entry].trace[n] -= RAW[entry].base.mean;
      }
      // Recalculate the baseline information for the RAW and RAW_Calib
      RAW[entry].base.mean = array_mean(RAW[entry].trace, 0,BASELINE_CUT);
      // Now calculate the std and TH for the baselines
      RAW[entry].base.std = array_std(RAW[entry].trace,0, BASELINE_CUT,RAW[entry].base.mean);
      RAW[entry].base.TH = THRESHOLD_MULTIPLICY * RAW[entry].base.std; 
    }
  }    
  // According to polarity and baseline, transform negative signals into positive signals
  for (int a = 0; a<(int)MAPPING.size(); a++){

    for (int b = 0; b<(int)MAPPING[a].size(); b++){

      // Mapped hardware channel in RAW container
      int h = MAPPING[a][b].board_nb * 8 + MAPPING[a][b].h_channel_start;
      // If it's a solo channel
      if (MAPPING[a][b].multis == 1 && MAPPING[a][b].polarity == -1){
        for (int n = 0; n < (int)RAW[h].trace.size(); n++){
          RAW[h].trace[n] = 2 * RAW[h].base.mean + (RAW[h].trace[n]*(-1));
        }
      } 
      // If not:

      if (MAPPING[a][b].multis == 2 && MAPPING[a][b].polarity == -1){
        for (int n = 0; n < (int)RAW[h].trace.size(); n++){
          RAW[h].trace[n] = 2 * RAW[h].base.mean + (RAW[h].trace[n]*(-1));
          RAW[h+1].trace[n] = 2 * RAW[h+1].base.mean + (RAW[h+1].trace[n]*(-1));
        }
      } 
      if (MAPPING[a][b].multis == 4 && MAPPING[a][b].polarity == -1){
        for (int n = 0; n < (int)RAW[h].trace.size(); n++){
          RAW[h].trace[n] = 2 * RAW[h].base.mean + (RAW[h].trace[n]*(-1));
          RAW[h+1].trace[n] = 2 * RAW[h+1].base.mean + (RAW[h+1].trace[n]*(-1));
          RAW[h+2].trace[n] = 2 * RAW[h+2].base.mean + (RAW[h+2].trace[n]*(-1));
          RAW[h+3].trace[n] = 2 * RAW[h+3].base.mean + (RAW[h+3].trace[n]*(-1));
        }
      } 
    }
  }
  // i is the software channel, including all multisampling channels. mapping from hardware channels has to be done
  for (int a = 0; a<(int)MAPPING.size(); a++){
    for (int b = 0; b<(int)MAPPING[a].size(); b++){
      // Chrystal channel (Channel of matrix)
      int ch = b*(int)MAPPING.size()+a; 
      // Mapped hardware channel in RAW container
      int h = MAPPING[a][b].board_nb * 8 + MAPPING[a][b].h_channel_start;
      // Check if hardware channel is valid channel, otherwise leave that channel out and set the flag
      if (MAPPING[a][b].board_nb != 99) {
        RAW_CALIB[ch].is_valid = true;      
        MA[ch].is_valid = true;      
        MWD[ch].is_valid = true;      
        TMAX[ch].is_valid = true;  
        NMO[ch].is_valid = true;  
        // Aditionally for the RAW channels
        RAW[h].is_valid = true;
        if (MAPPING[a][b].multis == 2){
          RAW[h+1].is_valid = true;
        }
        if (MAPPING[a][b].multis == 4){
          RAW[h+1].is_valid = true;
          RAW[h+2].is_valid = true;
          RAW[h+3].is_valid = true;
        }
      }
      else continue;
      // Now fill the software channels according to the mapping and multisampling
      for(int n=0; n<TRACELEN; n++){
        if (MAPPING[a][b].multis == 1){
          RAW_CALIB[ch].trace.push_back( ((double) RAW[h].trace[n] )* CALIB.multis[h] * CALIB.RAW_energy[ch] * GENERAL_SCALING);
          // Update the channel info
          RAW_CALIB[ch].hardware_addr = h;
          RAW_CALIB[ch].polarity = MAPPING[a][b].polarity;
        }
        else if (MAPPING[a][b].multis == 2){
          // Save the samples correctly 
          RAW_CALIB[ch].trace.push_back((double) RAW[h+1].trace[n] * CALIB.multis[h+1] * CALIB.RAW_energy[ch] * GENERAL_SCALING);       
          RAW_CALIB[ch].trace.push_back((double) RAW[h].trace[n] * CALIB.multis[h] * CALIB.RAW_energy[ch] * GENERAL_SCALING);
          // Update the channel information
          RAW_CALIB[ch].multis = 2;
          RAW_CALIB[ch].clock_speed = 200; // in MHz
          RAW_CALIB[ch].sample_t = 5.; // in MHz
          RAW_CALIB[ch].tracelen = TRACELEN * 2;
          RAW_CALIB[ch].hardware_addr = h;
          RAW_CALIB[ch].polarity = MAPPING[a][b].polarity;
        }
        else if (MAPPING[a][b].multis == 4){
          // Save the samples correctly
          RAW_CALIB[ch].trace.push_back((double) RAW[h+3].trace[n] * CALIB.multis[h+3] * CALIB.RAW_energy[ch] * GENERAL_SCALING);
          RAW_CALIB[ch].trace.push_back((double) RAW[h+2].trace[n] * CALIB.multis[h+2] * CALIB.RAW_energy[ch] * GENERAL_SCALING);
          RAW_CALIB[ch].trace.push_back((double) RAW[h+1].trace[n] * CALIB.multis[h+1] * CALIB.RAW_energy[ch] * GENERAL_SCALING);
          RAW_CALIB[ch].trace.push_back((double) RAW[h].trace[n] * CALIB.multis[h] * CALIB.RAW_energy[ch] * GENERAL_SCALING);
          // Update the channel information
          RAW_CALIB[ch].multis = 4;
          RAW_CALIB[ch].clock_speed = 400; // in MHz
          RAW_CALIB[ch].sample_t = 2.5; // in MHz
          RAW_CALIB[ch].tracelen = TRACELEN * 4;
          RAW_CALIB[ch].hardware_addr = h;
          RAW_CALIB[ch].polarity = MAPPING[a][b].polarity;
        }
      }
    }
  }

  for(int board=1; board<=BOARDS; board++){
    for(int channel=0; channel<8; channel++){
      // Convert the board channel to overall channel 
      entry = (board-1)*8 + channel;
      RAW[entry].CFD.trace.clear();
      RAW[entry].CFD.trace = CFD(RAW[entry].trace, 1);
    }
  }

  for (int i = 0; i < (int)RAW_CALIB.size(); i++){
	  /////////////////////////////////////////////////////
	  // APPLICATION OF THE FILTER TO RAW_CALIB 
	  /////////////////////////////////////////////////////
	  MA[i].trace.clear(); MA[i].sample_t = RAW_CALIB[i].sample_t; MA[i].multis = RAW_CALIB[i].multis;
	  MWD[i].trace.clear(); MWD[i].sample_t = RAW_CALIB[i].sample_t; MWD[i].multis = RAW_CALIB[i].multis;
	  TMAX[i].trace.clear(); TMAX[i].sample_t = RAW_CALIB[i].sample_t; TMAX[i].multis = RAW_CALIB[i].multis;
	  MA[i].trace = MA_filter(RAW_CALIB[i].trace, CALIB.MA_energy[i], L*MA[i].multis);
	  MWD[i].trace = MWD_filter(RAW_CALIB[i].trace, CALIB.MWD_energy[i]);
    // if (i == 7) printf("%3.3f\n", CALIB.TMAX_energy[i]);
	  TMAX[i].trace = FIR_filter(RAW_CALIB[i].trace, CALIB.TMAX_energy[i]);
    for (int n = 0; n < (int)MA[i].trace.size(); n++){
      // printf("%3.1f\n",RAW_CALIB[i].trace[n] );
      // printf("%3.1f %3.1f %3.1f\n", RAW_CALIB[i].trace[n], MA[i].trace[n], (double)FIR_COEF.size() );
    }
    // printf("++++++++++++++++++++++\n");

	  /////////////////////////////////////////////////////
	  // APPLICATION OF THE CONSTANT FRACTION DISCRIMINATOR 
	  /////////////////////////////////////////////////////
	  RAW_CALIB[i].CFD.trace.clear();
	  MA[i].CFD.trace.clear();
	  MWD[i].CFD.trace.clear();
	  TMAX[i].CFD.trace.clear();
	  RAW_CALIB[i].CFD.trace = CFD(RAW_CALIB[i].trace, RAW_CALIB[i].multis);
	  MA[i].CFD.trace = CFD(MA[i].trace, RAW_CALIB[i].multis);
	  MWD[i].CFD.trace = CFD(MWD[i].trace, RAW_CALIB[i].multis);
	  TMAX[i].CFD.trace = CFD(TMAX[i].trace, RAW_CALIB[i].multis);
	}

  /////////////////////////////////////////////////////
  // CALCULATE BASELINE STATISTICS 
  /////////////////////////////////////////////////////
  // Calculate the baseline statistics
  for (int i = 0; i < (int)RAW_CALIB.size(); i++){
    // Check if channel is valid
    if (RAW_CALIB[i].is_valid == false) continue;
    // otherwise do the calculation
    RAW_CALIB[i].base.mean = array_mean(RAW_CALIB[i].trace, 0,BASELINE_CUT*RAW_CALIB[i].multis);
    RAW_CALIB[i].base.std = array_std(RAW_CALIB[i].trace, 0, BASELINE_CUT*RAW_CALIB[i].multis,RAW_CALIB[i].base.mean);
    RAW_CALIB[i].base.TH = RAW_CALIB[i].base.TH_multiplicity * RAW_CALIB[i].base.std; 
    //
    MA[i].base.mean = array_mean(MA[i].trace, 0,BASELINE_CUT*MA[i].multis);
    MA[i].base.std = array_std(MA[i].trace, 0, BASELINE_CUT*MA[i].multis,MA[i].base.mean);
    MA[i].base.TH = RAW_CALIB[i].base.TH_multiplicity * MA[i].base.std; 
    //
    MWD[i].base.mean = array_mean(MWD[i].trace, 0,BASELINE_CUT*MWD[i].multis);
    MWD[i].base.std = array_std(MWD[i].trace, 0, BASELINE_CUT*MWD[i].multis,MWD[i].base.mean);
    MWD[i].base.TH = RAW_CALIB[i].base.TH_multiplicity * MWD[i].base.std; 
    //
    TMAX[i].base.mean = array_mean(TMAX[i].trace, 0,BASELINE_CUT*TMAX[i].multis);
    TMAX[i].base.std = array_std(TMAX[i].trace, 0, BASELINE_CUT*TMAX[i].multis,TMAX[i].base.mean);
    TMAX[i].base.TH = RAW_CALIB[i].base.TH_multiplicity * TMAX[i].base.std; 

    // printf("%3.3f %3.3f %3.3f %d\n", MA[i].base.mean, MA[i].base.std, MA[i].base.TH, MA[i].multis);
  }
  /////////////////////////////////////////////////////
  // EXTRACT FEATURES FROM TRACES
  /////////////////////////////////////////////////////
  // First the RAW traces
  for(int i=0; i<(int)RAW.size(); i++){
    // Extract the maximum
  	RAW[i].energy = RAW[i].trace[array_largest(RAW[i].trace, BASELINE_CUT, ENERGY_WINDOW_MAX)];
    RAW[i].energy_n = array_largest(RAW[i].trace, BASELINE_CUT, ENERGY_WINDOW_MAX);
  	// Look for CFD Zero Crossing
    /// Search for minimum before zero xrossing
  	RAW[i].CFD.Xzero = array_zero_xing(RAW[i].CFD.trace, ZERO_XING_CUT, RAW[i].energy_n, -1);
    // Already fill the samples into a histogram for baseline noise estimation
    for(int n=0; n < BASELINE_CUT; n++){
      RAW[i].base.h_samples.hist->Fill(RAW[i].trace[n]);   
    }
  }

	int plot = 0;

  //
  // Then the Filtered traces
  // 
  // In beam mode first check if the central crystal is healthy
  int central_healty = 0;
  int valid_max = 0;
  // Left and Right borders for maximum detection
  int left = BASELINE_CUT * RAW_CALIB[0].multis;
  int right = ENERGY_WINDOW_MAX * RAW_CALIB[0].multis;
  // Already extract features of RAW_CALIB
  if (strcmp(MODE, "BEAM")==0){
    RAW_CALIB[CENTRAL].energy_n = array_largest(RAW_CALIB[CENTRAL].trace, left, right); // sample number of largest sample
    RAW_CALIB[CENTRAL].energy = RAW_CALIB[CENTRAL].trace[RAW_CALIB[CENTRAL].energy_n];
    RAW_CALIB[CENTRAL].integral = signal_integral(RAW_CALIB[CENTRAL], 0);
    RAW_CALIB[CENTRAL].ratio = RAW_CALIB[CENTRAL].integral / RAW_CALIB[CENTRAL].energy;
    //
    TMAX[CENTRAL].energy_n = array_largest(TMAX[CENTRAL].trace, left, right);
    TMAX[CENTRAL].energy = TMAX[CENTRAL].trace[TMAX[CENTRAL].energy_n];
    TMAX[CENTRAL].integral = signal_integral(TMAX[CENTRAL], 0);
    TMAX[CENTRAL].ratio = TMAX[CENTRAL].integral / TMAX[CENTRAL].energy;
    TMAX[CENTRAL].is_signal = 1;
    TMAX[CENTRAL].CFD.Xzero = array_zero_xing(TMAX[CENTRAL].CFD.trace, ZERO_XING_CUT*TMAX[CENTRAL].multis, array_largest(TMAX[CENTRAL].CFD.trace, 0, 1E5), -1);
    
    if (strcmp("RAW_CALIB", VALIDITY) == 0) valid_max = is_valid_max( RAW_CALIB[CENTRAL], RAW_CALIB[CENTRAL].energy_n );
    if (strcmp("TMAX", VALIDITY) == 0) valid_max = is_valid_max( TMAX[CENTRAL], TMAX[CENTRAL].energy_n );
    
    //
    if ( valid_max == 0 ){
      // printf("%3.3f\n", TMAX[CENTRAL].ratio);
      if ( RAW_CALIB[CENTRAL].ratio < UPPER_RATIO && RAW_CALIB[CENTRAL].ratio > LOWER_RATIO){
      // if ( RAW_CALIB[CENTRAL].ratio < UPPER_RATIO && RAW_CALIB[CENTRAL].ratio > LOWER_RATIO){
        RAW_CALIB[CENTRAL].is_signal = 1;
        central_healty = 1;
        FILTER_EFFICIENCY[0]++;
      }
      else {
        central_healty = 0;
        plot = 6;
        FILTER_EFFICIENCY[6]++;
      }
    }
    else {
      central_healty = 0;
      plot = valid_max;
      FILTER_EFFICIENCY[valid_max]++;
    }
  }
  else central_healty = 1;

  // Now loop through all the channels
	for(int i=0; i<(int)RAW_CALIB.size(); i++){
    // plot = 1;
    // Only extract features from valid channels
    if (RAW_CALIB[i].is_valid == false) continue;
		// Left and Right borders for maximum detection
		int left = BASELINE_CUT * RAW_CALIB[i].multis;
		int right = ENERGY_WINDOW_MAX * RAW_CALIB[i].multis;
		// Already extract features of RAW_CALIB
    RAW_CALIB[i].energy_n = array_largest(RAW_CALIB[i].trace, RAW_CALIB[i].multis*BASELINE_CUT, RAW_CALIB[i].multis*ENERGY_WINDOW_MAX); // sample number of largest sample
    RAW_CALIB[i].energy = RAW_CALIB[i].trace[RAW_CALIB[i].energy_n];
    RAW_CALIB[i].integral = signal_integral(RAW_CALIB[i], 0);
    RAW_CALIB[i].ratio = RAW_CALIB[i].integral / RAW_CALIB[i].energy;
    RAW_CALIB[i].CFD.Xzero = array_zero_xing(RAW_CALIB[i].CFD.trace, ZERO_XING_CUT*RAW_CALIB[i].multis, array_largest(RAW_CALIB[i].CFD.trace, 0, 1E5), -1);
    // printf("%3.1f %3.1f %d\n", RAW_CALIB[i].energy, RAW_CALIB[i].base.TH, RAW_CALIB[i].energy_n);

    // printf("%d %3.3f %3.3f %3.3f %d\n", RAW_CALIB[i].energy_n, RAW_CALIB[i].energy, RAW_CALIB[i].integral, RAW_CALIB[i].ratio, is_valid_max( RAW_CALIB[i], RAW_CALIB[i].energy_n ));
    TMAX[i].energy_n = array_largest(TMAX[i].trace, TMAX[i].multis*BASELINE_CUT, TMAX[i].multis*ENERGY_WINDOW_MAX); // sample number of largest sample
    TMAX[i].energy = TMAX[i].trace[TMAX[i].energy_n];
    TMAX[i].integral = signal_integral(TMAX[i], 0);
    TMAX[i].ratio = TMAX[i].integral / TMAX[i].energy;
    TMAX[i].CFD.Xzero = array_zero_xing(TMAX[i].CFD.trace, ZERO_XING_CUT*TMAX[i].multis, array_largest(TMAX[i].CFD.trace, 0, 1E5), -1);

    // Search for maximum and check if valid
    // Decide which filter type to chose for making the validity decicion
    if ( is_valid_max( RAW_CALIB[i], RAW_CALIB[i].energy_n ) == 0 && central_healty == 1 ){ // The maximum is valid
      // Signal is good
      RAW_CALIB[i].is_signal = 1;
      // Save the sum of all signals per channel
      RAW_CALIB[i].proto_trace = array_sum(RAW_CALIB[i].proto_trace, RAW_CALIB[i].trace, 1);
      // Fill the proto trace hist
      for (int n = 0; n < (int)RAW_CALIB[i].proto_trace.size(); n++){
        RAW_CALIB[i].TH2D_proto_trace.hist->Fill(n, RAW_CALIB[i].trace[n]);
      }
      //
    	// Only look for the filter traces if raw trace is not a glitch
      MA[i].energy_n = array_largest(MA[i].trace, left, right);
      // printf("%3.3f %d %d\n", MA[i].base.TH, MA[i].energy_n, is_valid_max( MA[i], MA[i].energy_n ));
      // printf("%d %3.3f %d\n", MA[i].energy_n, MA[i].energy, is_valid_max( MA[i], MA[i].energy_n ));
      if ( is_valid_max( MA[i], MA[i].energy_n ) == 0 ){
        MA[i].energy = MA[i].trace[MA[i].energy_n];
        MA[i].integral = signal_integral(MA[i], 0);
        MA[i].ratio = MA[i].integral / MA[i].energy;
        MA[i].is_signal = 1;
        MA[i].CFD.Xzero = array_zero_xing(MA[i].CFD.trace, ZERO_XING_CUT*MA[i].multis, array_largest(MA[i].CFD.trace, 0, 1E5), -1);
      }
      else{ MA[i].energy = 0; MA[i].integral = 0; MA[i].ratio = 0; MA[i].is_signal = 0;}
      //
      MWD[i].energy_n = array_largest(MWD[i].trace, left, right);
      if ( is_valid_max( MWD[i], MWD[i].energy_n ) == 0 ){
        MWD[i].energy = MWD[i].trace[MWD[i].energy_n];
        MWD[i].integral = signal_integral(MWD[i], 0);
        MWD[i].ratio = MWD[i].integral / MWD[i].energy;
        MWD[i].is_signal = 1;
        MWD[i].CFD.Xzero = array_zero_xing(MWD[i].CFD.trace, ZERO_XING_CUT*MWD[i].multis, array_largest(MWD[i].CFD.trace, 0, 1E5), -1);

      }
      else{ MWD[i].energy = 0; MWD[i].integral = 0; MWD[i].ratio = 0; MWD[i].is_signal = 0;}
      //
      TMAX[i].energy_n = array_largest(TMAX[i].trace, left, right);
      // printf("%d %d\n", TMAX[i].energy_n, (int)TMAX[i].trace.size());
      // if ( is_valid_max( TMAX[i], TMAX[i].energy_n ) == 0 ){
        TMAX[i].energy = TMAX[i].trace[TMAX[i].energy_n];
        TMAX[i].integral = signal_integral(TMAX[i], 0);
        TMAX[i].ratio = TMAX[i].integral / TMAX[i].energy;
        TMAX[i].is_signal = 1;
        TMAX[i].CFD.Xzero = array_zero_xing(TMAX[i].CFD.trace, ZERO_XING_CUT*TMAX[i].multis, array_largest(TMAX[i].CFD.trace, 0, 1E5), -1);
      // }
      // else{ TMAX[i].energy = 0; TMAX[i].integral = 0; TMAX[i].ratio = 0; TMAX[i].is_signal = 0;}
      // printf("%d %d %d %d \n", RAW_CALIB[i].CFD.Xzero, MA[i].CFD.Xzero, MWD[i].CFD.Xzero, TMAX[i].CFD.Xzero );

      // For the sake of computational efficiency, only do the Nelder-Mead optimization for signal events

      // If the proto traces are already extracted, do the optimization
      if (EXTRACT_PROTO==0){
        // Now do optimization for prototraces and RAW_CALIB

        // For initial parameters, take:
            // For amplitude, calculate the ratio between trace maximum and proto trace maximum
            // For shift, look for sample number of trace maximum

        // double init_A = RAW_CALIB[i].trace[RAW_CALIB[i].energy_n] / RAW_CALIB[i].proto_trace[array_largest(RAW_CALIB[i].proto_trace, 0, (int)RAW_CALIB[i].proto_trace.size())];

        vector<double> init; init.push_back(1); init.push_back(0.0);

        double a0[]={0.0, -100.0}, a1[]={1.5, 0.0}, a2[]={ 0.0, 100.0};
        vector<vector<double> > simplex;
        simplex.push_back( vector<double>(a0,a0+2) );
        simplex.push_back( vector<double>(a1,a1+2) );
        simplex.push_back( vector<double>(a2,a2+2) );

        vector<double> weights;

        for (int n = 0; n < 90; n++){
          weights.push_back(1);
        }
        for (int n = 100; n < 120; n++){
          weights.push_back(10);
        }
        for (int n = 110; n < (int)RAW_CALIB[i].trace.size(); n++){
          weights.push_back(1);
        }

        vector<double> snadbox;
        snadbox.push_back(1.0); 
        snadbox.push_back(0.0); 

        if (i==0) {
          RAW_CALIB[i].trace = array_adjust(RAW_CALIB[i].trace, snadbox, 0);
          TMAX[i].trace = FIR_filter(RAW_CALIB[i].trace, CALIB.TMAX_energy[i]);
        }   
        
        // if (i==0) plot = 1;

        // The whole optimization process is stored, where the last element is the optimium
        vector<vector<double> > opt;
        // printf("\n%d %d\n", (int)RAW_CALIB[i].trace.size(), (int)RAW_CALIB[i].proto_trace_fit.size());
        int debug = 0;
        // if (i == 1) debug = 2;
        opt = BT::Simplex(array_compare, 
                      RAW_CALIB[i].trace,
                      RAW_CALIB[i].proto_trace_fit,
                      weights,
                      50, 250,
                      init,
                      (double) 1e-7,
                      simplex,
                      1E2,
                      debug
        );
        // printf("++++++++++\n %3.3f %3.3f\n++++++++++", opt[0], opt[1]);
        // Save optimized wave form in NMO trace
        
        NMO[i].trace = array_adjust(RAW_CALIB[i].proto_trace_fit, opt.back(), 0);
        NMO[i].is_signal = 1;
        NMO[i].is_valid = true;

        int len1 = (int)NMO[i].trace.size();
        int len2 = (int)RAW_CALIB[i].trace.size();
        double frac = (double)len1/(double)len2;


        NMO[i].energy_n = array_largest(NMO[i].trace, (int)left*frac, (int)right*frac); // sample number of largest sample
        NMO[i].energy = NMO[i].trace[NMO[i].energy_n];
        NMO[i].integral = signal_integral(NMO[i], 0);
        NMO[i].ratio = NMO[i].integral / NMO[i].energy;
        NMO[i].CFD.Xzero = array_zero_xing(NMO[i].CFD.trace, BASELINE_CUT*NMO[i].multis, array_largest(NMO[i].CFD.trace, 0, 1E5), -1);


        // if (i == CENTRAL ) plot = 1;

        // plot = 1;

      }
      else{
        // For the NMO, fill the non-optimized traces with 0
        for (int n = 0; n < (int)RAW_CALIB[0].trace.size(); n++){
          NMO[i].trace.push_back(0.0);
        }
      }

    }
    else { 
    	// If not valid, set all energies and integrals to 0
    	RAW_CALIB[i].energy = 0; RAW_CALIB[i].integral = 0;RAW_CALIB[i].ratio = 0; RAW_CALIB[i].is_signal = 0;
    	MA[i].energy = 0; MA[i].integral = 0; MA[i].ratio = 0; MA[i].is_signal = 0;
    	MWD[i].energy = 0; MWD[i].integral = 0; MWD[i].ratio = 0; MWD[i].is_signal = 0;
    	TMAX[i].energy = 0; TMAX[i].integral = 0; TMAX[i].ratio = 0; TMAX[i].is_signal = 0;
      NMO[i].energy = 0; NMO[i].integral = 0; NMO[i].ratio = 0; NMO[i].is_signal = 0;
      // For the NMO, fill the non-optimized traces with 0
      for (int n = 0; n < (int)RAW_CALIB[0].trace.size(); n++){
        NMO[i].trace.push_back(0.0);
      }
    }

    // Calculate sample_t of NMO which can be different than 
    double nmo_tracelen = (double)NMO[i].trace.size();
    NMO[i].sample_t = (double)( SAMPLE_t * (TRACELEN/nmo_tracelen) ); // NMO can have a higher sampling rate, calculate it based on the ADC sampling
    // printf("%3.2f\n", NMO[i].sample_t);

    // Calculate the CFD traces
    RAW_CALIB[i].CFD.trace.clear();
    RAW_CALIB[i].CFD.trace = CFD(RAW_CALIB[i].trace, RAW_CALIB[i].multis);
    MA[i].CFD.trace.clear();
    MA[i].CFD.trace = CFD(MA[i].trace, MA[i].multis);
    MWD[i].CFD.trace.clear();
    MWD[i].CFD.trace = CFD(MWD[i].trace, MWD[i].multis);
    TMAX[i].CFD.trace.clear();
    TMAX[i].CFD.trace = CFD(TMAX[i].trace, TMAX[i].multis);
    // Calculate the multiplication adjustment to the higher res NMO trace
    double multiplier = (double)(NMO[i].trace.size() / RAW_CALIB[i].trace.size());
    //
    NMO[i].CFD.trace.clear();
    NMO[i].CFD.trace = CFD(NMO[i].trace, NMO[i].multis*multiplier);
  

    // if ( RAW_CALIB[i].ratio == 0 ){
    // 	if ( i == 4 ) plot = 1;
    // }
    // if ( is_valid_max( RAW_CALIB[i], array_largest(RAW_CALIB[i].trace, left, right) ) == 4 ){
    // 	if ( i == 4 ) plot = 2;
    // }

  }



  /////////////////////////////////////////////////////
  // TEST FOR COINCIDENCE
  /////////////////////////////////////////////////////
  // Check for coincidences according to the analysis mode
  // + Column cut: only look for coincident events in stacked crystals

  // for (int i = 0; i < (int)RAW_CALIB.size(); i++){
  //   printf("%d ", RAW_CALIB[i].is_signal);
  // }
  // printf("\n");

  // Column cut / cosmics mode:
  bool is_coinc = false;
  //
  if(strcmp(MODE, "COSMICS") == 0) {
    // Array for saving column information
    int column = 0; 
    // Check for each column if there was any event
    for (int b = 0; b < (int)MAPPING[0].size(); b++){
      column = 0;
      for (int a = 0; a < (int)MAPPING.size(); a++){
        // Chrystal channel (Channel of matrix)
        int ch = b*(int)MAPPING.size()+a; 
        if (RAW_CALIB[7].is_signal == 1) {
          plot = 1;
          // printf("%3.3f %3.3f %3.3f\n", RAW_CALIB[7].energy, RAW_CALIB[7].base.TH, RAW_CALIB[7].base.std);
        }
        // Check if there was a signal in channel ch
        if (RAW_CALIB[ch].is_signal == 1 && RAW_CALIB[ch].is_valid == 1){
          column++;
        }
      }
      // printf("%d %d\n", column, (int)MAPPING.size() - 1);
      // Now check how many signals per column
      if (column < COINC_LEVEL ){//(int)MAPPING[0].size()-1){
        // Throw away events in this column 
        for (int a = 0; a < (int)MAPPING.size(); a++){
          // Chrystal channel (Channel of matrix)
          int ch = b*(int)MAPPING.size()+a; 
          RAW_CALIB[ch].energy = 0;
          MA[ch].energy = 0;
          MWD[ch].energy = 0;
          TMAX[ch].energy = 0;
          NMO[ch].energy = 0;
          //
          RAW_CALIB[ch].integral = 0;
          MA[ch].integral = 0;
          MWD[ch].integral = 0;
          TMAX[ch].integral = 0;
          NMO[ch].integral = 0;
        }
      }
      else is_coinc = true;
      // If it's only one crystal per column, set the coincidencec setting still to true
      if ( (int)MAPPING.size() == 1){
        is_coinc = true;
      }
      // If there are coincident events, do timing extraction
      else{
        // start the interpolation by looping over all channels
        // interpolate is searching for the zero crossing point of the CFD signals
        // interpolate(RAW_CALIB);
        // interpolate(MA);
        // interpolate(MWD);
        // interpolate(TMAX);
        // And now compare the timing
        // time_compare(RAW_CALIB);
        // time_compare(MA);
        // time_compare(MWD);
        // time_compare(TMAX);
      }
    }
  } 
  //
  //
  if(strcmp(MODE, "BEAM") == 0) {
    is_coinc = false;
    // If xtal in beam is not valid
    // Read out the tagger statistics
    int tags_per_event = 0;
    for(int n=0; n<TAG_CHANNELS; n++){
      TAGGER.time[n]=DETECTOR.get_value(BOARDS+1, n, 1);
      // Check for multiple taggs or non tags
      if (TAGGER.time[n] != 0.0 ){
        tags_per_event++;
        TAGGER.counts[n]++;
      } 
    }
    // Enter number of tags per event into the counter
    TAGGER.multiples_per_count[tags_per_event]++;
    // Check in witch channel the multi counts appeared
    if (tags_per_event > 1){
      for(int n=0; n<TAG_CHANNELS; n++){
        if (TAGGER.time[n] != 0.0 ){
          TAGGER.multiples_per_channel[n]++;
        } 
      }
    }
    // Enter the tagged energy in the right histogram and do the timing
    for (int k = 0; k < N_E_WINDOW; k ++){
      // If the right tagger channel is found
      for (int i = 0; i < (int)RAW_CALIB.size(); i++){
        // Check for the central crystal
        if ( i==CENTRAL && RAW_CALIB[i].is_signal == 1 ) is_coinc = true; 
        // Fill all events for the right tagger
        if ( RAW_CALIB[i].is_signal == true && // If signal is valid
             TAGGER.time[k] != 0 // and if tagger is set for energy k
              ){ 
          RAW_CALIB[i].tagged[k].energy = RAW_CALIB[i].energy;
          MA[i].tagged[k].energy = MA[i].energy;
          MWD[i].tagged[k].energy = MWD[i].energy;
          TMAX[i].tagged[k].energy = TMAX[i].energy;
          NMO[i].tagged[k].energy = NMO[i].energy;
          //
          RAW_CALIB[i].tagged[k].integral = RAW_CALIB[i].integral;
          MA[i].tagged[k].integral = MA[i].integral;
          MWD[i].tagged[k].integral = MWD[i].integral;
          TMAX[i].tagged[k].integral = TMAX[i].integral;
          NMO[i].tagged[k].integral = NMO[i].integral;
          //
          if (RAW_CALIB[i].tagged[k].energy > ENERGY_CUT) ECAL25[0].tagged[k].energy += RAW_CALIB[i].tagged[k].energy;
          if (MA[i].tagged[k].energy > ENERGY_CUT)        ECAL25[1].tagged[k].energy += MA[i].tagged[k].energy;
          if (MWD[i].tagged[k].energy > ENERGY_CUT)       ECAL25[2].tagged[k].energy += MWD[i].tagged[k].energy;
          if (TMAX[i].tagged[k].energy > ENERGY_CUT)      ECAL25[3].tagged[k].energy += TMAX[i].tagged[k].energy;
          if (NMO[i].tagged[k].energy > ENERGY_CUT)       ECAL25[4].tagged[k].energy += NMO[i].tagged[k].energy;
          //
          ECAL25[0].tagged[k].integral += RAW_CALIB[i].integral;
          ECAL25[1].tagged[k].integral += MA[i].integral;
          ECAL25[2].tagged[k].integral += MWD[i].integral;
          ECAL25[3].tagged[k].integral += TMAX[i].integral;
          ECAL25[4].tagged[k].integral += NMO[i].integral;
        }
        else{
          RAW_CALIB[i].tagged[k].energy = 0.0;
          MA[i].tagged[k].energy = 0.0;
          MWD[i].tagged[k].energy = 0.0;
          TMAX[i].tagged[k].energy = 0.0;
          NMO[i].tagged[k].energy = 0.0;
          //
          RAW_CALIB[i].tagged[k].integral = 0.0;
          MA[i].tagged[k].integral = 0.0;
          MWD[i].tagged[k].integral = 0.0;
          TMAX[i].tagged[k].integral = 0.0;
          NMO[i].tagged[k].integral = 0.0;
        }
        // Fill energies only if no multiples are detected
        if ( RAW_CALIB[i].is_signal == true && // If signal is valid
             TAGGER.time[k] != 0 && // and if tagger is set for energy k
             tags_per_event == 1 // and if only one tagged energy per tag
              ){    
          RAW_CALIB[i].tagged[k].energy_m = RAW_CALIB[i].energy;
          MA[i].tagged[k].energy_m = MA[i].energy;
          MWD[i].tagged[k].energy_m = MWD[i].energy;
          TMAX[i].tagged[k].energy_m = TMAX[i].energy;
          NMO[i].tagged[k].energy_m = NMO[i].energy;
          //
          RAW_CALIB[i].tagged[k].integral_m = RAW_CALIB[i].integral;
          MA[i].tagged[k].integral_m = MA[i].integral;
          MWD[i].tagged[k].integral_m = MWD[i].integral;
          TMAX[i].tagged[k].integral_m = TMAX[i].integral;
          NMO[i].tagged[k].integral_m = NMO[i].integral;
          //
          if (RAW_CALIB[i].tagged[k].energy_m > ENERGY_CUT) ECAL25[0].tagged[k].energy_m += RAW_CALIB[i].tagged[k].energy_m;
          if (MA[i].tagged[k].energy_m > ENERGY_CUT)        ECAL25[1].tagged[k].energy_m += MA[i].tagged[k].energy_m;
          if (MWD[i].tagged[k].energy_m > ENERGY_CUT)       ECAL25[2].tagged[k].energy_m += MWD[i].tagged[k].energy_m;
          if (TMAX[i].tagged[k].energy_m > ENERGY_CUT)      ECAL25[3].tagged[k].energy_m += TMAX[i].tagged[k].energy_m;
          if (NMO[i].tagged[k].energy_m > ENERGY_CUT)       ECAL25[4].tagged[k].energy_m += NMO[i].tagged[k].energy_m;
          //
          ECAL25[0].tagged[k].integral_m += RAW_CALIB[i].tagged[k].integral_m;
          ECAL25[1].tagged[k].integral_m += MA[i].tagged[k].integral_m;
          ECAL25[2].tagged[k].integral_m += MWD[i].tagged[k].integral_m;
          ECAL25[3].tagged[k].integral_m += TMAX[i].tagged[k].integral_m;
          ECAL25[4].tagged[k].integral_m += NMO[i].tagged[k].integral_m;

        }
        else{
          RAW_CALIB[i].tagged[k].energy_m = 0.0;
          MA[i].tagged[k].energy_m = 0.0;
          MWD[i].tagged[k].energy_m = 0.0;
          TMAX[i].tagged[k].energy_m = 0.0;
          NMO[i].tagged[k].energy_m = 0.0;
          //
          RAW_CALIB[i].tagged[k].integral_m = 0.0;
          MA[i].tagged[k].integral_m = 0.0;
          MWD[i].tagged[k].integral_m = 0.0;
          TMAX[i].tagged[k].integral_m = 0.0;
          NMO[i].tagged[k].integral_m = 0.0;
        }
        // Fill energies for constrained timing window and without multiples
        if ( RAW_CALIB[i].is_signal == true && // If signal is valid
             TAGGER.time[k] != 0 && // and if tagger is set for energy k
             tags_per_event == 1 && // and if only one tagged energy per tag
             TAGGER.time[k] >= (TAGGER.cut[k] - TAGGER_WINDOW) &&
             TAGGER.time[k] <= (TAGGER.cut[k] + TAGGER_WINDOW) // and if tagger time is in the timing window cut
              ){ 
          RAW_CALIB[i].tagged[k].energy_mt = RAW_CALIB[i].energy;
          MA[i].tagged[k].energy_mt = MA[i].energy;
          MWD[i].tagged[k].energy_mt = MWD[i].energy;
          TMAX[i].tagged[k].energy_mt = TMAX[i].energy;
          NMO[i].tagged[k].energy_mt = NMO[i].energy;
          //
          RAW_CALIB[i].tagged[k].integral_mt = RAW_CALIB[i].integral;
          MA[i].tagged[k].integral_mt = MA[i].integral;
          MWD[i].tagged[k].integral_mt = MWD[i].integral;
          TMAX[i].tagged[k].integral_mt = TMAX[i].integral;
          NMO[i].tagged[k].integral_mt = NMO[i].integral;
          //
          if (RAW_CALIB[i].tagged[k].energy_mt > ENERGY_CUT) ECAL25[0].tagged[k].energy_mt += RAW_CALIB[i].tagged[k].energy_mt;
          if (MA[i].tagged[k].energy_mt > ENERGY_CUT)        ECAL25[1].tagged[k].energy_mt += MA[i].tagged[k].energy_mt;
          if (MWD[i].tagged[k].energy_mt > ENERGY_CUT)       ECAL25[2].tagged[k].energy_mt += MWD[i].tagged[k].energy_mt;
          if (TMAX[i].tagged[k].energy_mt > ENERGY_CUT)      ECAL25[3].tagged[k].energy_mt += TMAX[i].tagged[k].energy_mt;
          if (NMO[i].tagged[k].energy_mt > ENERGY_CUT)       ECAL25[4].tagged[k].energy_mt += NMO[i].tagged[k].energy_mt;
          //
          ECAL25[0].tagged[k].integral_mt += RAW_CALIB[i].tagged[k].integral_mt;
          ECAL25[1].tagged[k].integral_mt += MA[i].tagged[k].integral_mt;
          ECAL25[2].tagged[k].integral_mt += MWD[i].tagged[k].integral_mt;
          ECAL25[3].tagged[k].integral_mt += TMAX[i].tagged[k].integral_mt;
          ECAL25[4].tagged[k].integral_mt += NMO[i].tagged[k].integral_mt;

          if (i == CENTRAL && k == 0 ){//&& TMAX[i].tagged[k].energy_mt < 5000){
            // plot = 1;
          }
          if (i == CENTRAL && k == 6 ){//&& TMAX[i].tagged[k].energy_mt < 5000){
            // plot = 2;
          }

          // if (i==CENTRAL && RAW_CALIB[i].energy > 3000){
          //   for (int n = 0; n < (int)RAW_CALIB[i].trace.size(); n++){
          //     printf("%3.3f\n", RAW_CALIB[i].trace[n]);
          //   }
          // printf("++++++++++++++++++++\n");
          // }


        }
        else{
          RAW_CALIB[i].tagged[k].energy_mt = 0.0;
          MA[i].tagged[k].energy_mt = 0.0;
          MWD[i].tagged[k].energy_mt = 0.0;
          TMAX[i].tagged[k].energy_mt = 0.0;
          NMO[i].tagged[k].energy_mt = 0.0;
          //
          RAW_CALIB[i].tagged[k].integral_mt = 0.0;
          MA[i].tagged[k].integral_mt = 0.0;
          MWD[i].tagged[k].integral_mt = 0.0;
          TMAX[i].tagged[k].integral_mt = 0.0;
          NMO[i].tagged[k].integral_mt = 0.0;
        }
      }
    }
    // After all single channels were looked at, now look at the detector sum
    //
    // First for the general untagged energies
    // If central crystal has an event
    if (RAW_CALIB[CENTRAL].is_signal == 1){
      // look for the whole matrix (or submatrix)
      for (int i = 0; i < (int)RAW_CALIB.size(); i++){
        // If signal, sum up
        if (RAW_CALIB[i].is_signal == 1){
          //
          ECAL25[0].energy += RAW_CALIB[i].energy; 
          ECAL25[0].integral += RAW_CALIB[i].integral; 
          //
          for (int k = 0; k < N_E_WINDOW; k++){

          }
        }
        if (MA[i].is_signal == 1){
          ECAL25[1].energy += MA[i].energy; 
          ECAL25[1].integral += MA[i].integral; 
        }
        if (MWD[i].is_signal == 1){
          ECAL25[2].energy += MWD[i].energy; 
          ECAL25[2].integral += MWD[i].integral; 
        }
        // printf("%d %d %d\n", i, (int)RAW_CALIB.size(),(int)TMAX.size());
        if (TMAX[i].is_signal == 1){
          ECAL25[3].energy += TMAX[i].energy; 
          ECAL25[3].integral += TMAX[i].integral; 
        }
        if (NMO[i].is_signal == 1){
          ECAL25[4].energy += NMO[i].energy; 
          ECAL25[4].integral += NMO[i].integral; 
        }
      }
    }
    // If central crystal does not have a valid event, don't sum up
    else{
      for (int i = 0; i < (int)ECAL25.size(); i++){ 
        //
        ECAL25[i].energy = 0; 
        ECAL25[i].integral = 0; 
        //
        for (int k = 0; k < N_E_WINDOW; k++){
          ECAL25[i].tagged[k].energy = 0;
          ECAL25[i].tagged[k].energy_m = 0;
          ECAL25[i].tagged[k].energy_mt = 0;
          ECAL25[i].tagged[k].integral = 0;
          ECAL25[i].tagged[k].integral_m = 0;
          ECAL25[i].tagged[k].integral_mt = 0;
        }
      }
    }
    // Now enter the the sum energy of the ECAL into the tagger time vs ecal energy 2D histogram
    // Only if tagger cut and multiplicity is obeyed
    tags_per_event = 0;
    int tagger_channel = 0;
    for (int k = 0; k < N_E_WINDOW; k++){
      if ( 
        TAGGER.time[k] != 0 && // and if tagger is set for energy k
        TAGGER.time[k] >= (TAGGER.cut[k] - TAGGER_WINDOW) &&
        TAGGER.time[k] <= (TAGGER.cut[k] + TAGGER_WINDOW) // and if tagger time is in the timing window cut
      ){ 
        // Check if only one tagger channel fired
        tagger_channel = TAGGER.time[k];
        tags_per_event++;
      }
    }
    // Only fill if only one tag per event
    if ( tags_per_event == 1){
      if (ECAL25[4].energy != 0) TAGGER.h_tagger_vs_energy.hist->Fill( ECAL25[4].energy, tagger_channel);
    }

    // Also extract timing 
    if (tags_per_event == 1){ // Tagger timing cuts 
      // Calculate the zero crossings
      for (int i = 0; i < (int)NMO.size(); i++){
        if (NMO[i].is_signal == 1){
          NMO[i].CFD.int_x0 = array_zero_xing(NMO[i].CFD.trace,9000, 12000, 1);
          // printf("%3.3f\n", NMO[i].CFD.int_x0);
        } 
      }
      // Do the timing correlation
      for (int k = 0; k < N_E_WINDOW; k++){
        for (int i = 0; i < (int)NMO.size(); i++){
          for (int j = 0; j < (int)NMO.size(); j++){
            if (TAGGER.time[k] != 0 && NMO[i].is_signal == 1 && NMO[j].is_signal == 1){
              NMO[i].time[j].timing[k] = (NMO[i].CFD.int_x0 - NMO[j].CFD.int_x0) * NMO[i].sample_t;
              // printf("++++%3.1f %3.1f\n", NMO[i].time[j].timing[k], NMO[i].sample_t);
              // plot = 1;
            }
            else { NMO[i].time[j].timing[k] = 0; }
          }
        }
      }
    }

  } 

  // 0: Is valid max 
  // 1: Glitch detected 
  // 2: Saturation detected 
  // 3: Trace is not not above TH
  // 4: Baseline is weird
  // 5: Maximum not in energy range
  // 6: Outside area/signal ratio


  if (plot == 1){
    // printf("PLOTTED_1\n");
    // plot_trace(RAW_CALIB[CENTRAL].trace, "GLITCH", "JUNK", "");
    // for (int i = 0; i < (int)RAW_CALIB[7].trace.size(); i++){
    //   printf("%3.3f\n", RAW_CALIB[7].trace[i]);
    // }
    // printf("\n");
  }
  if (plot == 2){
    // printf("PLOTTED_2\n");
    // plot_trace(RAW_CALIB[CENTRAL].trace, "SATURATION", "JUNK", "");
  }
  if (plot == 4 ){
    // printf("PLOTTED_4\n");
    // plot_trace(RAW_CALIB[CENTRAL].trace, "BASELINE", "JUNK", "");
  }


  if (plot == 5 ){
    // printf("PLOTTED_5\n");
    // plot_trace(RAW_CALIB[CENTRAL].trace, "ENERGY_RANGE", "JUNK", "");
  }

  if (plot == 6 ){
    // printf("PLOTTED_6\n");
    // plot_trace(RAW_CALIB[CENTRAL].trace, "RATIO", "JUNK", "");
  }
  hfile->cd("JUNK");
  if (TMAX[CENTRAL].tagged[0].energy_mt > 6000 ){
    // printf("PLOTTED RANGE 25 - 34\n");
    // plot_trace(TMAX[CENTRAL].trace, "Energy_n_80-100", "JUNK", "");
    // plot_waves_compare("Bigger6000", "TRACE");
  }
  if (TMAX[CENTRAL].tagged[0].energy_mt < 6000 ){
    // printf("PLOTTED RANGE 25 - 34\n");
    // plot_trace(TMAX[CENTRAL].trace, "Energy_n_80-100", "JUNK", "");
    // plot_waves_compare("Bigger6000", "TRACE");
  }
  if (TMAX[CENTRAL].energy_n > 100 &&  TMAX[CENTRAL].energy_n < 120){
    // printf("PLOTTED RANGE 25 - 34\n");
    // plot_trace(TMAX[CENTRAL].trace, "Energy_n_100-120", "JUNK", "");
    // plot_waves_compare("Energy_n_100-120", "TRACE");

  }
  if (TMAX[CENTRAL].energy_n > 120 &&  TMAX[CENTRAL].energy_n < 150){
    // printf("PLOTTED RANGE 25 - 34\n");
    // plot_trace(TMAX[CENTRAL].trace, "Energy_n_120-150", "JUNK", "");
    // plot_waves_compare("Energy_n_120-150", "TRACE");
  }




  // Now print wave forms
  // print coincident waveforms
  if( (strcmp(MODE, "COSMICS") == 0 && is_coinc == true) || // either Cosmics mode or
      (strcmp(MODE, "BEAM") == 0 && is_coinc == true) || // Beam mode or
      (strcmp(MODE, "PULSER") == 0) ) { // Pulser mode
    if(NOE%1000==0){
      hfile->cd("WAVE_FORMS/");
      plot_waves_compare("FILTER_COMPARE", "TRACE");
      hfile->cd("WAVE_FORMS/RAW_CALIB");
      plot_waves(RAW_CALIB, "SIGNAL_RAW_CALIB", "TRACE");
      // Set printing folder to MA coincident waves
      hfile->cd("WAVE_FORMS/MA");
      plot_waves(MA, "SIGNAL_MA", "TRACE");
      // Set printing folder to MA coincident waves
      hfile->cd("WAVE_FORMS/MWD");
      plot_waves(MWD, "SIGNAL_MWD", "TRACE");
      // Set printing folder to MA coincident waves
      hfile->cd("WAVE_FORMS/TMAX");
      plot_waves(TMAX, "SIGNAL_TMAX", "TRACE");
      // Set printing folder to MA coincident waves
      hfile->cd("WAVE_FORMS/NMO");
      plot_waves(NMO, "SIGNAL_NMO", "TRACE");
      // Set printing folder to CFD 
      hfile->cd("WAVE_FORMS/CFD/RAW_CALIB");
      plot_waves(RAW_CALIB, "CFD_RAW_CALIB", "CFD");
      hfile->cd("WAVE_FORMS/CFD/MA");
      plot_waves(MA, "CFD_MA", "CFD");
      hfile->cd("WAVE_FORMS/CFD/MWD");
      plot_waves(MWD, "CFD_MWD", "CFD");
      hfile->cd("WAVE_FORMS/CFD/TMAX");
      plot_waves(TMAX, "CFD_TMAX", "CFD");
      hfile->cd("WAVE_FORMS/CFD/NMO");
      plot_waves(NMO, "CFD_NMO", "CFD");

      // if( (strcmp(MODE, "COSMICS") == 0 && is_coinc == true ) && ((int)MAPPING.size() > 1) ){
      //   // The interpolation
      //   hfile->cd("WAVE_FORMS/CFD/RAW_CALIB/INTERPOL");
      //   plot_interpol(RAW_CALIB[0].CFD.x_interpol, RAW_CALIB[0].CFD.y_interpol, RAW_CALIB[0].sample_t);
      //   hfile->cd("WAVE_FORMS/CFD/MA/INTERPOL");
      //   plot_interpol(MA[0].CFD.x_interpol, MA[0].CFD.y_interpol, MA[0].sample_t);
      //   hfile->cd("WAVE_FORMS/CFD/MWD/INTERPOL");
      //   plot_interpol(MWD[0].CFD.x_interpol, MWD[0].CFD.y_interpol, MWD[0].sample_t);
      //   hfile->cd("WAVE_FORMS/CFD/TMAX/INTERPOL");
      //   plot_interpol(TMAX[0].CFD.x_interpol, TMAX[0].CFD.y_interpol, TMAX[0].sample_t);
      //   hfile->cd("WAVE_FORMS/CFD/NMO/INTERPOL");
      //   plot_interpol(NMO[0].CFD.x_interpol, NMO[0].CFD.y_interpol, NMO[0].sample_t);
      // }
    }
  }
} 

/////////////////////////////////////////////////////
// MULTI SAMPLING CALIBRATION EXTRACTION
/////////////////////////////////////////////////////
// Extract information from the raw data for the inter sampling calibration
void multis_calib(){
  /////////////////////////////////////////////////////
  // FETCH RAW CONTENT FROM DATA
  /////////////////////////////////////////////////////
  reset_signal(RAW);
  // Create dummy container for fetching the signal
  int entry = 0;
  unsigned int dumm_cont[CHANNELS][TRACELEN];
  // Initialization of the boards (has to start with 1, since 0 is usually the IOL board)
  //    and fetching the ADC content
  for(int board=1; board<=BOARDS; board++){
    for(int channel=0; channel<8; channel++){
      // Convert the board channel to overall channel 
      entry = (board-1)*8 + channel;
      // Fetch the channels
      DETECTOR.get_trace(board, channel, dumm_cont[entry]);
      // Save the entries in the RAW container
      for(int n=0; n<TRACELEN; n++){
        RAW[entry].trace.push_back((double) dumm_cont[entry][n] * CALIB.multis[entry]);
      }
      // Calculate the baseline information for the RAW and RAW_Calib
      RAW[entry].base.mean = array_mean(RAW[entry].trace,0,BASELINE_CUT);
      // Already subtract the baseline from the signals
      for (int n = 0; n<TRACELEN; n++){
        RAW[entry].trace[n] -= RAW[entry].base.mean;
      }
      // Recalculate the baseline information for the RAW and RAW_Calib
      // RAW[entry].base.mean = array_mean(0,BASELINE_CUT,RAW[entry].trace);
      // Now calculate the std and TH for the baselines
      RAW[entry].base.std = array_std(RAW[entry].trace, 0, BASELINE_CUT,RAW[entry].base.mean);
      RAW[entry].base.TH = THRESHOLD_MULTIPLICY * RAW[entry].base.std; 
    }
  }
  // Number of multisampled effective channels
  int multis_nb = 0;
  // i is the software channel, including all multisampling channels. mapping from hardware channels has to be done
  for (int a = 0; a<(int)MAPPING.size(); a++){
    for (int b = 0; b<(int)MAPPING[a].size(); b++){
      // Mapped hardware channel in RAW container
      int h = MAPPING[a][b].board_nb * 8 + MAPPING[a][b].h_channel_start;
      // Now fill the software channels according to the mapping and multisampling
      if (MAPPING[a][b].multis == 1){
        // 
        RAW[h].multis = 1;
      }
      else if (MAPPING[a][b].multis == 2){
        //
        RAW[h].multis = 2;
        RAW[h].is_valid = true;
        RAW[h+1].is_valid = true;
        multis_nb++;
      }
      else if (MAPPING[a][b].multis == 4){
        //
        RAW[h].multis = 4;
        RAW[h].is_valid = true;
        RAW[h+1].is_valid = true;
        RAW[h+2].is_valid = true;
        RAW[h+3].is_valid = true;
        multis_nb++;
      }
    }
  } 

  /////////////////////////////////////////////////////
  // SAMPLE ONLY THE MULTISAMPLED SIGNALS
  /////////////////////////////////////////////////////
  int is_signal[CHANNELS];
  // Variable declaration for feature extraction
  for(int i=0; i<CHANNELS; i++){
    // only look at valid channels
    if (RAW[i].is_valid == false) {
      continue;
    }
    is_signal[i] = 1;    
    /////////////////////////////////////////////////////
    // SIGNAL REGION
    /////////////////////////////////////////////////////
    for(int n=BASELINE_CUT; n<ENERGY_WINDOW_MAX; n++){
      // Subtract the baseline
      // RAW[i].trace[n] -= RAW[i].base.mean;
      // Check for energy
      if(RAW[i].trace[n]>RAW[i].energy){RAW[i].energy=RAW[i].trace[n];}
      // Check if new value is higher than previous max value and above TH
      if(RAW[i].trace[n]>RAW[i].energy){
        // If glitch filter is not activated or trace is almost at the and of range:
        if (GLITCH_FILTER == false){// || n+GLITCH_FILTER_RANGE >= TRACELEN){
          RAW[i].energy=RAW[i].trace[n];
        }
        // If glitch filter is activated
        else{
          int is_glitch_array[GLITCH_FILTER_RANGE];
          int is_glitch = 1;
          // Check for the next few samples if all are above threshold
          for(int k=n; k<n+GLITCH_FILTER_RANGE; k++){
            // If k-th sample is greater than TH, then save 0 in array
            // Test with RAW signal because RAW_CALIB is not yet calculated
            if (RAW[i].trace[k]>RAW[i].base.TH){
              is_glitch_array[k-n] = 1;
            }
            // If not, then set it save 1 in array
            else{
              is_glitch_array[k-n] = 0;
            }
          }
          // Check of samples vary around baseline TH 
          for(int k=0; k<GLITCH_FILTER_RANGE; k++){
            is_glitch *= is_glitch_array[k]; 
          }
          // If not a glitch, save energy
          if (is_glitch==1){
            RAW[i].energy = RAW[i].trace[n];
          }
        }
      }
    }
    /////////////////////////////////////////////////////
    // REST OF SIGNAL
    /////////////////////////////////////////////////////
    for(int n=ENERGY_WINDOW_MAX; n<TRACELEN; n++){
      RAW[i].trace[n] -= RAW[i].base.mean;
    }
    /////////////////////////////////////////////////////
    // EXTRACTION OF FEATURES 
    /////////////////////////////////////////////////////
    if (RAW[i].energy < RAW[i].base.TH){ is_signal[i] = 0; }
    else { is_signal[i]++; }
  }
  /////////////////////////////////////////////////////
  // TEST FOR COINCIDENCE ONLY IN THE MULTISAMPLED CHANNELS
  /////////////////////////////////////////////////////
  // print waveforms
  if(NOE%1000==0){
    hfile->cd("WAVE_FORMS/RAW");
    plot_waves(RAW, "SIGNAL_RAW", "TRACE");
  }
  // For all events, do the calculation 
  //    An event in a multisampled channel must be in all partaking channels
  for(int i=0; i<CHANNELS; i++){
    // Check if channel takes part in multisampling
    if (RAW[i].multis == 1) continue;
    // if not do calculation
    for(int j=0; j<RAW[i].multis; j++){
      for(int k=0; k<RAW[i].multis; k++){
        if (RAW[i+k].energy == 0.0) MULTIS_NORM[i][j][k].ratio = 0.0;
        else {
          MULTIS_NORM[i][j][k].ratio = RAW[i+j].energy / RAW[i+k].energy;
        }
      }
    }
  }
}

/////////////////////////////////////////////////////
// INTERPOLATE zero crossing for given trace
/////////////////////////////////////////////////////
void interpolate(vector<signal_struct> &signal)
{
  // For time extraction, compare all channels with each other and extract the time information and store 
  //  it in the correct field
  // Outer channel loop: Interpolate each channel trace and calculate the zero crossing point
  int warning_counter = 0;
  for(int i=0; i<(int)signal.size(); i++){
    // If channel is not valid or there's no event, save zero crossing of 0.0
    if (signal[i].is_valid == false || signal[i].is_signal == 0 ){
      for (int j=0; j<(int)signal.size(); j++){
        for (Int_t k = 0; k < N_E_WINDOW; k++){
          signal[i].time[j].timing[k] =  0.0; // When filling the histograms, 0.0 is not entered in the histogram 
        }
      }
      continue;
    } 
    //
    // Else, go on
    // Reset the Xzero marker for both xtals
    signal[i].CFD.Xzero_int=0;
    // Now set the interpolation interval 
    Int_t int_left = signal[i].CFD.Xzero - round((N_INTPOL_SAMPLES*signal[i].multis)/2);
    Int_t int_right = signal[i].CFD.Xzero + round((N_INTPOL_SAMPLES*signal[i].multis)/2);
    // Check if interval is out of range
    if (int_left < 0) int_left = 0;
    if (int_right > (int)signal[i].CFD.trace.size()) int_right = (int)signal[i].CFD.trace.size();
    Int_t int_range = int_right - int_left;
    // initialize the new containers for storing the interpolated signal
    Double_t xi[int_range]; 
    Double_t yi[int_range]; 
    signal[i].CFD.x_interpol.clear();
    signal[i].CFD.y_interpol.clear();
    // Initilaize the interpolator
    ROOT::Math::Interpolator inter(int_range, ROOT::Math::Interpolation::kPOLYNOMIAL);
    // Fill arrays with data
    for ( Int_t k = 0; k < int_range; k++)
    {
      xi[k]  = (Double_t) int_left + k;  
      yi[k]  = (Double_t) signal[i].CFD.trace[xi[k]];
    }
    // Set the Data
    inter.SetData(int_range, xi, yi);
    // printf("%i %i %i %i\n", int_left, int_right, int_range, NB);
    // Be careful with the range switching from one grid to the other
    for ( Int_t k = 0; k < (Int_t)((NB) * int_range - (NB) + 1); k++ )
    {
      Double_t x_value = ((Double_t) int_left + (Double_t) k/(NB));
      signal[i].CFD.x_interpol.push_back(x_value);
      signal[i].CFD.y_interpol.push_back( (Double_t) inter.Eval(x_value));
      // printf("%3.3f %3.3f\n", signal[i].CFD.x_interpol[k], signal[i].CFD.y_interpol[k]);
      // Already look for the zero crossing point
      if (signal[i].CFD.y_interpol[k] > 0 && signal[i].CFD.Xzero_int==0){signal[i].CFD.Xzero_int=k;}
    }
    // Old method:
    // Calculate zero crossing for first trace
    // signal[i].CFD.int_m = (y[0][signal[i].CFD.Xzero_int]-y[0][signal[i].CFD.Xzero_int-1])/(x[0][signal[i].CFD.Xzero_int]-x[0][signal[i].CFD.Xzero_int-1]);
    // signal[i].CFD.int_b = y[0][signal[i].CFD.Xzero_int] - signal[i].CFD.int_m*x[0][signal[i].CFD.Xzero_int];
    // signal[i].CFD.int_x0 = - (signal[i].CFD.int_b/signal[i].CFD.int_m);
    // // Calculate zero crossing for second trace
    // signal[j].CFD.int_m = (y[1][signal[j].CFD.Xzero_int]-y[1][signal[j].CFD.Xzero_int-1])/(x[0][signal[j].CFD.Xzero_int]-x[0][signal[j].CFD.Xzero_int-1]);
    // signal[j].CFD.int_b = y[1][signal[j].CFD.Xzero_int] - signal[j].CFD.int_m*x[0][signal[j].CFD.Xzero_int];
    // signal[j].CFD.int_x0 = - (signal[j].CFD.int_b/signal[j].CFD.int_m);

    // Remove some interpolated samples close to the boundaries where the interpolation fails
    for (int k = 0; k < (NB); k++){
      signal[i].CFD.x_interpol.erase(signal[i].CFD.x_interpol.begin()); signal[i].CFD.x_interpol.erase(signal[i].CFD.x_interpol.end()-1);
      signal[i].CFD.y_interpol.erase(signal[i].CFD.y_interpol.begin()); signal[i].CFD.y_interpol.erase(signal[i].CFD.y_interpol.end()-1);
    }
    // Use fast linear regression fit to find time value
    if (!linreg(signal[i].CFD.x_interpol,signal[i].CFD.y_interpol,&signal[i].CFD.int_m,&signal[i].CFD.int_b)) {
      warning_counter++;
      if ( warning_counter < 50){
        printf("WARNING (interpolate): Linreg: Singular matrix, can't solve problem\n");
      }
      else{
        printf("WARNING (interpolate): Linreg: Further warnings suppressed.\n");
      }
    }
    // Save the timing information of the zero crossing point
    signal[i].CFD.int_x0 = - (signal[i].CFD.int_b/signal[i].CFD.int_m);

    // signal[i].CFD.int_x0 = signal[i].CFD.x_interpol[array_zero_xing(signal[i].CFD.y_interpol, 0, 1E5, -1)];
    // printf("%3.3f\n", signal[i].CFD.int_x0);
  }
}

// Function for comparing a signal with all other signal and saving the time difference of occurance
void time_compare(vector<signal_struct> &signal){
  // Now compare all the channel
  for (int i = 0; i < (int)signal.size(); i++){
    for (int j = 0; j < (int)signal.size(); j++){
      // Scan through all energy windows and extract timing
      for (Int_t k = 0; k < N_E_WINDOW; k++){
        // Check if channel is valid/is a signal
        if (signal[i].is_valid == false || signal[j].is_valid == false || signal[i].is_signal == 0 || signal[i].is_signal == 0){
          signal[i].time[j].timing[k] = 0.0;
          continue;
        }
        // If all good, proceed
        if (signal[i].energy >= (double) (E_WINDOW_LEFT+k*E_WINDOW_LENGTH) && 
            signal[i].energy < (double) (E_WINDOW_LEFT+k*E_WINDOW_LENGTH+E_WINDOW_LENGTH) && 
            signal[j].energy >= (double) (E_WINDOW_LEFT+k*E_WINDOW_LENGTH) && 
            signal[j].energy < (double) (E_WINDOW_LEFT+k*E_WINDOW_LENGTH+E_WINDOW_LENGTH)){
          // Calculate and norm time difference
          double time_diff = (signal[i].CFD.int_x0 - signal[j].CFD.int_x0) * signal[i].sample_t;
          signal[i].time[j].timing[k] = time_diff;
        }
      }
    }
  }
}



void print_final_statistics(){
  Double_t total_counts=0;
  // 
  //  GENERAL STATISTICS
  //
  for (Int_t i = 0; i<CHANNELS_EFF; i++){
    total_counts += RAW_CALIB[i].h_energy.hist->Integral();
  }
  printf("\n\n\n++++++++     FINAL STATISTICS     ++++++++\n");
  printf("+\n");
  printf("+ TOTAL COUNTS: %i\n", (Int_t) total_counts);
  // Print counts per channel in square form
  for (int a = 0; a<(int)MAPPING.size(); a++){
    printf("-    ");
    for (int b = 0; b<(int)MAPPING[a].size(); b++){
      int ch = b*(int)MAPPING.size()+a; 
      printf("%6d ", (Int_t) RAW_CALIB[ch].h_energy.hist->Integral());
    }
    printf("\n");
  }
  printf("+\n");

  //
  //  COSMICS MODE STATISTICS  
  //
  if(strcmp(MODE, "COSMICS") == 0) {
    Int_t total_coincidents=RAW_CALIB[0].h_energy.hist->Integral();
    Double_t total_efficiency = total_coincidents/(total_counts/CHANNELS_EFF);
    printf("+ TOTAL COINCIDENCES: %i\n", (Int_t)total_coincidents);
    printf("-    Coincidence efficiency %i/%i: %3.1f%%\n", 
          (Int_t)total_coincidents, 
          (Int_t)(total_counts/CHANNELS_EFF), 
          total_efficiency*100);
    printf("-\n");
    if (total_coincidents==0){
      printf("\n\nCoincidence error, check code\n\n");
    }
    // Do the langaus fits for the energy histograms
    for (Int_t i = 0; i < (int)RAW_CALIB.size(); i++){
      if (RAW_CALIB[i].is_valid == false) continue;
      // Scale histograms to MeV range
      vector<double> lin_scaling; lin_scaling.push_back(0.01); lin_scaling.push_back(0.00);
      ScaleXaxis(RAW_CALIB[i].h_energy.hist, ScaleX, lin_scaling, "linear");
      ScaleXaxis(MA[i].h_energy.hist, ScaleX, lin_scaling, "linear");
      ScaleXaxis(MWD[i].h_energy.hist, ScaleX, lin_scaling, "linear");
      ScaleXaxis(TMAX[i].h_energy.hist, ScaleX, lin_scaling, "linear");
      // fit the energy histograms
      RAW_CALIB[i].h_energy.params = fit_hist(RAW_CALIB[i].h_energy.hist, RAW_CALIB[i].h_energy.fit, "langaus");
      MA[i].h_energy.params = fit_hist(MA[i].h_energy.hist, MA[i].h_energy.fit, "langaus");
      MWD[i].h_energy.params = fit_hist(MWD[i].h_energy.hist, MWD[i].h_energy.fit, "langaus");
      TMAX[i].h_energy.params = fit_hist(TMAX[i].h_energy.hist, TMAX[i].h_energy.fit, "langaus");
      // fit_hist(TMAX[i].h_energy.hist, TMAX[i].h_energy.fit, "langaus_roofit");
      // NMO[i].h_energy.params = fit_hist(NMO[i].h_energy.hist, NMO[i].h_energy.fit, "langaus");
      // Fill the calibration parameters into the histogram
      if ( RAW_CALIB[i].h_energy.params[8] != 0 ){
        // Fill hist
        CALIB.h_RAW_energy.hist->Fill((ENERGY_NORM/RAW_CALIB[i].h_energy.params[8])*CALIB.RAW_energy[i]);
      }
      if ( TMAX[i].h_energy.params[8] != 0 ){
        CALIB.h_TMAX_energy.hist->Fill((ENERGY_NORM/TMAX[i].h_energy.params[8])*(CALIB.TMAX_energy[i]*CALIB.RAW_energy[i]));
      }
      // Some runtime information
      if (is_in_string(VERBOSE,"p")){
        printf("+ Energy Fit for channel %d done.\n", i);
      }
    }    
    // Save the calibration parameter to a file
    *energy_out << "RAW_CALIB_PARAMETERS" << endl;
    for (int i = 0; i < (int)RAW_CALIB.size(); i++){
      *energy_out << "Channel_" << i << ";";
      for (int t = 0; t < (int)RAW_CALIB[i].h_energy.params.size(); t++){
        *energy_out << fixed << setprecision(2) << RAW_CALIB[i].h_energy.params[t] << ";";
      }
      //Also add the calibration parameter
      *energy_out << fixed << setprecision(2) << (ENERGY_NORM/RAW_CALIB[i].h_energy.params[8])*CALIB.RAW_energy[i];
      *energy_out << endl;
    }
    *energy_out << endl;
    *energy_out << "TMAX_PARAMETERS" << endl;
    for (int i = 0; i < (int)TMAX.size(); i++){
      *energy_out << "Channel_" << i << ";";
      for (int t = 0; t < (int)TMAX[i].h_energy.params.size(); t++){
        *energy_out << fixed << setprecision(2) << TMAX[i].h_energy.params[t] << ";";
      }
      //Also add the calibration parameter
      *energy_out << fixed << setprecision(2) << (ENERGY_NORM/TMAX[i].h_energy.params[8])*(CALIB.TMAX_energy[i]*CALIB.RAW_energy[i]);
      *energy_out << endl;
    }
    energy_out->close();
    // Print Energy extraction 
    if (is_in_string(VERBOSE,"e")){
      // Statistics for the fit
      // print_energy_statistics(RAW_CALIB, "RAW_CALIB");
      // print_energy_statistics(MA, "MA");
      // print_energy_statistics(MWD, "MWD");
      // print_energy_statistics(TMAX, "TMAX");
      // print_energy_statistics(NMO, "NMO");
      // Resulting calibration parameters
      print_energy_calib();
    }

    //////// TIMING (if there is more than one channel)

    // if (CHANNELS_EFF > 1){
    //   // Start extracting information for all windows
    //   // Do the fitting for the histograms and plotting
    //   plot_timing_hist(RAW_CALIB, "TIMING/RAW_CALIB");
    //   if (is_in_string(VERBOSE,"p")) printf("+ Fitting time hists finished (RAW_CALIB)\n");
    //   plot_timing_hist(MA, "TIMING/MA");
    //   if (is_in_string(VERBOSE,"p")) printf("+ Fitting time hists finished (MA)\n");
    //   plot_timing_hist(MWD, "TIMING/MWD");
    //   if (is_in_string(VERBOSE,"p")) printf("+ Fitting time hists finished (MWD)\n");
    //   plot_timing_hist(TMAX, "TIMING/TMAX");
    //   if (is_in_string(VERBOSE,"p")) printf("+ Fitting time hists finished (TMAX)\n");

    //   // Plot the time vs energy graph
    //   hfile->cd("TIMING/RAW_CALIB");
    //   plot_time_energy(RAW_CALIB[0].time[1]);
    //   hfile->cd("TIMING/MA");
    //   plot_time_energy(MA[0].time[1]); 
    //   hfile->cd("TIMING/MWD");
    //   plot_time_energy(MWD[0].time[1]); 
    //   hfile->cd("TIMING/TMAX");
    //   plot_time_energy(TMAX[0].time[1]); 


    //   // Print out the timing for a filter type
    //   if (is_in_string(VERBOSE,"t")){
    //     print_timing_statistics(RAW_CALIB[0].time[1], total_coincidents, "RAW_CALIB");
    //     print_timing_statistics(MA[0].time[1], total_coincidents, "MA");
    //     print_timing_statistics(MWD[0].time[1], total_coincidents, "MWD");
    //     print_timing_statistics(TMAX[0].time[1], total_coincidents, "TMAX");
    //   }
    // }
  }
  //
  //  PULSER MODE STATISTICS  
  //
  if(strcmp(MODE, "PULSER") == 0){
    for (int i = 0; i < (int)RAW_CALIB.size(); i++){
      fit_hist(RAW_CALIB[i].h_energy.hist, RAW_CALIB[i].h_energy.fit, "multigaus");  
      fit_hist(MA[i].h_energy.hist, MA[i].h_energy.fit, "multigaus");  
      fit_hist(MWD[i].h_energy.hist, MWD[i].h_energy.fit, "multigaus");  
      fit_hist(TMAX[i].h_energy.hist, TMAX[i].h_energy.fit, "multigaus");  
      fit_hist(NMO[i].h_energy.hist, NMO[i].h_energy.fit, "multigaus");  
    }
  }

  //
  //  BEAM MODE STATISTICS
  //
  if(strcmp(MODE, "BEAM") == 0) {
  //   // Signal Filter statistics 
  //   printf("+ SIGNAL FILTER STATISTICS\n");
  //   printf("- Valid maxia   : %5d\n", FILTER_EFFICIENCY[0]);
  //   printf("- Glitches      : %5d\n", FILTER_EFFICIENCY[1]);
  //   printf("- Saturation    : %5d\n", FILTER_EFFICIENCY[2]);
  //   printf("- Below TH      : %5d\n", FILTER_EFFICIENCY[3]);
  //   printf("- Baseline weird: %5d\n", FILTER_EFFICIENCY[4]);
  //   printf("- Max o.f. range: %5d\n", FILTER_EFFICIENCY[5]);
  //   printf("- Ratio is off  : %5d\n", FILTER_EFFICIENCY[6]);
  //   // Tagger energy statistics if verbose is set
  //   int sum = 0;
  //   if (is_in_string(VERBOSE, "g")){
  //     printf("+++ Tagger Energy statistics +++\n");
  //     printf("+Tagger #: total counts (multiple couts) (counts w/o multiples and w/ timing cut)\n");
  //     for (int k = 0; k < TAG_CHANNELS; k++){
  //       printf("+ Tagger %2d: %d (%d) (%d)\n", k, TAGGER.counts[k], TAGGER.multiples_per_channel[k], (Int_t) RAW_CALIB[CENTRAL].tagged[k].h_energy_mt.hist->Integral());
  //       sum += TAGGER.counts[k];
  //     }
  //     printf("+\n+ Sum of all tagged channels: %d (includes multiples)\n", sum);
  //     printf("+\n");
  //     // Tagger multiplicity statistics
  //     printf("+++ Multiple tagger counts +++\n");
  //     for (int i = 0; i < TAG_CHANNELS; i++){
  //       if (TAGGER.multiples_per_count[i] != 0) printf("+ %d events per tag: %2d\n", i, TAGGER.multiples_per_count[i]);
  //     }
  //     printf("+\n");
  //     for (int k = 0; k < TAG_CHANNELS; k++){
  //       printf("+ TAGGER ENERGY %2d:\n", k);
  //       for (int a = 0; a<(int)MAPPING.size(); a++){
  //         printf("-    ");
  //         for (int b = 0; b<(int)MAPPING[a].size(); b++){
  //           int ch = b*(int)MAPPING.size()+a; 
  //           printf("%6d ", (Int_t) RAW_CALIB[ch].tagged[k].h_energy.hist->Integral());
  //         }
  //         printf("\n");
  //       }
  //     }
  //     printf("+\n");
  //     // Tagger timing histogram: with with most entries for cutting decision
  //     for (int k = 0; k < TAG_CHANNELS; k++){
  //       printf("TAGGER_CUT%02d=%i#\n", k, largest_1Dbin(TAGGER.t_hist[k].hist,-5000,5000));  
  //     }
  //   }
  //   // Calculate the energy hist of the ring around the central crystal
  //   for (int k = 0; k < (int)ECAL25[4].tagged.size(); k++){
  //     for (int i = 0; i < (int)NMO.size(); i++){
  //       if ( i == CENTRAL ) continue;
  //       ECAL25[3].tagged[k].h_energy_mt_ring.hist->Add(TMAX[i].tagged[k].h_energy_mt.hist);
  //     }
  //   }

  //   /// Scale all relevant histograms
  //   vector<double> tagged_energy;
  //   vector<double> tagged_energy_err;
  //   vector<double> tagged_energy_err_err;
  //   vector<double> lin_scaling; lin_scaling.push_back(0.01); lin_scaling.push_back(0.00);

  //   for (int i = 0; i < 5; i++){
  //     ScaleXaxis(ECAL25[i].h_energy.hist, ScaleX, lin_scaling, "linear");
  //     for (int k = 0; k < N_E_WINDOW; k++){
  //       ScaleXaxis(ECAL25[i].tagged[k].h_energy.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(ECAL25[i].tagged[k].h_energy_m.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(ECAL25[i].tagged[k].h_energy_mt.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(ECAL25[i].tagged[k].h_energy_mt_ring.hist, ScaleX, lin_scaling, "linear");
  //     }
  //   }
  //   for (int i = 0; i < (int)RAW_CALIB.size(); i++){
  //     ScaleXaxis(RAW_CALIB[i].h_energy.hist, ScaleX, lin_scaling, "linear");
  //     ScaleXaxis(MA[i].h_energy.hist, ScaleX, lin_scaling, "linear");
  //     ScaleXaxis(MWD[i].h_energy.hist, ScaleX, lin_scaling, "linear");
  //     ScaleXaxis(TMAX[i].h_energy.hist, ScaleX, lin_scaling, "linear");
  //     ScaleXaxis(NMO[i].h_energy.hist, ScaleX, lin_scaling, "linear");
  //     for (int k = 0; k < N_E_WINDOW; k++){
  //       ScaleXaxis(RAW_CALIB[i].tagged[k].h_energy.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(RAW_CALIB[i].tagged[k].h_energy_m.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(RAW_CALIB[i].tagged[k].h_energy_mt.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(MA[i].tagged[k].h_energy.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(MA[i].tagged[k].h_energy_m.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(MA[i].tagged[k].h_energy_mt.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(MWD[i].tagged[k].h_energy.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(MWD[i].tagged[k].h_energy_m.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(MWD[i].tagged[k].h_energy_mt.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(TMAX[i].tagged[k].h_energy.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(TMAX[i].tagged[k].h_energy_m.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(TMAX[i].tagged[k].h_energy_mt.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(NMO[i].tagged[k].h_energy.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(NMO[i].tagged[k].h_energy_m.hist, ScaleX, lin_scaling, "linear");
  //       ScaleXaxis(NMO[i].tagged[k].h_energy_mt.hist, ScaleX, lin_scaling, "linear");
  //     }
  //   }

  //   // Print the split screen tagger energy histograms
  //   plot_tagger_hist(RAW_CALIB, "ENERGY/TAGGER/PULSE_HIGHT/RAW_CALIB", "pulseheight");
  //   plot_tagger_hist(MA, "ENERGY/TAGGER/PULSE_HIGHT/MA", "pulseheight");
  //   plot_tagger_hist(MWD, "ENERGY/TAGGER/PULSE_HIGHT/MWD", "pulseheight");
  //   plot_tagger_hist(TMAX, "ENERGY/TAGGER/PULSE_HIGHT/TMAX", "pulseheight");
  //   plot_tagger_hist(NMO, "ENERGY/TAGGER/PULSE_HIGHT/NMO", "pulseheight");
  //   // Print the split screen tagger energy histograms
  //   plot_tagger_hist(RAW_CALIB, "ENERGY/TAGGER/INTEGRAL/RAW_CALIB", "integral");
  //   plot_tagger_hist(MA, "ENERGY/TAGGER/INTEGRAL/MA", "integral");
  //   plot_tagger_hist(MWD, "ENERGY/TAGGER/INTEGRAL/MWD", "integral");
  //   plot_tagger_hist(TMAX, "ENERGY/TAGGER/INTEGRAL/TMAX", "integral");
  //   plot_tagger_hist(NMO, "ENERGY/TAGGER/INTEGRAL/NMO", "integral");

  //   for (int i = 3; i < 4; i++){
  //   // for (int i = 0; i < (int)ECAL25.size(); i++){
  //     for (int k = 0; k < N_E_WINDOW; k++){
  //       // if (k<3) ECAL25[i].tagged[k].h_energy_mt.hist->Rebin(50);
  //       // if (k>=3) ECAL25[i].tagged[k].h_energy_mt.hist->Rebin(50);

  //       // if (k<3) ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "gaus", 0, 50,  0); 
  //       // if (k==3) ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "gaus", 10, 80,  0); 
  //       // if (k==4) ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "gaus", 10, 80,  0); 
  //       // if (k>4) ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "gaus", 0, 800,  0); 
  //       if (LIN_COMP == 0) {
  //         ECAL25[i].tagged[k].h_energy_mt.hist->Rebin(50);

  //         // if (k<3) ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "gaus", 0, 50,  0); 
  //         // if (k==3) ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "gaus", 10, 80,  0); 
  //         // if (k==4) ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "gaus", 10, 80,  0); 
  //         // if (k>4 && k<10) ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "gaus", 0, 800,  0); 
  //         // if (k==10) ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "gaus", 200, 350,  0);           
  //         // if (k>10) ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "gaus", 250, 350,  0); 

  //         ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "gaus", 0, 200,  0); 
  //         tagged_energy.push_back(ECAL25[i].tagged[k].h_energy_mt.params[2]);
  //         tagged_energy_err.push_back(ECAL25[i].tagged[k].h_energy_mt.params[4]);
  //         tagged_energy_err_err.push_back(ECAL25[i].tagged[k].h_energy_mt.params[5]);
  //       }
  //       if (LIN_COMP != 0){
  //         // if (k<3) {
  //         //   ECAL25[i].tagged[k].h_energy_mt.hist->Rebin(50);
  //         //   ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "gaus", 0, 50,  0); 
  //         //   tagged_energy.push_back(ECAL25[i].tagged[k].h_energy_mt.params[2]);
  //         //   tagged_energy_err.push_back(ECAL25[i].tagged[k].h_energy_mt.params[4]);
  //         //   tagged_energy_err_err.push_back(ECAL25[i].tagged[k].h_energy_mt.params[5]);
  //         // }
  //         // if (k==3 || k==4) {
  //         //   ECAL25[i].tagged[k].h_energy_mt.hist->Rebin(50);
  //         //   ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "gaus", 10, 100,  0); 
  //         //   tagged_energy.push_back(ECAL25[i].tagged[k].h_energy_mt.params[2]);
  //         //   tagged_energy_err.push_back(ECAL25[i].tagged[k].h_energy_mt.params[4]);
  //         //   tagged_energy_err_err.push_back(ECAL25[i].tagged[k].h_energy_mt.params[5]);
  //         // }
  //         // if (k==6 ){
  //         //   ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "lingaus", 100, 300,  0); 
  //         //   tagged_energy.push_back(ECAL25[i].tagged[k].h_energy_mt.params[8]);
  //         //   tagged_energy_err.push_back(ECAL25[i].tagged[k].h_energy_mt.params[9]);
  //         //   tagged_energy_err_err.push_back(sqrt(ECAL25[i].tagged[k].h_energy_mt.params[9]));
  //         // } 
  //         // if (k == 7){
  //         //   ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "lingaus", 120, 180,  0); 
  //         //   tagged_energy.push_back(ECAL25[i].tagged[k].h_energy_mt.params[8]);
  //         //   tagged_energy_err.push_back(ECAL25[i].tagged[k].h_energy_mt.params[9]);
  //         //   tagged_energy_err_err.push_back(sqrt(ECAL25[i].tagged[k].h_energy_mt.params[9]));
  //         // } 
  //         // if (k==8){
  //         //   ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "lingaus", 140, 300,  0); 
  //         //   tagged_energy.push_back(ECAL25[i].tagged[k].h_energy_mt.params[8]);
  //         //   tagged_energy_err.push_back(ECAL25[i].tagged[k].h_energy_mt.params[9]);
  //         //   tagged_energy_err_err.push_back(sqrt(ECAL25[i].tagged[k].h_energy_mt.params[9]));          
  //         // } 
  //         // if (k==9){
  //         //   ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "lingaus", 180, 800,  0); 
  //         //   tagged_energy.push_back(ECAL25[i].tagged[k].h_energy_mt.params[8]);
  //         //   tagged_energy_err.push_back(ECAL25[i].tagged[k].h_energy_mt.params[9]);
  //         //   tagged_energy_err_err.push_back(sqrt(ECAL25[i].tagged[k].h_energy_mt.params[9]));          
  //         // } 
  //         // if (k>9){
  //         //   ECAL25[i].tagged[k].h_energy_mt.hist->Rebin(50);
  //         //   ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "gaus", 300, 650,  0); 
  //         //   tagged_energy.push_back(ECAL25[i].tagged[k].h_energy_mt.params[2]);
  //         //   tagged_energy_err.push_back(ECAL25[i].tagged[k].h_energy_mt.params[4]);
  //         //   tagged_energy_err_err.push_back(sqrt(ECAL25[i].tagged[k].h_energy_mt.params[5]));
  //         // }           
  //         ECAL25[i].tagged[k].h_energy_mt.params = fit_hist(ECAL25[i].tagged[k].h_energy_mt.hist, ECAL25[i].tagged[k].h_energy_mt.fit, "lingaus", 0, 800,  1); 
  //         tagged_energy.push_back(ECAL25[i].tagged[k].h_energy_mt.params[8]);
  //         tagged_energy_err.push_back(ECAL25[i].tagged[k].h_energy_mt.params[9]);
  //         tagged_energy_err_err.push_back(sqrt(ECAL25[i].tagged[k].h_energy_mt.params[9]));
  //       }
  //     }
  //   }
  //   // Plot the tagger time versus ECAL energy 
  //   plot_TH2D_hist(TAGGER.h_tagger_vs_energy.hist, "JUNK", "Tagger_time_vs_energy");
  //   // Plot ECAL energy vs tagger energy
  //   plot_energy_vs_tagged(tagged_energy, tagged_energy_err, "JUNK", "energy_vs_tagged");
  //   // Plot ECAL energy resolution
  //   plot_energy_resolution(tagged_energy_err, tagged_energy_err_err, "JUNK", "energy_resolution");
  //   // Plot bar chart for the tagger counting frequency
  //   vector<double> tagger_frequencies;
  //   vector<double> tagger_energies;
  //   for (int k = 0; k<16; k++){
  //     tagger_frequencies.push_back( TAGGER.counts[k] );
  //     tagger_energies.push_back( TAGGER.energy[k] );
  //     printf("%d\n", TAGGER.counts[k]);
  //     TAGGER.h_stats.hist->SetBinContent(round(TAGGER.energy[k]),TAGGER.counts[k]);
  //   }
  //   // plot_trace(tagger_frequencies, "Tagger Frequency", "ENERGY/TAGGER/",  "AB");
  //   // plot_1D_xy(tagger_energies, tagger_frequencies, "Tagger Frequency", "ENERGY/TAGGER/",  "AB");

    // Timin statistics
    for (int i = 0; i < (int)NMO.size(); i ++){
      for (int j = 0; j < (int)NMO[i].time.size(); j ++){
        for (int k = 0; k < (int)NMO[i].time[j].h_timing.size(); k++){
          // NMO[i].time[j].h_timing[k].hist->Rebin(10);
          NMO[i].time[j].h_timing[k].params = fit_hist(NMO[i].time[j].h_timing[k].hist, NMO[i].time[j].h_timing[k].fit, "gaus", -50, 50, 0);
        }
      }
    }
    hfile->cd("JUNK");
    plot_time_energy(NMO[0].time[1]);

  }

  // Extract TRACELEN 1D hists out of the 2D hist
  char path[100];
  if (EXTRACT_PROTO == 1){
    for (int i = 0; i < (int)RAW_CALIB.size(); i++){
      sprintf(path,"WAVE_FORMS/RAW_CALIB/PROTO_TRACE_%02d", i);
      hfile->cd(path);
      double largest = 0;
      int min = 0;
      int max = 0;
      int nbins = 0;
      for (int n = 1; n <= (int)RAW_CALIB[i].trace.size(); n++){
        sprintf(path, "SLICE%2d", n);
        RAW_CALIB[i].TH1D_proto_trace[n].hist = RAW_CALIB[i].TH2D_proto_trace.hist->ProjectionY(path, n,n);
        RAW_CALIB[i].TH1D_proto_trace[n].hist->Rebin(1);
        // Search for the largest bin
        largest = largest_1Dbin( RAW_CALIB[i].TH1D_proto_trace[n].hist, -5000, 10000);
        // Recalculate bin to normal grid
        min = RAW_CALIB[i].TH1D_proto_trace[n].hist->GetXaxis()->GetXmin();
        max = RAW_CALIB[i].TH1D_proto_trace[n].hist->GetXaxis()->GetXmax();
        nbins = RAW_CALIB[i].TH1D_proto_trace[n].hist->GetSize()-2;
        largest = min + ((max+abs(min))/nbins * largest);
        RAW_CALIB[i].proto_trace_maxbin.push_back(largest);
        // Fit gauss aroundthe largest value
        if ( 110 < n && n < 130) RAW_CALIB[i].TH1D_proto_trace[n].params = fit_hist(RAW_CALIB[i].TH1D_proto_trace[n].hist, RAW_CALIB[i].TH1D_proto_trace[n].fit, "gaus", largest-300,largest+300,0);
        else RAW_CALIB[i].TH1D_proto_trace[n].params = fit_hist(RAW_CALIB[i].TH1D_proto_trace[n].hist, RAW_CALIB[i].TH1D_proto_trace[n].fit, "gaus", largest-300,largest+100,0);
        RAW_CALIB[i].proto_trace_fit.push_back(RAW_CALIB[i].TH1D_proto_trace[n].params[2]);
      }
      plot_trace(RAW_CALIB[i].proto_trace, "PROTO_TRACE", path, "AL*");
      plot_trace(RAW_CALIB[i].proto_trace_maxbin, "PROTO_TRACE_MAXBIN", path, "AL*");
      plot_trace(RAW_CALIB[i].proto_trace_fit, "PROTO_TRACE_FIT", path, "AL*");

      sprintf(path,"WAVE_FORMS/RAW_CALIB/PROTO_TRACE_%02d", i);
      plot_TH2D_hist_graph(RAW_CALIB[i].TH2D_proto_trace.hist, RAW_CALIB[i].proto_trace_maxbin, path, "PROTO_TRACE_2D_MAXBIN");
      plot_TH2D_hist_graph(RAW_CALIB[i].TH2D_proto_trace.hist, RAW_CALIB[i].proto_trace_fit, path, "PROTO_TRACE_2D_FIT");
      // adjust trace for baseline offset
      double mean = array_mean(RAW_CALIB[i].proto_trace_fit, 0, BASELINE_CUT);
      for (int n = 0; n < (int)RAW_CALIB[i].proto_trace_fit.size(); n++ ){
        RAW_CALIB[i].proto_trace_fit[n] -= mean;  
      }
      plot_trace(RAW_CALIB[i].proto_trace_fit, "PROTO_TRACE_FIT_ADJUSTED", path, "AL*");
    }
    // Extract proto trace to text file for later reading 
    if (proto_out->is_open()){
      for (int n = 0; n < (int)RAW_CALIB[0].proto_trace_fit.size(); n++){
        for (int i = 0; i < (int)RAW_CALIB.size(); i++){
          *proto_out << RAW_CALIB[i].proto_trace_fit[n] << ";"; 
        }
        *proto_out << endl;
      }
      proto_out->close();
    }
    else printf("WARNING(statistics): Unable to write proto trace file.\n");

  }
  if (EXTRACT_PROTO == 0){
    for (int i = 0; i < (int)RAW_CALIB.size(); i++){
      RAW_CALIB[i].proto_trace_fit = PROTO[i].proto_trace_fit;
      plot_trace(PROTO[i].proto_trace_fit, "PROTO_TRACE_FIT", path, "AL*");
      sprintf(path,"WAVE_FORMS/RAW_CALIB/PROTO_TRACE_%02d", i);
      plot_TH2D_hist_graph(RAW_CALIB[i].TH2D_proto_trace.hist, PROTO[i].proto_trace_fit,path, "PROTO_TRACE_2D_FIT");
    }
  }


  //

  //
  //  Print the split screen histograms  
  //
  // // plot_energy_hist(RAW, "ENERGY/PULSE_HIGHT/RAW", "pulseheight");
  // plot_energy_hist(RAW_CALIB, "ENERGY/PULSE_HIGHT/RAW_CALIB", "pulseheight");
  // plot_energy_hist(MA, "ENERGY/PULSE_HIGHT/MA", "pulseheight");
  // plot_energy_hist(MWD, "ENERGY/PULSE_HIGHT/MWD", "pulseheight");
  // plot_energy_hist(TMAX, "ENERGY/PULSE_HIGHT/TMAX", "pulseheight");
  // // plot_energy_hist(NMO, "ENERGY/PULSE_HIGHT/NMO", "pulseheight");
  // //
  // // plot_energy_hist(RAW, "ENERGY/INTEGRAL/RAW", "integral");
  // plot_energy_hist(RAW_CALIB, "ENERGY/INTEGRAL/RAW_CALIB", "integral");
  // plot_energy_hist(MA, "ENERGY/INTEGRAL/MA", "integral");
  // plot_energy_hist(MWD, "ENERGY/INTEGRAL/MWD", "integral");
  // plot_energy_hist(TMAX, "ENERGY/INTEGRAL/TMAX", "integral");
  // // plot_energy_hist(NMO, "ENERGY/INTEGRAL/NMO", "integral");
  
  printf("+ END OF STATISTICS +\n");

} 

void print_energy_statistics(vector<signal_struct> &array, const char *name){
  // Print Energy extraction 
  printf("+ COSMIC ENERGY DISTRIBUTIONS FOR %s\n", name);
  for (int i = 0; i < (int)array.size(); i++){
    if (array[i].is_valid == false) continue;
    // Check if histograms/parameters are emtpty are emtpy 
    printf("-    Channel %i:\n", i);
    if (array[i].h_energy.hist->Integral() == 0){
      printf("-       Histogram empty!\n");
      continue;
    } 
    else{
      printf("-       Pos : %4.1f\n", array[i].h_energy.params[8]);
      printf("-       FWHM: %4.1f\n", array[i].h_energy.params[9]);
      printf("-       Calib_param: %4.3f\n", ENERGY_NORM/array[i].h_energy.params[8]);
      printf("-\n"); 
    }
  }
  printf("-\n");
}

void print_energy_calib(){
  for (int i = 0; i < (int)RAW_CALIB.size(); i++){
    if (RAW_CALIB[i].is_valid == false){
      printf("ENERGY_CALIB%02d=1.000,1.000,1.000,1.000 (invalid channel)\n", i);
    }
    else {
      if (i == 0) printf("+ Absolut calibration parameters\n");
      printf("ENERGY_CALIB%02d=%3.3f,%3.3f,%3.3f,%3.3f,%3.3f\n", 
      i,
      (ENERGY_NORM/RAW_CALIB[i].h_energy.params[8])*CALIB.RAW_energy[i],
      (ENERGY_NORM/MA[i].h_energy.params[8])*(CALIB.MA_energy[i]*CALIB.RAW_energy[i]),
      (ENERGY_NORM/MWD[i].h_energy.params[8])*(CALIB.MWD_energy[i]*CALIB.RAW_energy[i]),
      (ENERGY_NORM/TMAX[i].h_energy.params[8])*(CALIB.TMAX_energy[i]*CALIB.RAW_energy[i]),
      (ENERGY_NORM/NMO[i].h_energy.params[8])*(CALIB.NMO_energy[i]*CALIB.RAW_energy[i])
      );
    }
  }
  printf("+\n");
  for (int i = 0; i < (int)RAW_CALIB.size(); i++){
    if (RAW_CALIB[i].is_valid == false){
      printf("ENERGY_CALIB%02d=1.000,1.000,1.000,1.000 (invalid channel)\n", i);
    }
    else{
      if (i == 0) printf("+ Relative calibration parameters\n");
      printf("ENERGY_CALIB%02d=%3.3f,%3.3f,%3.3f,%3.3f,%3.3f\n", 
      i,
      (ENERGY_NORM/RAW_CALIB[i].h_energy.params[8]),
      (ENERGY_NORM/MA[i].h_energy.params[8]),
      (ENERGY_NORM/MWD[i].h_energy.params[8]),
      (ENERGY_NORM/TMAX[i].h_energy.params[8]),
      (ENERGY_NORM/NMO[i].h_energy.params[8])
      );
    }
  }
  printf("+\n");
  for (int i = 0; i < (int)RAW_CALIB.size(); i++){
    if (RAW_CALIB[i].is_valid == false){
      printf("ENERGY_CALIB%02d=1.000,1.000,1.000,1.000 (invalid channel)\n", i);
    }
    else{
      if (i == 0) printf("+ Multiplication calibration factors\n");
      printf("ENERGY_CALIB%02d=%3.3f,%3.3f,%3.3f,%3.3f,%3.3f\n", 
      i,
      CALIB.RAW_energy[i],
      CALIB.MA_energy[i],
      CALIB.MWD_energy[i],
      CALIB.TMAX_energy[i],
      CALIB.NMO_energy[i]
      );
    }
  }
  printf("+\n");
}



// Function for printing timing fit information from a time_struct element
void print_timing_statistics(time_struct &array, Int_t total_coincidents, const char *name){
  // Print out the timing for a filter type
  Int_t e_matching_efficiency = 0;
  printf("+ TIMING FOR %s FILTER\n", name);
  for (Int_t k = 0; k < N_E_WINDOW; k++){
    printf("-   TIMING ENERGY WINDOW %i-%i : %3.3f+-%3.3f (%i ENTRIES)\n", 
      (int) E_WINDOW_LEFT + E_WINDOW_LENGTH*k, 
      (int) E_WINDOW_LEFT + E_WINDOW_LENGTH*k +E_WINDOW_LENGTH, 
      array.h_timing[k].params[4], 
      array.h_timing[k].params[5], 
      (int) array.h_timing[k].hist->Integral());
    e_matching_efficiency += (Int_t) array.h_timing[k].hist->Integral();
  }
  printf("-\n");
  printf("-   Timing/Energy matching efficiency: %i/%i: %3.1f%%\n", 
        e_matching_efficiency, 
        total_coincidents, 
        (Double_t)e_matching_efficiency/total_coincidents * 100);
  printf("+\n");
}

void print_stat_multis_calib(){
  Double_t total_counts=0;
  for (int i = 0; i<CHANNELS; i++){
    total_counts += RAW[i].h_energy.hist->Integral();
  }
  printf("\n\n\n++++++++     FINAL STATISTICS     ++++++++\n");
  printf("++++  MULTISAMPLING NORM MODE     ++++++++\n");
  printf("+\n");
  printf("+ TOTAL COUNTS (COINC): %i\n", (Int_t) total_counts);
  for (Int_t i = 0; i<CHANNELS; i++){
    printf("-    Channel %i: %i\n", i, (Int_t) RAW[i].h_energy.hist->Integral());
  }
  printf("+\n");
  printf("+ CALIBRATION RATIOS\n");
  // Do the fitting for the calibration matrices
  for (int i = 0; i<CHANNELS; i++){
    // Only print the multi sampled channels
    if (RAW[i].multis == 1) continue;
    // Else, continue
    // Calculate the real channel from the hardware channel
    //  
    printf("-   Effective Channel %i\n", i);
    vector<vector<vector<double> > > params;
    for (int j = 0; j<RAW[i].multis; j++){
      vector<vector<double> > dim2;
      for (int k = 0; k<RAW[i].multis; k++){
        dim2.push_back(fit_hist(MULTIS_NORM[i][j][k].h_hist.hist, MULTIS_NORM[i][j][k].h_hist.fit, "gaus", 0.,3.));
      }
      params.push_back(dim2);
    }
    // Now print the results
    for (int j = 0; j<RAW[i].multis; j++){
      for (int k = 0; k<RAW[i].multis; k++){
        printf("%f ", params[j][k][2]);
      }
      printf("\n");
    }
    printf("\n");
  }

  printf("+\n");
  printf("+++++++++++++++++++++++++++++++++++++++++\n");
}