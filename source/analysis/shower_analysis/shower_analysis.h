//
//	SHOWER ANALYSIS SOFTWARE 
//
//  HEADER File
//
//  Written by: 
//		Lukas Nies <Lukas.Nies@Physik.uni-giessen.de> 
//		+
//

#include <stdlib.h>
#include <stdio.h>
#include <iomanip>      // std::setprecision
#include <cmath>
#include <math.h>
#include <string.h>
#include <vector>
#include <fstream>
#include <cstddef>
#include <chrono> // measuring high-res execution time 
#include "Math/Interpolator.h"
#include "Math/Polynomial.h"
#include "TROOT.h"
#include "TMath.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TF1.h"
#include "TProfile.h"
#include "TNtuple.h"
#include "TRandom.h"
#include "TTree.h"
#include "TAxis.h"
#include "TArrayD.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TMultiGraph.h"
#include "TLegend.h"
#include "TDirectory.h"
#include "TStyle.h"
#include "TList.h"
#include "TExec.h"
#include "TText.h"
#include "TGraphPainter.h"
#include "TSpectrum.h"  // For spectrum and peak analysis
#include "TVirtualFitter.h" // fitting
#include "TPaveStats.h"
#include "TMarker.h"
// RooFit Framework
#include "RooAddPdf.h"
#include "RooDataSet.h"
#include "RooPlot.h"
#include "RooRealVar.h"
#include "RooDataHist.h"
#include "TLatex.h"
#include "RooNovosibirsk.h"

#include "RooRealVar.h"
// #include "RooDataSet.h"
#include "RooGaussian.h"
#include "RooLandau.h"
#include "RooFFTConvPdf.h"
// #include "RooGenericPdf.h"
#include "RooPlot.h"
#include "RooCurve.h"
#include "RooDataHist.h"


// shower_analysis specific header
#include "shower_analysis_globals.h"
#include "shower_analysis_structs.h"
#include "shower_analysis_functions.h"
