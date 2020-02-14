//
//  SHOWER SIMULATION SOFTWARE 
//
//  FUNCTIONS File
//
//  Written by: 
//    Lukas Nies <Lukas.Nies@cern.ch> 
//    +
//

void print_usage();

//






// Prints usage informaion for user
void print_usage(){
  printf("SHOWER SIMULATION SOFTWARE\n\n");
  printf("How it works: \n ");
  printf("For NEVE seconds, detector events in NDET detectors are simulated. Starting from the current point of time, t_start, \n"); 
  printf("for each second and detector a count rate is drawn from a Gaussian distribution with RATE_MEAN and RATE_VAR.\n");
  printf("For this second and detector, a certain number of events is uniformly generated and sorted timewise. The \n");
  printf("output is written to NDET different .txt file in plain text format. The ouput is as follows:\n");
  printf(" 		[rising] [falling] [ID] [accEst] [valid] [timebase] [utc] \n\n");
  printf("Use the following: ./shower_simulation [-command line options]\n");
  printf("Command line options:\n"); 
  printf("  - n (int)      : Number of seconds to be simulated. Standard: 3600\n"); 
  printf("  - d (int)      : Number of detector stations to be simulated. Standard: 2\n"); 
  printf("  - m (double)   : Mean of the Gaussian distributed detector countrate. Standard: 1.5 Hertz\n"); 
  printf("  - s (double)   : StdDev of the Gaussian distributed detector countrate. Standard: 1.0 Hertz\n"); 
  printf("  - v (int)      : Turns on verbose mode. Because more ouput is always better!. Standard: 1\n"); 
  printf("\n\n");
}

void prozentanzeige(unsigned long int prozent, string& ausgabe)
{
  int width = 50;
  int balken;    char a = '|';
  balken = prozent * width / 100;
  if (balken > (int)oldBalken)
  {
    string prozentbalken = "";
    stringstream convert;
    convert << prozent;
    prozentbalken += "\r ";
    prozentbalken += ausgabe;
    prozentbalken += " |";
    if (prozent < 100)
    {
      //sonst length_error
      prozentbalken += string(balken, a);
      prozentbalken += string(width - balken, ' ');
    }
    else
    {
      prozentbalken += string(width, a);
    }
    prozentbalken += "|";
    prozentbalken += " ";
    if (prozent < 100)
    {
      prozentbalken += convert.str();
    }
    else {
      //sonst mit etwas pech ueber 100%
      prozentbalken += "100";
    }
    prozentbalken += "%";
    prozentbalken += "     ";
    cout << prozentbalken << flush;// <<endl;
    //cout<<"\r[%3d%%]"<<prozentbalken; 
    oldBalken = balken;
  }
}

