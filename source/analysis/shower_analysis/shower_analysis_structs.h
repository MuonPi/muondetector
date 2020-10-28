//
//  SHOWER ANALYSIS SOFTWARE 
//
//  STRUCTURES File
//
//  Written by: 
//    Lukas Nies <Lukas.Nies@Physik.uni-giessen.de> 
//    +

struct hist_struct_TH1D
{
  TF1 *fit;
  TH1D *hist;
  vector<Double_t> params;
};

struct hist_struct_TH2D
{
  TF1 *fit;
  TH2D *hist;
  vector<Double_t> params;
};
