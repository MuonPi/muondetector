//
//	SHOWER SIMULATION SOFTWARE 
//
//  GLOBALS File
//
//  Written by: 
//		Lukas Nies <Lukas.Nies@cern.ch> 
//		+

// Verbose level
int VERBOSE = 1;
// Number of events to be simulated
int NEVE = 3600; 
// Number of detector stations to be simulated
	// Will generate NDET of output files
int NDET = 2; 
// Detector count rate
double RATE_MEAN = 1.5; // in Hertz 
double RATE_VAR = 1.0; // in Hertz 
// Loading screen
unsigned long int oldBalken = 0;