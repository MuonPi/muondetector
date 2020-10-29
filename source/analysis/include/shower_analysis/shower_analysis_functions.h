//
//  SHOWER ANALYSIS SOFTWARE 
//
//  FUNCTIONS File
//
//  Written by: 
//    Lukas Nies <Lukas.Nies@Physik.uni-giessen.de> 
//    +
//

// Plot functions
void plot_interpol(vector<double> &x, vector<double> &y, double calib);
void plot_TH2D_hist(TH2D *hist, char const *path, const char *name);
void plot_TH2D_hist_graph(TH2D *hist, vector<double> trace, char const *path, const char *name);
void plot_energy_vs_tagged(vector<double> energy, vector<double> error, char const *path, const char *name);
void plot_energy_resolution(vector<double> sigma, vector<double> sigma_err, char const *path, const char *name);
void plot_trace(vector<double> &trace, char const *name, char const *path, char const *modus);
void plot_1D_xy(vector<double> &x, vector<double> &y, char const *name, char const *path, char const *modus);
// Main "Physics" functions

// "Janitor" functions
double randit(int ini=0);
void print_usage();
void build_structure();
bool read_file(string file);
bool read_config(char const *file);

// Math and fit functions
double polnx(double x, vector<double> &par);
double log3x(double x, vector<double> &par);
double ExpDecay1(double x, vector<double> &par);
double ExpGro1(double x, vector<double> &par);
Double_t SIPMpixel(Double_t *x, Double_t *par);
Double_t SIPMlinearize(Double_t x, Double_t A);
Double_t resolution(Double_t *x, Double_t *par);
Double_t langaufun(Double_t *x, Double_t *par);
Double_t lingaufun(Double_t *x, Double_t *par);
TF1 *langaufit(TH1 *his, Double_t *fitrange, Double_t *startvalues, Double_t *parlimitslo, Double_t *parlimitshi, Double_t *fitparams, Double_t *fiterrors, Double_t *ChiSqr, Int_t *NDF, bool silent);
TF1 *lingaufit(TH1 *his, Double_t *fitrange, Double_t *startvalues, Double_t *parlimitslo, Double_t *parlimitshi, Double_t *fitparams, Double_t *fiterrors, Double_t *ChiSqr, Int_t *NDF, bool silent);
Int_t langaupro(Double_t *params, Double_t &maxx, Double_t &FWHM, char const *function);
bool linreg(vector<double> &x, vector<double> &y, double *m, double *b);

// Array operations
double array_mean(const vector<double> &array, int start, int end);
double array_std(const vector<double> &array, int start, int end, double mean);
int array_largest(vector<double> &array, int lower, int upper);
int array_zero_xing(vector<double> &array, int lower, int upper, int direction = 1);
double array_compare(vector<double> &array1, vector<double> &array2, vector<double> &weigths, vector<double> &par, int start, int end, int debug);
vector<double> array_adjust(vector<double> &array, vector<double> &x, int debug);
vector<double> array_sum(vector<double> &array1, vector<double> &array2, double factor);
vector<double> array_smooth(vector<double> &array, int s, int L);
vector<Double_t> fit_hist(TH1D *hist, TF1 *fit, char const *func, Double_t lower = 0, Double_t upper = 1, int verbose = 0);
vector<Double_t> fit_graph_err(TGraphErrors *graph, char const *func, char const *options, Double_t lower, Double_t upper, int verbose);
Int_t largest_1Dbin(TH1D *hist, Int_t lower, Int_t upper);

// Program routines

// Signal_struct routines

// Histogram operations
Double_t ScaleX(Double_t x, vector<double> par, const char *modus);
Double_t ScaleX(Double_t y, vector<double> par, const char *modus);
Double_t ScaleX(Double_t y, vector<double> par, const char *modus);
void ScaleAxis(TAxis *a, Double_t (*Scale)(Double_t, Double_t, const char), vector<double> par, const char *modus);
void ScaleXaxis(TH1D *h, Double_t (*Scale)(Double_t), vector<double> par, const char *modus);
void ScaleYaxis(TH1D *h, Double_t (*Scale)(Double_t), vector<double> par, const char *modus);
void ScaleZaxis(TH1D *h, Double_t (*Scale)(Double_t), vector<double> par, const char *modus);


////////////////////////////////////////////////////////////////////////////////////////////
/// BODY OF FUNCTIONS HERE
// Some functions still in main, have to be migrated later when done (or not)


void plot_interpol(vector<double> &x, vector<double> &y, double calib){
  TCanvas *c_waves = new TCanvas("c1","Wave_interpolation",200,10,500,300);
  TMarker *xing; 
  c_waves->SetGrid();
  TMultiGraph *mg_waves = new TMultiGraph();
  mg_waves->SetTitle("Interpolation exaple; Time [ns]; interpol. ADC channel [arb. unit]");
  TGraph *tg_waves;
  TGraph *tg_fits;
  TLegend* legend = new TLegend(0.7,0.7,0.9,0.9);
  int warning_counter = 0;
  legend->SetHeader("Interpolated ADC values"); // option "C" allows to center the header
  // Calibrate the x-values
  for (int n = 0; n < (int)x.size(); n++){
  	x[n] *= calib;
  }

  // Fit the passed waves with a linear regression
  double m = 0; double b = 0;
  if (!linreg(x,y,&m,&b)) {
    warning_counter++;
    if (warning_counter < 50){
      printf("WARNING (plot_interpol): Linreg: Singular matrix, can't solve problem\n");
    }
    else{
      printf("WARNING (plot_interpol): Linreg: Further warnigs supressed.\n");
    }
  }
  // Transform vector to array, for handing it to incompetent root TGraph
  Double_t wave_x[x.size()];
  Double_t wave_y[y.size()];
  Double_t fit_y[y.size()];
  vector<double> fit_y_vector;
  for(Int_t n = 0; n<(Int_t)x.size(); n++){
    wave_x[n] = (Double_t) x[n];
    wave_y[n] = (Double_t) y[n];
    fit_y[n] = (Double_t) m * x[n] + b; // linear fit for wave
    fit_y_vector.push_back( (Double_t) m * x[n] + b ); // linear fit for wave
  }
  // Build TGraph
  tg_waves = new TGraph(x.size(),wave_x,wave_y);
  tg_waves->SetLineColor(1);
  tg_waves->SetMarkerColor(1);
  tg_waves->SetLineWidth(2);
  tg_waves->SetTitle("Channel i");
  tg_waves->SetMarkerStyle(kOpenSquare); // Asterisk
  tg_fits = new TGraph(x.size(),wave_x,fit_y);
  tg_fits->SetLineColor(kRed);
  tg_fits->SetMarkerColor(kRed);
  tg_fits->SetLineWidth(2);
  tg_fits->SetTitle("Channel i");
  tg_fits->SetMarkerStyle(kDot);
  //tg_waves[i-1]->Draw("AL*");
  char waves_number[25];
  char fit_number[25];
  char waves_text[25] = "Channel_";
  char fit_text[25] = "Channel_";
  sprintf(waves_number,"1");
  sprintf(fit_number,"1_fit");
  strcat(waves_text,waves_number);
  strcat(fit_text,fit_number);
  legend->AddEntry(tg_waves,waves_text,"f");
  legend->AddEntry(tg_fits,fit_text,"f");
  mg_waves->Add(tg_waves);
  mg_waves->Add(tg_fits);

  double xcrossing = -b/m;

  xing = new TMarker(xcrossing, 0, 1);
  xing->SetMarkerStyle(5);
  xing->SetMarkerColor(kRed);
  xing->SetMarkerSize(3);

  mg_waves->Draw("AL");
  xing->Draw();
  legend->Draw();
  gPad->Modified();
  gPad->Update();
  c_waves->Write("Signal_Interpolated"); 
  delete c_waves;
}

void plot_TH2D_hist(TH2D *hist, char const *path, const char *name){
  // GENERAL Energy Histograms
  hfile->cd(path);
  TCanvas *cs = new TCanvas("cs","cs",10,10,1280,1024);
  // Check if array is a RAW trace
  gPad->SetGridx();
  gPad->SetGridy();
  gStyle->SetOptFit(1111);
  gStyle->SetLabelSize(0.05,"X");
  hist->Draw("COLZ");

  cs->Write(name); 
  delete cs;
}

void plot_TH2D_hist_graph(TH2D *hist, vector<double> trace, char const *path, const char *name){
  // GENERAL Energy Histograms
  hfile->cd(path);
  TCanvas *cs = new TCanvas("cs","cs",10,10,1280,1024);
  // Check if array is a RAW trace
  gPad->SetGridx();
  gPad->SetGridy();
  gStyle->SetOptFit(1111);
  gStyle->SetLabelSize(0.05,"X");
  hist->Draw("COLZ");
  // Now the graph part
  TGraph *graph;
  Double_t wave_y[trace.size()];
  Double_t wave_x[trace.size()];
  for(Int_t n = 0; n< (Int_t)trace.size(); n++){
    wave_y[n] = trace[n]; 
    wave_x[n] = n; // Calibrate to the sampling rate
  }

  graph = new TGraph((Int_t)trace.size(),wave_x,wave_y);
  
  graph->SetLineColor(kRed);
  // graph->SetMarkerColor(i+1);
  graph->SetMarkerColor(kRed);
  graph->SetMarkerSize(0.35);
  graph->SetLineWidth(2);

  graph->Draw("SAME");


  cs->Write(name); 
  delete cs;
}


void plot_energy_vs_tagged(vector<double> energy, vector<double> error, char const *path, const char *name){
  // GENERAL Energy Histograms
  hfile->cd(path);
  TCanvas *cs = new TCanvas("cs","cs",10,10,1280,1024);
  // Check if array is a RAW trace
  gPad->SetGridx();
  gPad->SetGridy();
  gStyle->SetOptFit(1111);
  gStyle->SetLabelSize(0.05,"X");
  // Now the graph part
  TGraphErrors *graph;
  TGraph *graph2;
  Double_t wave_y[(int)energy.size()];
  Double_t wave_y_err[(int)energy.size()];
  Double_t wave_x_err[(int)energy.size()];
  Double_t wave_x[(int)energy.size()];

  for(Int_t n = 0; n < (int)energy.size(); n++){
    wave_y[n] = energy[n]; //  Norm to MeV 
    wave_x[n] = TAGGER.energy[n]; //  Norm to MeV 
    wave_y_err[n] = error[n]; // Norm to MeV
    wave_x_err[n] = TAGGER.energy_err[n]/2; // Tagger energies
    printf("%3.3f %3.3f+-%3.3f\n", TAGGER.energy[n], energy[n], error[n]/2);
  }

  graph = new TGraphErrors((int)energy.size(),wave_x,wave_y,wave_x_err,wave_y_err);

  graph->RemovePoint(5);
  
  graph->SetLineColor(kBlack);
  // graph->SetMarkerColor(i+1);
  graph->SetMarkerColor(kBlack);
  graph->SetMarkerStyle(24);
  graph->SetMarkerSize(2);
  graph->SetLineWidth(2);
  graph->GetXaxis()->SetTitle("Tagger Energy [MeV]");
  graph->GetYaxis()->SetTitle("Detector Energy [MeV]");

  graph->Draw("ap");

  Double_t wave_y2[800];
  Double_t wave_x2[800];

  for(Int_t n = 0; n < 800; n++){
    wave_y2[n] = n;
    wave_x2[n] = n;
  }

  graph2 = new TGraph(800,wave_x2,wave_y2);

  graph2->SetMarkerColor(kGray+2);
  graph2->SetMarkerStyle(31);
  graph2->SetMarkerSize(0.35);
  graph2->SetLineWidth(2);
  graph2->SetLineStyle(9);

  graph2->Draw("SAME");

  vector<Double_t> fit_params;

  fit_params = fit_graph_err(graph, "SIPMpixel", "R", 0, 500, 0);


  // printf("+++++++++ FIT +++++++++++\n");
  // for (int i = 0; i < (int)fit_params.size(); i++){
  //   printf("%3.3f\n", fit_params[i]);
  // }



  cs->Write(name); 
  delete cs;
}

// Energy resolution
void plot_energy_resolution(vector<double> sigma, vector<double> sigma_err, char const *path, const char *name){
  // GENERAL Energy Histograms
  hfile->cd(path);
  TCanvas *cs = new TCanvas("cs","cs",10,10,1280,1024);
  // Check if array is a RAW trace
  gPad->SetGridx();
  gPad->SetGridy();
  gStyle->SetOptFit(1111);
  gStyle->SetLabelSize(0.05,"X");
  // Now the graph part
  TGraphErrors *graph;
  Double_t wave_y[(int)sigma.size()];
  Double_t wave_y_err[(int)sigma.size()];
  Double_t wave_x[(int)sigma.size()];
  Double_t wave_x_err[(int)sigma.size()];

  for(Int_t n = 0; n < (int)sigma.size(); n++){
    wave_y[n] = sigma[n] / (TAGGER.energy[n]) *100; //  Norm to MeV and to percent
    wave_y_err[n] = sqrt(wave_y[n]) ; // Norm to MeV and to percent
    wave_x[n] = TAGGER.energy[n]; // Tagger energies
    wave_x_err[n] = TAGGER.energy_err[n]; // Tagger energies
  }

  graph = new TGraphErrors((int)sigma.size(),wave_x,wave_y,wave_x_err,wave_y_err);

  graph->RemovePoint(5); // Tagger energy does not exist
  graph->SetName("data_graph");
  
  graph->SetLineColor(kBlack);
  graph->SetTitle("Data");
  // graph->SetMarkerColor(i+1);
  graph->SetMarkerColor(kBlue);
  graph->SetMarkerStyle(23);
  graph->SetMarkerSize(4);
  graph->SetLineWidth(2);
  graph->GetXaxis()->SetTitle("Tagger Energy [MeV]");
  graph->GetYaxis()->SetTitle("\\frac{\\sigma}{E} \\left[%\\right]");

  gROOT->SetStyle("Plain");
  gStyle->SetOptFit(1111);



  graph->Draw("ap");

  vector<Double_t> fit_params;

  fit_params = fit_graph_err(graph, "resolution", "R", 10, 300, 0);

  // graph->GetFunction("resolution")->SetLineColor(kRed);
  // graph->GetFunction("resolution")->SetLineWidth(4);

  cs->BuildLegend(0.15, 0.7, 0.4, 0.9,"");

  // TPaveStats st = ((TPaveStats)(graph->GetListOfFunctions()->FindObject("resolution")));
  // if (st) {
  //   st->SetTextColor(graph->GetLineColor());
  //   st->SetX1NDC(0.64); st->SetX2NDC(0.99);
  //   st->SetY1NDC(0.4); st->SetY2NDC(0.6);
  // }

  cs->Modified(); cs->Update(); // make sure it’s really (re)drawn

  cs->Write(name); 
  delete cs;
}


void plot_trace(vector<double> &trace, char const *name, char const *path, char const *modus) {
  // Canvas for the combined waveforms
  TCanvas *canvas = new TCanvas("canvas","Wave_forms",200,10,1280,1024);
  canvas->SetGrid();
  canvas->SetTitle("Signal example; Time [1ns]; ADC channel [arb. unit]");

  TGraph *graph;

  TLegend* legend = new TLegend(0.7,0.7,0.9,0.9);
  legend->SetHeader("ADC Digitization"); // option "C" allows to center the header

  Double_t wave_y[trace.size()];
  Double_t wave_x[trace.size()];
  for(Int_t n = 0; n< (Int_t)trace.size(); n++){
    wave_y[n] = trace[n]; 
    wave_x[n] = n; // Calibrate to the sampling rate
  }

  graph = new TGraph((Int_t)trace.size(),wave_x,wave_y);
  
  graph->SetLineColor(1);
  // graph->SetMarkerColor(i+1);
  graph->SetMarkerColor(1);
  graph->SetMarkerSize(0.35);
  graph->SetLineWidth(2);

  if (is_in_string(modus, "B")) graph->SetFillColor(38);

  graph->Draw(modus);

  hfile->cd(path);
  canvas->Write(name);
  // delete legend;
  // delete[] tg_combined;
  // delete mg_combined;
  delete canvas;
}


void plot_1D_xy(vector<double> &x, vector<double> &y, char const *name, char const *path, char const *modus) {
  // Canvas for the combined waveforms
  TCanvas *canvas = new TCanvas("canvas","canvas",200,10,1280,1024);
  canvas->SetGrid();
  canvas->SetTitle("1D Graph; x; y");

  TGraph *graph;

  TLegend* legend = new TLegend(0.7,0.7,0.9,0.9);
  legend->SetHeader("Legend header"); // option "C" allows to center the header

  Double_t wave_y[y.size()];
  Double_t wave_x[x.size()];
  for(Int_t n = 0; n< (Int_t)x.size(); n++){
    wave_y[n] = y[n]; 
    wave_x[n] = x[n]; // Calibrate to the sampling rate
  }

  graph = new TGraph((Int_t)x.size(),wave_x,wave_y);
  
  graph->SetLineColor(1);
  // graph->SetMarkerColor(i+1);
  graph->SetMarkerColor(1);
  graph->SetMarkerSize(0.35);
  graph->SetLineWidth(2);

  if (is_in_string(modus, "B")) graph->SetFillColor(38);

  graph->Draw(modus);

  hfile->cd(path);
  canvas->Write(name);
  // delete legend;
  // delete[] tg_combined;
  // delete mg_combined;
  delete canvas;
}


double randit(int ini){
  if(ini==1) srand(time(NULL));
  return (((rand()%100) -50.) /100.);
}

// Calculates the mean of all elements between start and end of an array
double array_mean(const vector<double> &array, int start, int end){
  double sum = 0;
  for (int i = start; i<end; i++){
    sum += array[i];
  }
  return (double) sum/(end-start);
}

// Calculates the std of all elements between start and end of an array
double array_std(const vector<double> &array, int start, int end, double mean){
  double sum = 0;
  for (int i = start; i<end; i++){
    sum += (double) pow(array[i]-mean, 2.);
  }
  return (double) TMath::Sqrt(sum/(end-start));
}

// Return index of the largest value in array
int array_largest(vector<double> &array, int lower, int upper){
	//
	int index = 0;
	int max = 0;
	// Check boundaries
	if (lower < 0) lower = 0;
	if (upper > (int)array.size()) upper = (int)array.size(); 
	// Loop over all indices
	for (int n = lower; n < upper; n++){
		//
		if ( array[n] > max ) {
			max = array[n];
			index = n;
		}
	}
	// 
	return(index);

}// Return an array of the sum of to arrays. each field is summed individually and can be automatically divided by a factor
vector<double> array_sum(vector<double> &array1, vector<double> &array2, double factor = 1){
  //
  vector<double> returned; 
  // Check boundaries if arrays have the same length
  if ( array1.size() != array2.size() ){
    printf("WARNING(array_sum): Arrays do not have the same lentgh! Nul array of lentgh 0 returned!\n");
    return returned;
  }
  // If ok, do the sum
  for (int i = 0; i < (int)array1.size(); i++){
    returned.push_back( (array1[i]+array2[i]) * factor );
  }
  // 
  return(returned);
}

// Looks for the first zero crossing between lower and upper of an array, returns index right to the xing
int array_zero_xing(vector<double> &array, int lower, int upper, int direction){
	// Check boundaries
	if (lower < 0) lower = 0;
	if (upper > (int)array.size()) upper = (int)array.size(); 
	// For Forward direction:
	if ( direction > 0 ){
		// Loop over all indices
		for (int n = lower; n < upper; n++){
			//
		    if ( array[n-1] < 0 && array[n] > 0 ){
		    	return(n);
		    }
		}
	}
	// For Revers direction:
	if ( direction < 0 ){
		// Loop over all indices
		for (int n = upper; n > lower; n--){
			//
		    if ( array[n-1] < 0 && array[n] > 0 ){
		    	return(n);
		    }
		}
	}
	// if no xing is found, return 0
	return(0);
}

// Returns the sum of the weighted differences of each element of array1 and array2 between index start and index end
double array_compare(vector<double> &array1, vector<double> &array2, vector<double> &weigths, vector<double> &par, int start, int end, int debug){
  // Parameter
  double A = par[0]; // Multiplication
  double m = par[1]; // Shift
  // Some debug output
  if (debug == 1){
    printf("DEBUG(array_compare): %d %d %d\n", (int)array1.size(), (int)array2.size(), (int)weigths.size());
  }
  // Conversion factor to compare two non equal arrays (who have the same time frame)
  double conversion = 1.;
  double size1 = (double)array1.size();
  double size2 = (double)array2.size();
  // Check all arrays have the same size
  if ( array1.size() != array2.size() ){
    conversion = size1 / size2 ;
    if (debug == 1) printf("WARNING(array_compare): array1 (%d) and array2 (%d) of different length! Using conversion factor %3.2f to compare arrays.\n", (int)array1.size(), (int)array2.size(), conversion);
  }
  // Check if the start and stop values are valid 
  if ( start < 0 ) start = 0;
  if ( start > (int)array1.size() ) start = 0;
  if ( end > (int)array1.size() ) end = (int)array1.size();
  // if no weigths are passed, initialize the weigths as 1 for every sample
  if ( weigths.size() == 0 ){
    for (int n = 0; n < (int)array1.size(); n++) weigths.push_back(1.0);
  }
  if ( array1.size() != weigths.size() ){
    printf("WARNING(array_compare): array1 and weigths of different length! returned value of 1000000.\n");
    return(1000000.0);
  }
  // Check position adjustment parameter is an "integer"
  if ( floor(m) != ceil(m) ){
    if (debug == 1 ) printf("WARNING(array_adjust): Passed position parameter par[1] is not an integer. Rounded to %d.\n", (int)round(m));
    m = (double)round(m);
  }

  // 
  double sum = 0.0;
  int n2 = 0; 
  // No everything should be of the correct size
  for (int n1 = start; n1 < end; n1++){
    // Using array1 as reference frame, calculate equivalent sample value for array2
    n2 = (int)( (int)(round(n1/conversion)) - (int)m );
    // 
    if (n2 > (int)array2.size()){ // If shift is out of range, give penalty back
      if (debug == 1) printf("WARNING(array_compare): Conversion went wrong, calculated sample n2=%d is larger than array2 size %d! Array value is %3.3f!\n", n2, (int)array2.size(), array2[n2]);
      sum += 100000;
    }
    // Do the sum
    else {
      sum += weigths[n1] * abs( array1[n1]- A * array2[n2] );
    }
  }
  return(sum/(end-start));
}

/* Adjust vector according to adjustment parameters vector<double> x:
    - x[0]: A is multiplication factor for amplitude adjustment
    - x[1]: p is position adjustment, should be integer. Shifts array by p positions towards higher or lower indices
*/
vector<double> array_adjust(vector<double> &array, vector<double> &x, int debug){
  // Return array
  vector<double> returned ((int)array.size(), 0.0);
  // Parameters:
  double A = x[0]; // Amplitude
  double m = x[1]; // Time shift
  // x[2] is the error value from array_compare, not being used here
  // for (int i = 0; i < (int)array.size(); i++) returned.push_back(0.0);
  vector<double> dummy ((int)array.size(), 0.0);
  // for (int i = 0; i < (int)array.size(); i++) dummy.push_back(0.0);
  // Check if passed parameters are healthy
  if ( (int)array.size() == 0 ){
    if (debug == 1 ) printf("WARNING(array_adjust): Passed array is empty. Return empty array.\n"); 
    returned.clear();
    return(returned);
  }
  if ( (int)x.size() > 3 ){
    if (debug == 1 ) printf("WARNING(array_adjust): Passed parameter array is of wrong dimension (has to be N=2 (or 3) ). Return empty array.\n"); 
    returned.clear();
    return(returned);
  }
  // Check position adjustment parameter is an "integer"
  if ( floor(m) != ceil(m) ){
    if (debug == 1 ) printf("WARNING(array_adjust): Passed position parameter x[1] is not an integer. Rounded to %d.\n", (int)round(x[1]));
    m = (double)round(m);
  }
  // Check amplitude adjustment parameter is an greater 0 (not 0 or negative)
  if ( A <= 0 ){
    if (debug == 1 ) printf("WARNING(array_adjust): Passed amplitude parameter x[0] is not greter 0. Return x[0] = 1.\n"); 
    A = 0.0;
  }
  // Adjust amplitude 
  for (int i = 0; i < (int)array.size(); i++){
    dummy[i] = array[i] * A;
  }
  // If shift adjustment is zero then dont shift and return array here
  if ( m == 0 ){
    if (debug == 1 ) if (debug == 1) printf("DEBUG(array_adjust): m = 0\n");
    return(dummy);
  }  
  // Shift array by m indizes, the sign is important: - is shift to lower indices, + is shift to higher indices
  // If shift is negative:
  if ( m < 0 ) {
    for (int i = 0; i < (int)array.size()-(int)(abs(m)); i++){
      returned[i] = dummy[i+abs(m)]; // The rightmost m values of the array are 0
    } 
  }
  // If shift is positive:
  if ( m > 0 ) {
    for (int i = 0; i < (int)array.size()-(int)(abs(m)); i++){
      returned[i+abs(m)] = dummy[i]; // The leftmost m values of the array are 0
    } 
  }
  // Test if the adjusted array has the same dimension as the input array
  if ( (int)returned.size() != (int)array.size() ){
    if (debug == 1 ) printf("WARNING(array_adjust): After processing, the adjusted arrray lost initial dimension (bug in code).\n"); 
    returned.clear();
    return(returned);
  }
  // if everything is fine, return valid array
  else{
    return(returned);
  }
}

// From $ROOTSYS/tutorials/fit/langaus.C
Double_t langaufun(Double_t *x, Double_t *par) {
  //Fit parameters:
  //par[0]=Width (scale) parameter of Landau density
  //par[1]=Most Probable (MP, location) parameter of Landau density
  //par[2]=Total area (integral -inf to inf, normalization constant)
  //par[3]=Width (sigma) of convoluted Gaussian function
  //
  //In the Landau distribution (represented by the CERNLIB approximation),
  //the maximum is located at x=-0.22278298 with the location parameter=0.
  //This shift is corrected within this function, so that the actual
  //maximum is identical to the MP parameter.
  // Numeric constants
  // Double_t invsq2pi = 0.3989422804014;   // (2 pi)^(-1/2)
  Double_t mpshift  = -0.22278298;       // Landau maximum location
  // Control constants
  Double_t np = 100.0;      // number of convolution steps
  Double_t sc =   5.0;      // convolution extends to +-sc Gaussian sigmas
  // Variables
  Double_t xx;
  Double_t mpc;
  Double_t fland;
  Double_t sum = 0.0;
  Double_t xlow,xupp;
  Double_t step;
  Double_t i;
  // MP shift correctionj
  mpc = par[1] - mpshift * par[0];
  // Range of convolution integral
  xlow = x[0] - sc * par[3];
  xupp = x[0] + sc * par[3];
  step = (xupp-xlow) / np;
  // Convolution integral of Landau and Gaussian by sum
  //   Gaus(Double_t x, Double_t mean, Double_t sigma, Bool_t norm), norm==true is normalizing the gaus
  //   Landau(Double_t x, Double_t mpv, Double_t sigma, Bool_t norm), norm==true is normalizing the landau
  for(i=0.; i<np; i++) {
  // for(i=1.0; i<=np/2; i++) {
     // xx = xlow + (i-.5) * step;
     xx = xlow + (i) * step;
     fland = TMath::Landau(xx,mpc,par[0],true);//+ (par[4]/(pow(x[0], par[5])));
     sum += fland * TMath::Gaus(x[0],xx,par[3], true);

     // xx = xupp - (i-.5) * step;
     // fland = TMath::Landau(xx,mpc,par[0],true);
     // sum += fland * TMath::Gaus(x[0],xx,par[3], true);
  }
  return (par[2] * step * sum  / par[3]);
}

Double_t lingaufun(Double_t *x, Double_t *par) {
  //Fit parameters:
  //par[0]=Width (scale) parameter of Gaus density
  //par[1]=Most Probable (MP, location) parameter of Gaus density
  //par[2]=Amplitude
  //par[3]=Ntot of linearization function
  //
  // Control constants
  // Variables
  Double_t lingau;
  Double_t x_conv = SIPMpixel(x, &par[3]);

  lingau = par[2] * TMath::Gaus(x_conv, par[1], par[0]);

  return(lingau);
}

// From $ROOTSYS/tutorials/fit/langaus.C
TF1 *langaufit(TH1D *his, Double_t *fitrange, Double_t *startvalues, Double_t *parlimitslo, Double_t *parlimitshi, Double_t *fitparams, Double_t *fiterrors, Double_t *ChiSqr, Int_t *NDF, bool silent){
  // Once again, here are the Landau * Gaussian parameters:
  //   par[0]=Width (scale) parameter of Landau density
  //   par[1]=Most Probable (MP, location) parameter of Landau density
  //   par[2]=Total area (integral -inf to inf, normalization constant)
  //   par[3]=(sigma) of convoluted Gaussian function
  //
  // Variables for langaufit call:
  //   his             histogram to fit
  //   fitrange[2]     lo and hi boundaries of fit range
  //   startvalues[4]  reasonable start values for the fit
  //   parlimitslo[4]  lower parameter limits
  //   parlimitshi[4]  upper parameter limits
  //   fitparams[4]    returns the final fit parameters
  //   fiterrors[4]    returns the final fit errors
  //   ChiSqr          returns the chi square
  //   NDF             returns ndf
  Int_t i;
  Char_t FunName[100];
  sprintf(FunName,"Fitfcn_%s",his->GetName());
  TF1 *ffitold = (TF1*)gROOT->GetListOfFunctions()->FindObject(FunName);
  if (ffitold) delete ffitold;
  TF1 *ffit = new TF1(FunName,langaufun,fitrange[0],fitrange[1],4);
  ffit->SetNpx(10000);
  // TF1 *ffit = new TF1(FunName,langaufun,fitrange[0],fitrange[1],4);
  ffit->SetParameters(startvalues);
  ffit->SetParNames("Width","MP","Area","GSigma");//, "A", "m");
  // ffit->SetParNames("Width","MP","Area","GSigma");
  for (i=0; i<4; i++) {
    ffit->SetParLimits(i, parlimitslo[i], parlimitshi[i]);
  }
  // Do the fit (if suppress output, use silent=true aka "Q")
  if (silent) his->Fit(FunName,"RBQ");
  else his->Fit(FunName,"RB");
  // fit within specified range, use ParLimits, "Q" for quite output
  ffit->GetParameters(fitparams);    // obtain fit parameters
  for (i=0; i<4; i++) {
    fiterrors[i] = ffit->GetParError(i);     // obtain fit parameter errors
  }
  ChiSqr[0] = ffit->GetChisquare();  // obtain chi^2
  NDF[0] = ffit->GetNDF();           // obtain ndf
  return (ffit);              // return fit function
}

TF1 *lingaufit(TH1D *his, Double_t *fitrange, Double_t *startvalues, Double_t *parlimitslo, Double_t *parlimitshi, Double_t *fitparams, Double_t *fiterrors, Double_t *ChiSqr, Int_t *NDF, bool silent){
  // Once again, here are the Landau * Gaussian parameters:
  //   par[0]=Width (scale) parameter of Gaus density
  //   par[1]=Most Probable (MP, location) parameter of Gaus density
  //   par[2]=Amplitude
  //   par[3]=Ntot of linearization function
  //
  // Variables for langaufit call:
  //   his             histogram to fit
  //   fitrange[2]     lo and hi boundaries of fit range
  //   startvalues[4]  reasonable start values for the fit
  //   parlimitslo[4]  lower parameter limits
  //   parlimitshi[4]  upper parameter limits
  //   fitparams[4]    returns the final fit parameters
  //   fiterrors[4]    returns the final fit errors
  //   ChiSqr          returns the chi square
  //   NDF             returns ndf
  Int_t i;
  Char_t FunName[100];
  sprintf(FunName,"Fitfcn_%s",his->GetName());
  TF1 *ffitold = (TF1*)gROOT->GetListOfFunctions()->FindObject(FunName);
  if (ffitold) delete ffitold;
  TF1 *ffit = new TF1(FunName,lingaufun,fitrange[0],fitrange[1],4);
  ffit->SetNpx(10000);
  // TF1 *ffit = new TF1(FunName,langaufun,fitrange[0],fitrange[1],4);
  ffit->SetParameters(startvalues);
  ffit->SetParNames("Width","MP","Amplitude", "Ntot");//, "A", "m");
  // ffit->SetParNames("Width","MP","Area","GSigma");
  for (i=0; i<4; i++) {
    ffit->SetParLimits(i, parlimitslo[i], parlimitshi[i]);
  }
  // Do the fit (if suppress output, use silent=true aka "Q")
  if (silent) his->Fit(FunName,"RBQ");
  else his->Fit(FunName,"RB");
  // fit within specified range, use ParLimits, "Q" for quite output
  ffit->GetParameters(fitparams);    // obtain fit parameters
  for (i=0; i<4; i++) {
    fiterrors[i] = ffit->GetParError(i);     // obtain fit parameter errors
  }
  ChiSqr[0] = ffit->GetChisquare();  // obtain chi^2
  NDF[0] = ffit->GetNDF();           // obtain ndf
  return (ffit);              // return fit function
}

// From $ROOTSYS/tutorials/fit/langaus.C
Int_t langaupro(Double_t *params, Double_t &maxx, Double_t &FWHM, char const *function) {
  // Seaches for the location (x value) at the maximum of the
  // Landau-Gaussian convolute and its full width at half-maximum.
  //
  // The search is probably not very efficient, but it's a first try.
  Double_t p,x,fy,fxr,fxl;
  Double_t step;
  Double_t l,lold;
  Int_t i = 0;
  Int_t MAXCALLS = 50000;
  // Search for maximum
  p = params[1] - 0.1 * params[0];
  step = 0.05 * params[0];
  lold = -2.0;
  l    = -1.0;
  while ( (l != lold) && (i < MAXCALLS) ) {
    i++;
    lold = l;
    x = p + step;
    if (strcmp(function, "langaus") == 0) l = langaufun(&x,params);
    if (strcmp(function, "lingaus") == 0) l = lingaufun(&x,params);
    if (l < lold){
      step = -step/10;
    }
    p += step;
  }
  if (i == MAXCALLS)
    return (-1);
  maxx = x;
  fy = l/2;
  // Search for right x location of fy
  p = maxx + params[0];
  step = params[0];
  lold = -2.0;
  l    = -1e300;
  i    = 0;
  while ( (l != lold) && (i < MAXCALLS) ) {
    i++;
    lold = l;
    x = p + step;
    if (strcmp(function, "langaus") == 0) l = TMath::Abs(langaufun(&x,params) - fy);
    if (strcmp(function, "lingaus") == 0) l = TMath::Abs(lingaufun(&x,params) - fy);
    if (l > lold)
       step = -step/10;
    p += step;
  }
  if (i == MAXCALLS)
    return (-2);
  fxr = x;
  // Search for left x location of fy
  p = maxx - 0.5 * params[0];
  step = -params[0];
  lold = -2.0;
  l    = -1e300;
  i    = 0;
  while ( (l != lold) && (i < MAXCALLS) ) {
    i++;
    lold = l;
    x = p + step;
    if (strcmp(function, "langaus") == 0) l = TMath::Abs(langaufun(&x,params) - fy);
    if (strcmp(function, "lingaus") == 0) l = TMath::Abs(lingaufun(&x,params) - fy);
    if (l > lold)
       step = -step/10;
    p += step;
  }
  if (i == MAXCALLS)
    return (-3);
  fxl = x;
  FWHM = fxr - fxl;
  return (0);
}

// Look for the bins in a histogram with the most entries between lower and upper
Int_t largest_1Dbin(TH1D *hist, Int_t lower = 0, Int_t upper = 100000){
  // Get number of cells plus overflow and underfolw
  Int_t ncells = hist->GetSize();
  // Check if hist has entries different from under and overflow
  if (ncells < 3){
    printf("WARNING (largest_1Dbin): Histogram empty, check under/overflow!\n");
    return(0);
  } 
  // Check if lower and upper are included in the cells
  if (upper > ncells) {
    upper = ncells -1;
  }
  if (lower > ncells) {
    lower = 1;
  }

  // Now loop over the cells and look for highest entry
  Int_t largest = 0;
  Int_t largest_bin = 0;
  // Int_t xmin = hist->GetXaxis()->GetXmin(); // Xmin of histogram
  // Int_t xmax = hist->GetXaxis()->GetXmax(); // Xmin of histogram
  for (Int_t i = abs(lower); i < (upper+abs(lower)); i++){
    if (largest < hist->GetBinContent(i)){ 
      largest_bin = i ; // Norm it to the hist range
      largest = hist->GetBinContent(i);
    }
  }
  // Return index of largest bin
  // printf("%d %d %d %d %d\n", xmin, xmax, ncells, lower, largest_bin);
  return(largest_bin);
}

// Do various fits for a 1D histogram 
vector<Double_t> fit_hist(TH1D *hist, TF1 *fit, char const *func, Double_t lower, Double_t upper, int verbose){
  vector<Double_t> params;
  // Set the fit area around the mean value of the gaussian
  //hist->GetXaxis()->SetRange(hist->GetMean()-range,hist->GetMean()+range);
  // Check for which fit function is chosen
  int n;
  if (strcmp(func, "gaus")==0){n = 3;}
  else if (strcmp(func, "multigaus")==0){n = 3;}
  else if (strcmp(func, "langaus")==0){n = 4;}
  else if (strcmp(func, "lingaus")==0){n = 4;}
  else if (strcmp(func, "langaus_roofit")==0){n = 6;}
  else{ printf("ERROR (fit_hist): Wrong fit function name passed! Currently available: gaus, multigaus, langaus, lingaus.\n"); }
  // Check if the histograms were filled
  if ( (int) hist->Integral() < 2 ){
    if (is_in_string(VERBOSE, "h")){
      printf("WARNING (fit_hist: %s): Histogram empty (Integral=%d!\n", func, (int)hist->Integral());     
    }
    for (int i = 0; i<n+2; i++){
      params.push_back(1.);
      params.push_back(1.);
    } 
    return(params);   
  }
  // Now start the fit routines
  // Gaus fit
  if (strcmp(func, "gaus")==0){
    n = 3;
    // Field 0,1: amp, amp_err
    // Field 2,3: mean, mean_err
    // Field 4,5: std, std_err 
    hist->SetAxisRange(lower, upper);
    // Check if subrange of hist is empty
    if ( (int) hist->Integral() < 2 ){
      if (is_in_string(VERBOSE, "h")){
        printf("WARNING (fit_hist: %s, range %d to %d): Histogram empty!\n", func, (int)lower, (int)upper);     
      }
      for (int i = 0; i<n+2; i++){
        params.push_back(1.);
        params.push_back(1.);
      } 
      return(params);   
    }
    // If not, fit the sub range
    hist->Fit("gaus", "Q", "SAME", lower, upper);
    fit = hist->GetFunction("gaus");
    for (Int_t i = 0; i<n; i++){
      params.push_back(fit->GetParameter(i));
      params.push_back(fit->GetParError(i));
    }
    return(params);  
  }
  // Langaus (landau convoluted with gaus) fit
  if (strcmp(func, "lingaus")==0){
    n = 4;
    // Rebin a temp histogram before fitting
    Int_t nrebin = 50;
    int errors = 0;
    // Setting fit range and start values
    hist->Rebin(nrebin);
    // Double_t divide = hist->GetXaxis()->GetXmax() / hist->GetNbinsX();
    // Double_t largest_bin = largest_1Dbin(hist, 0, 1000); // heaviest bin between lower and upper bound
    // printf("%3.1f\n", largest_bin);
    Double_t fr[2]; // fit boundaries
    Double_t sv[4], pllo[4], plhi[4], fp[4], fpe[4]; 
    fr[0]=lower; // Lower fit boundary
    fr[1]=upper; // Upper fit boundary
    // fr[0]=0.65*largest_bin; // Lower fit boundary
    // fr[1]=3.0*largest_bin; // Upper fit boundary
    // printf("%3.3f %3.3f %3.3f %3.3f\n", largest_bin, fr[0], fr[1], divide);

    //Fit parameters:
    //par[0]=Width (scale) parameter of Gaus density
    //par[1]=Most Probable (MP, location) parameter of Gaus density
    //par[2]=Amplitude
    //par[3]=Ntot of the linearization function
    // ALL VALUES IN MeV or RELATED VALUES
    pllo[0]=1      ; pllo[1]=0               ; pllo[2]=1.0;  pllo[3]=0.01; 
    plhi[0]=100.   ; plhi[1]=1000            ; plhi[2]=1e6; plhi[3]=1e5; 
      sv[0]=5.    ;    sv[1]=50              ;   sv[2]=5e3;    sv[3] =5e3; 
    Double_t chisqr; // Chi squared
    Int_t ndf; // # degrees of freedom
    bool silent = true;
    if (is_in_string(VERBOSE, "f")) silent = false; 
    fit = lingaufit(hist,fr,sv,pllo,plhi,fp,fpe,&chisqr,&ndf, silent); // true/false for quite mode
    Double_t SNRPeak, SNRFWHM; 
    errors = langaupro(fp,SNRPeak,SNRFWHM, "lingaus"); // Search for peak and FWHM
    if (errors != 0) printf("WARNING(fit_hist): ERROR(langaupro): %d\n", errors);
    // Save the parameters in the return container
    for (int i = 0; i<n; i++){ 
      params.push_back(fp[i]);
      params.push_back(fpe[i]);
    }
    // Add the parameters from the langaupro search
    params.push_back(SNRPeak); params.push_back(SNRFWHM/2);
    printf("%3.3f\n", SNRPeak);
    return(params);
  }
  // Multigaus: uses peaksensing to find peaks in a histgrams and then fits them with a superposition of gaussians
  if (strcmp(func, "multigaus")==0){
    //Use TSpectrum to find the peak candidates
    TSpectrum *s = new TSpectrum(5);
    Int_t upper_range = hist->GetSize() - 2;
    hist->Rebin(20);
    hist->SetAxisRange(200, upper_range);
    Int_t nfound = s->Search(hist,3,"",0.50);
    // printf("Found %d candidate peaks to fit\n",nfound);
    // nfound = 3;
    //Loop on all found peaks. Eliminate peaks at the background level
    Double_t *xpeaks = s->GetPositionX();
    TF1 *g[nfound]; 
    char t_name[1000]="gaus(0)";
    for (int p = 0; p < nfound; p++){
      char g_name[100];
      sprintf(g_name, "m%d", p);
      if (p > 0) sprintf(t_name, "%s+gaus(%d)", t_name, p*3);
      g[p] = new TF1(g_name,"gaus",xpeaks[p]-200,xpeaks[p]+200);
      // printf("%s %s\n", g_name, t_name);
    }
    // The total is the sum of the three, each has 3 parameters
    fit = new TF1("mstotal",t_name,xpeaks[0]-500,xpeaks[nfound-1]+500);
    Double_t par[nfound*3];
    for (int p = 0; p < nfound; p++){
      char g_name[100];
      sprintf(g_name, "m%d", p);
      if (p == 0) hist->Fit(g_name,"0R");
      if (p > 0) hist->Fit(g_name,"0R+");
      g[p]->GetParameters(&par[p*3]);
    }
    fit->SetParameters(par);
    hist->Fit(fit,"R+");
    fit->GetParameters(par);
    for (int i = 0; i < nfound*3; i++){
      params.push_back(fit->GetParameter(i));
      params.push_back(fit->GetParError(i));
    }
    return(params);
  }
  // Langaus (landau convoluted with gaus) fit
  if (strcmp(func, "langaus")==0){
    n = 4;
    // Rebin a temp histogram before fitting
    Int_t nrebin = 100;
    int errors = 0;
    // Setting fit range and start values
    hist->Rebin(nrebin);
    Double_t divide = hist->GetXaxis()->GetXmax() / hist->GetNbinsX();
    Double_t largest_bin = largest_1Dbin(hist, 20*GENERAL_SCALING/divide, 800*GENERAL_SCALING/nrebin)*divide*GENERAL_SCALING; // heaviest bin between lower and upper bound
    // printf("%3.1f\n", largest_bin);
    Double_t fr[2]; // fit boundaries
    Double_t sv[4], pllo[4], plhi[4], fp[4], fpe[4]; 
    fr[0]=0.65*largest_bin; // Lower fit boundary
    fr[1]=3.0*largest_bin; // Upper fit boundary
    printf("%3.3f %3.3f %3.3f %3.3f\n", largest_bin, fr[0], fr[1], divide);

    //Fit parameters:
    //par[0]=Width (scale) parameter of Landau density
    //par[1]=Most Probable (MP, location) parameter of Landau density
    //par[2]=Total area (integral -inf to inf, normalization constant)
    //par[3]=Width (sigma) of convoluted Gaussian function
    //par[4]=A from A/(x^(m))
    //par[5]=m from A/(x^(m))
    //ADDED LATER: par[6]= Maximum of convoluted function
    //ADDED LATER: par[7]= FWHM of convoluted function
    // ALL VALUES IN MeV or RELATED VALUES
    pllo[0]=1      ; pllo[1]=largest_bin-2.50; pllo[2]=1.0; pllo[3]=1.     ;// pllo[4]=-10.0 ; pllo[5]=0.0001;  // Lower parameter limits
    plhi[0]=10.   ; plhi[1]=largest_bin+2.50; plhi[2]=1e10; plhi[3]=50.0;// plhi[4]=100000.0; plhi[5]=5.0; // Upper parameter limits
    sv[0]  =2.    ; sv[1]  =largest_bin      ; sv[2]  =5e3 ; sv[3]  =5.0 ;// sv[4]  =100.0   ; sv[5]=0.05;// Start values
    Double_t chisqr; // Chi squared
    Int_t ndf; // # degrees of freedom
    bool silent = true;
    if (is_in_string(VERBOSE, "f")) silent = false; 
    fit = langaufit(hist,fr,sv,pllo,plhi,fp,fpe,&chisqr,&ndf, silent); // true/false for quite mode
    Double_t SNRPeak, SNRFWHM; 
    errors = langaupro(fp,SNRPeak,SNRFWHM, "langaus"); // Search for peak and FWHM
    if (errors != 0) printf("WARNING(fit_hist): ERROR(langaupro): %d\n", errors);
    // Save the parameters in the return container
    for (int i = 0; i<n; i++){ 
      params.push_back(fp[i]);
      params.push_back(fpe[i]);
    }
    // Add the parameters from the langaupro search
    params.push_back(SNRPeak); params.push_back(SNRFWHM/2);
    printf("%3.3f\n", SNRPeak);
    return(params);
  }
  // Novosibirsk function
  // if (strcmp(func, "novosibirsk")==0){
  //   // Declare observable x
  //   int lower = hist->GetXaxis()->GetXmin();
  //   int upper = hist->GetXaxis()->GetXmax();
  //   RooRealVar x("x","x",lower,upper);
  //   // --- Import data ---
  //   RooDataHist data("data","data",x,Import(*hist)) ;
  //   // --- Parameters ---
  //   RooRealVar mean("sigmean","mean",5.28,5.20,5.30);
  //   RooRealVar width("sigwidth","width",0.0027,0.001,1.);
  //   RooRealVar tail("sigwidth","tail",0.0027,0.001,1.);

  //   return(params);
  // }

  // Stable langaus with RooFit 
  if (strcmp(func, "langaus_roofit")==0){
    //
    // Look for the maximum value
    hfile->cd("JUNK");
    int nrebin = 1;
    Double_t divide = hist->GetXaxis()->GetXmax() / hist->GetNbinsX();  
    Double_t maxBin = largest_1Dbin(hist, 2000*GENERAL_SCALING/divide, 80000*GENERAL_SCALING/nrebin)*divide*GENERAL_SCALING; // heaviest bin between lower and upper bound
    printf("+++++++++++++++++++ MAXBIN: %3.3f\n", maxBin);
    Double_t minX = 0.75 * maxBin;
    Double_t maxX = 3.00 * maxBin;
    Double_t leftX = 0.75 * maxBin;
    Double_t rightX = 1.25 * maxBin;
    //assuming your histogram is the variable "hist".
    Double_t sigma = (rightX-leftX)/2.35;
    // Construct observable
    // RooRealVar t("t","t",minX,maxX);
    RooRealVar t("t","t",hist->GetXaxis()->GetBinLowEdge(1),hist->GetXaxis()->GetBinUpEdge(hist->GetNbinsX()));
    // Define fit range
    t.setRange("ROI_1",minX,maxX);
    // Construct gauss(t,mg,sg)
    RooRealVar mg("mg","mg",0) ;
    RooRealVar sg("sg","sg",sigma,0.1*sigma,5.*sigma) ;
    RooGaussian gauss("gauss","gauss",t,mg,sg) ;

    // Construct landau(t,ml,sl) ;
    RooRealVar ml("ml","mean landau",maxBin,maxBin-sigma,maxBin+sigma) ;
    RooRealVar sl("sl","sigma landau",sigma,0.1*sigma,5.*sigma) ;
    // RooRealVar sl("sl","sigma landau",0.04,0.,0.2) ;
    RooLandau landau("lx","lx",t,ml,sl) ;

    // C o n s t r u c t   c o n v o l u t i o n   p d f 
    // ---------------------------------------

    // Set #bins to be used for FFT sampling
    t.setBins(5000,"cache") ; 

    // Construct landau (x) gauss
    RooFFTConvPdf lxg("lxg","landau (X) gauss",t,landau,gauss) ;

    // S a m p l e ,   f i t   a n d   p l o t   c o n v o l u t e d   p d f 
    // ----------------------------------------------------------------------

    RooDataHist* data = new RooDataHist("dh","dh",t,RooFit::Import(*hist)) ;

    // Fit gxlx to data
    lxg.fitTo(*data,RooFit::Range("ROI_1"));
    
    // Plot data, landau pdf, landau (X) gauss pdf
    TCanvas *canvas;
    canvas = new TCanvas("Langaus","Langaus",800,800);
    RooPlot* frame = t.frame(RooFit::Title((TString)"FitProjection")) ;
    // data->plotOn(frame) ;
    RooPlot* fitLine=lxg.plotOn(frame) ;

    lxg.paramOn(frame);
    data->statOn(frame);
    landau.plotOn(frame,RooFit::LineStyle(kDashed)) ;
    // gauss.plotOn(frame,RooFit::LineStyle(kDashed)) ;

    TF1* flxg = lxg.asTF (RooArgList(t)) ;//NOT NORMALIZED
      
    Double_t modX=flxg->GetMaximumX();
    
    RooCurve* fitCurve = (RooCurve*)fitLine->findObject("lxg_Norm[t]");
    // RooCurve* fitCurve = (RooCurve*)fitLine->findObject("lxg_Norm[t]_Range[fit_nll_lxg_dh]_NormRange[fit_nll_lxg_dh]");
    Double_t maxpico=fitCurve?fitCurve->getYAxisMax():0; 
    Double_t maxpicoU=flxg->Eval(modX);//unnormalized
    Double_t extrems[2];
    
    extrems[0]=flxg->GetX(maxpicoU/2.0,minX,modX);
    extrems[1]=flxg->GetX(maxpicoU/2.0,modX,maxX);

    printf("%3.3f %3.3f %3.3f %3.3f\n", maxpico, maxpicoU, extrems[0], extrems[1]);
    
    // Draw frame on canvas
    frame->Draw();

    TH1F *clone = (TH1F*)(hist->Clone("clone"));
    Double_t norm = clone->GetEntries();
    clone->Scale(1/clone->GetBinContent(maxBin/100));
    printf("\n\n\n+++++ BINCONTENT(%d): %3.3f\n\n\n\n\n", clone->GetMaximumBin(), clone->GetBinContent(clone->GetMaximumBin()));

    clone->Draw("SAME");

    canvas->Write("RooFit_Langaus");
  
    TLine* l;
    l = new TLine(extrems[0],maxpico/2.0,extrems[1],maxpico/2.0);
    l->SetLineWidth(1);
    l->SetLineColor(1);
    l->Draw("same");

    l = new TLine(extrems[0],hist->GetMinimum(),extrems[1],hist->GetMinimum());
    l->SetLineWidth(1);
    l->SetLineColor(1);
    l->Draw("same");
    
    
    l = new TLine(modX,hist->GetMinimum(),modX,maxpico);
    l->SetLineWidth(0.5);
    l->SetLineColor(1);
    l->Draw("same");
    l=NULL;

    delete canvas;



    return(params);


  }

  // 
  else {return(params);}
}

vector<Double_t> fit_graph_err(TGraphErrors *graph, char const *func, char const *options, Double_t lower, Double_t upper, int verbose){
  //
  //
  vector<Double_t> params;
  int n = 0;
  TF1 *fit;
  //
  if (strcmp(func, "SIPMpixel")==0){
    n = 1;
    Double_t par[1];
    par[0] = 1000.;
    fit = new TF1( "SIPMpixel_fit", SIPMpixel,  lower, upper, 1);
    // printf("%3.3f\n", par[0]);
    fit->SetParameters(par);
    fit->SetParNames("N_tot");
    graph->Fit("SIPMpixel_fit", options, "", lower, upper);

    for (int i = 0; i < n; i ++){
      params.push_back(fit->GetParameter(i));
      params.push_back(fit->GetParError(i));
    }
  }
  //
  //
  if (strcmp(func, "resolution")==0){
    // par[0]: A is stochastic shower development parameter
    // par[1]: B is parameter for electric noice  
    // par[2]: C is free parameter
    // par[3]: D is third degree parameter
    // par[4]: x_0 is pole shift parameter
    n = 5;
    //
    double par_start[n+1], par_low[n+1], par_up[n+1];
    par_low[0] = 0.00001; par_start[0] = 430; par_up[0] = 1000;
    par_low[1] = 0.00001; par_start[1] = 2200; par_up[1] = 50000;
    par_low[2] = 0.00001; par_start[2] = 15; par_up[2] = 50;
    par_low[3] = 0.001; par_start[3] = 1000; par_up[3] = 50000;
    par_low[4] = 0.01; par_start[4] = 1; par_up[4] = 25;
    //
    fit = new TF1( "Resolution_fit", resolution,  lower, upper, n);
    // printf("%3.3f\n", par[0]);
    fit->SetParameters(par_start);
    fit->SetParNames("A", "B", "C", "D", "x_{0}");
    //
    for (int i=0; i<n; i++) {
      fit->SetParLimits(i, par_low[i], par_up[i]);
    }

    //
    graph->Fit("Resolution_fit", options, "", lower, upper);

    for (int i = 0; i < n; i ++){
      params.push_back(fit->GetParameter(i));
      params.push_back(fit->GetParError(i));
    }



  }
  return params;
}

// Fast linear regression fit 
// Source: http://stackoverflow.com/questions/5083465/fast-efficient-least-squares-fit-algorithm-in-c
bool linreg(vector<double> &x, vector<double> &y, double *m, double *b){
  // double r; // Correlation coefficient
  int n = (int)x.size();
  double sumx = 0.0;                      /* sum of x     */
  double sumx2 = 0.0;                     /* sum of x**2  */
  double sumxy = 0.0;                     /* sum of x * y */
  double sumy = 0.0;                      /* sum of y     */
  double sumy2 = 0.0;                     /* sum of y**2  */
  for (int i=0; i < n; i++){ 
    sumx  += x[i];       
    sumx2 += pow(x[i], 2.);  
    sumxy += x[i] * y[i];
    sumy  += y[i];      
    sumy2 += pow(y[i], 2.); 
  } 
  double denom = (n * sumx2 - pow(sumx, 2.));
  if (denom == 0) {
    // singular matrix. can't solve the problem.
    *m = 0;
    *b = 0;
    return false;
  }
  *m = (n * sumxy  -  sumx * sumy) / denom;
  *b = (sumy * sumx2  -  sumx * sumxy) / denom;
  // r = (sumxy - sumx * sumy / n) /    /* compute correlation coeff */
  //       sqrt((sumx2 - pow(sumx, 2.)/n) *
  //       (sumy2 - pow(sumy, 2.)/n));
  return true; 
}

// Prints usage informaion for user
void print_usage(){
  printf("Use the following: ./shower_analysis datafile.dat rootfile.root (optional: configfile.ini) [-command line options]\n");
  printf("Command line options:\n"); 
  printf("  - n (int)      : Max number of events to be read in.\n"); 
  printf("  - v (str)      : Turns on verbose mode. Because more ouput is always better!\n"); 
  printf("      + p: extra output (tbi)\n");
  printf("\n\n");
}

// Builds folder structure
void build_structure(){
  hfile->mkdir("GENERAL");
  hfile->mkdir("JUNK");
}

// Searches string if character is present
bool is_in_string(char const *character, char const *letter){
  // convert char to str
  string str(character);
  // look for character
  size_t found = str.find_first_of(letter);
  // printf("%d\n", (int)found);
  // cout << character << " " << letter << " " << found << endl;
  if ((int)found == -1) return false;
  else return true;
}

// Polynomial of nth degree, dimension given by par.size()
double polnx(double x, vector<double> &par){
  // Dimension
  int n = (int)par.size();
  // Return value
  double f = 0;
  // Execute sum of polynomials
  for (int i = 0; i < n; i++){
    f += par[i] * pow(x, i);
  }
  // Return f
  return(f);
}

// Logarithm with 3 parameters
double log3x(double x, vector<double> &par){
  // Dimension
  if ((int)par.size() != 3){
    printf("WARNING(log3x): Wrong amount of parameters passed (size: %d, must be 3!\n", (int)par.size());
    return(0);
  } 
  // Return value
  double f = par[0] - par[1] * log(x+par[2]);
  // Return f
  return(f);
}

// Exponential Growth
double ExpGro1(double x, vector<double> &par){
  // Dimension
  if ((int)par.size() != 3){
    printf("WARNING(ExpGro1): Wrong amount of parameters passed (size: %d, must be 3!\n", (int)par.size());
    return(0);
  } 
  // Return value
  double f = par[0] * exp( x / par[1] ) + par[2];
  // Return f
  return(f);
}

// Exponential Decay with undershoot compensation
double ExpDecay1(double x, vector<double> &par){
  // Dimension
  if ((int)par.size() != 5){
    printf("WARNING(ExpGro1): Wrong amount of parameters passed (size: %d, must be 55!\n", (int)par.size());
    return(0);
  } 
  // Return value
  double f = par[0] * exp( - x / par[1] ) + par[2] + par[3] * x + par[4] * pow(x, 2);
  // Return f
  return(f);
}

// Fit function for pixel saturation
Double_t SIPMpixel(Double_t *x, Double_t *par){
  // Parameter carriers
  // par[0]: corresponds to total amount of pixels, N_total
  // x: N of pixels fired 
  double value = par[0] * ( 1 - exp( - x[0] / par[0] ) );
  return value;
}

// Compensation function for pixel saturation
Double_t SIPMlinearize(Double_t x, Double_t A){
  // Parameter carriers
  // par[0]: corresponds to total amount of pixels, N_total
  // x: N of pixels fired 
  if ( x/A > 1 ) return (0.0);
  else {
    double value = - A * log( 1 - ( x / A ) );
    return value;
  }
}

// Energy  and time resolution fit
Double_t resolution(Double_t *x, Double_t *par){
  // Parameter carriers
  // par[0]: A is stochastic shower development parameter
  // par[1]: B is parameter for electric noice  
  // par[2]: C is free parameter
  // par[3]: D is third degree parameter
  // par[4]: x_0 is pole shift parameter
  // x: Energy
  double value = sqrt( (pow(par[0], 2)/(x[0]-par[4])) + (pow(par[1], 2)/pow((x[0]-par[4]),2)) + (pow(par[3], 3)/pow((x[0]-par[4]),3)) + pow(par[2], 2) );
  return value;
}

// Smooth array around passed value with smoothing interval given
vector<double> array_smooth(vector<double> &array, int s, int L){
  // copy array
  vector<double> returned = array;
  // do the smoothing around sample s
  for (int n = s-L; n < (s+L); n++){
    // adjust smoothing interval around current sample n to the distance to s
    // smoothing interval length
    int length = L;//- (int)( 0.5 *abs(s-n))+ L;
    int counter = 0;
    double sum = 0;
    for (int k = n-length ; k < n+length; k++){
      sum += array[k];
      counter++;
    }
    sum /= counter;
    returned[n] = sum;
  }
  return(returned);
}


// Scale x or y axis of histogram
//// Taken and modified from
  // "https://root-forum.cern.ch/t/can-we-shift-histogram-for-several-channels/12050/2"

Double_t ScaleX(Double_t x, vector<double> par, const char *modus)
{
  Double_t v;
  if (strcmp("linear", modus) == 0) v = par[0] * x + par[1]; // "linear scaling" function example
  if (strcmp("SIPMlinearize", modus) == 0){
    v = SIPMlinearize(x, par[0]); // SiPMLinerization
  }
  return v;
}

Double_t ScaleY(Double_t y, vector<double> par, const char *modus)
{
  Double_t v;
  if (strcmp("linear", modus) == 0) v = par[0] * y + par[1]; // "linear scaling" function example
  if (strcmp("SIPMlinearize", modus) == 0) v = SIPMlinearize(y, par[0]); // SiPMLinerization
  return v;
}

Double_t ScaleZ(Double_t z, vector<double> par, const char *modus)
{
  Double_t v;
  if (strcmp("linear", modus) == 0) v = par[0] * z + par[1]; // "linear scaling" function example
  if (strcmp("SIPMlinearize", modus) == 0) v = SIPMlinearize(z, par[0]); // SiPMLinerization
  return v;
}

void ScaleAxis(TAxis *a, Double_t (*Scale)(Double_t, vector<double>, const char*), vector<double> par, const char *modus)
{
  if (!a) return; // just a precaution
  if (a->GetXbins()->GetSize())
    {
      // an axis with variable bins
      // note: bins must remain in increasing order, hence the "Scale"
      // function must be strictly (monotonically) increasing
      TArrayD X(*(a->GetXbins()));
      for(Int_t i = 0; i < (int)X.GetSize(); i++) {
        X[i] = Scale(X[i], par, modus);
      }
      printf("\n");
      a->Set((X.GetSize() - 1), X.GetArray()); // new Xbins
    }
  else
    {
      // an axis with fix bins
      // note: we modify Xmin and Xmax only, hence the "Scale" function
      // must be linear (and Xmax must remain greater than Xmin)
      if (strcmp(modus, "linear") == 0){
        // printf("Xmin: %3.3f, Xmax: %3.3f\n", Scale(a->GetXmin(), par, modus), Scale(a->GetXmax(), par, modus));
        a->Set( a->GetNbins(),
          Scale(a->GetXmin(), par, modus), // new Xmin
          Scale(a->GetXmax(), par, modus) 
          ); // new Xmax
      }
    }
  return;
}

void ScaleXaxis(TH1D *h, Double_t (*Scale)(Double_t, vector<double>, const char*), vector<double> par, const char *modus)
{
  if (!h) return; // just a precaution
  ScaleAxis(h->GetXaxis(), Scale, par, modus);
  h->ResetStats();
  return;
}

void ScaleYaxis(TH1D *h, Double_t (*Scale)(Double_t, vector<double>, const char*), vector<double> par, const char *modus)
{
  if (!h) return; // just a precaution
  ScaleAxis(h->GetYaxis(), Scale, par, modus);
  h->ResetStats();
  return;
}

void ScaleZaxis(TH1D *h, Double_t (*Scale)(Double_t, vector<double>, const char*), vector<double> par, const char *modus)
{
  if (!h) return; // just a precaution
  ScaleAxis(h->GetZaxis(), Scale, par, modus);
  h->ResetStats();
  return;
}
