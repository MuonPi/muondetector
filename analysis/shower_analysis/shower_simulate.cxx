//
//	SHOWER SIMULATION SOFTWARE 
//
//  MAIN file
//
//  Written by: 
//		Lukas Nies <Lukas.Nies@cern.ch> 
//		in parts by <Marvin.Peter@physik.uni-giessen.de>
//
// Version 0.1
// 03.11.2019
// 

// Compile with `g++ -Wall -g -pthread -std=c++11 shower_simulate.cxx -o shower_simulate`

// How it works:
/*
		For NEVE seconds, detector events in NDET detectors are simulated. Starting from the current point of time, t_start, 
		for each second and detector a count rate is drawn from a Gaussian distribution with RATE_MEAN and RATE_VAR.
		For this second and detector, a certain number of events is uniformly generated and sorted timewise. The 
		output is written to NDET different .txt file in plain text format. The ouput is as follows:
		 		[rising] [falling] [accEst] [ID] [valid] [timebase] [utc]
*/

// Namespaces

using namespace std;

// Include shower_simulation-specific headers

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <iomanip>      // std::setprecision
#include <cmath>
#include <algorithm>
#include <ctime> // Unix time stemp
#include <math.h>
#include <string.h>
#include <vector>
#include <fstream>
#include <cstddef>
#include <chrono> // measuring high-res execution time 
#include <random> // modern C++ random numbers generator
#include <sstream>      // std::stringstream

#include "shower_simulate_globals.h"
#include "shower_simulate_functions.h"

// Start the main function

int main(int argc, char *argv[])
{
	// Now parse all command line options
	for(int n=0; n<argc; n++){
		if(strstr(argv[n],"-n")!=NULL){  
			n++;  
			NEVE = stoi(argv[n]);
			printf("+ Numbers of seconds to be simulated set to %i.\n", NEVE);
		}
		if(strstr(argv[n],"-d")!=NULL){  
			n++;  
			NDET = stoi(argv[n]);
			printf("+ Numbers of detector stations to be simulated set to %i.\n", NDET);
		}
		if(strstr(argv[n],"-m")!=NULL){  
			n++;  
			RATE_MEAN = stof(argv[n]);
			printf("+ Mean of the countrate set to %3.2f.\n", RATE_MEAN);
		}
		if(strstr(argv[n],"-d")!=NULL){  
			n++;  
			RATE_VAR = stof(argv[n]);
			printf("+ StdDev of the countrate set to %3.2f.\n", RATE_VAR);
		}
		if(strstr(argv[n],"-v")!=NULL){  
			n++;  
			VERBOSE = stoi(argv[n]);
			printf("+ Verbose level set to %i.\n", NDET);
		}
		if(strstr(argv[n],"-h")!=NULL){  // print help page
			print_usage();
			return(-1);
		}
	}

	// Initialize the loading screen
	//Prozentanzeige:
	unsigned long int prozent = 0;
	string ausgabeAlgorithmString = "simulating...";
	prozentanzeige(0, ausgabeAlgorithmString);

	// Start the routine by opening NDET output streams each representing one detector
	ofstream det_out[NDET];
	char det_name[200];
	// Loop over the ofstreams to create output files
	for (int n = 0; n < NDET; n++){
		//
		sprintf(det_name, "Detector_%i.txt", n);
		det_out[n].open(det_name, ofstream::trunc); 
		// Test if creation was successfull
	  if (det_out[n].is_open()){
    	if (VERBOSE>1) printf("NOTICE(main): Opened detector file %s.\n", det_name);
		}
		else printf("WARNING(statistics): Unable to open detector file %s.\n", det_name);
		// Write the header line to each file
		det_out[n] << "#rising falling accEst valid timebase utc" << endl;
	}
	// Initialize the event ID counter
	vector<int> event_ID;	
	for (int n = 0; n < NDET; n++){
		event_ID.push_back(0);
	}
	// Now simulate randomly NEVE events starting from same time stamp
	time_t t_start = time(nullptr); // current UNIX time 
	// Uniform number generator
	random_device rd; // obtain a random number from hardware
	mt19937 eng(rd()); // seed the generator
	uniform_int_distribution<> uniform(0, 1000000000); // define the range
	// construct a trivial random generator engine from a time-based seed:
	unsigned seed = chrono::system_clock::now().time_since_epoch().count();
	default_random_engine generator (seed);
	normal_distribution<double> normal(RATE_MEAN,RATE_VAR);
	//
	double local_rate = 1.5;
	vector<long double> events;
	// Loop through NEVE seconds 
	for (int s = 0; s < NEVE; s++){
		// Calculate the simulation progress
		if ( s % (NEVE/100) ==0 ) {
			prozent++;
			prozentanzeige(prozent, ausgabeAlgorithmString);
		}
		// Loop through NDET detectors
		for (int n = 0; n < NDET; n++){
			// Calculate the number of events for this detector for second t_start+s according to Gaussian distribution and expacted count rate RATE
			local_rate = normal(generator);
			if (local_rate < 0) local_rate = 0;
			// Generate the calculated amount of events
			events.clear();
			for (int i = 0; i < local_rate; i++){
				// Generate event
				events.push_back(t_start + s + uniform(eng)/1000000000.0);
			}
			// Sort events 
      sort(events.begin(), events.end());
			// Writing the data to the file
			for (int i = 0; i < (int)events.size(); i++){
				event_ID[n]++;
				det_out[n] 	<< std::fixed; // Fix output precision
				det_out[n] 	<< std::setprecision(9) 
										<< events[i] << " " 				// Rise time
										<< events[i]+0.000001000 << " "		// Fall time
										<< "33" << " "						// Accuracy
										<< event_ID[n] << " "				// Event ID
										<< "1" << " "						// Validity
										<< "01" << " "						// Timebase (?)
										<< "0" << endl;					// UTC (?)
			}
		}
	}
	// Simulation finished
	prozentanzeige(100, ausgabeAlgorithmString); printf("\n");
	printf("Hello simulated muon-world!\n");
	return 0;
}

