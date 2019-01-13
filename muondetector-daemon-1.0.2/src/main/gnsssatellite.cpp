#include <gnsssatellite.h>
using namespace std;


void GnssSatellite::PrintHeader(bool wIndex)
{
	if (wIndex) {
		cout << "   ----------------------------------------------------------------------------------" << endl;
		cout << "   Nr   Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr" << endl;
		cout << "   ----------------------------------------------------------------------------------" << endl;
	}
	else {
		cout << "   -----------------------------------------------------------------" << endl;
		cout << "    Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr" << endl;
		cout << "   -----------------------------------------------------------------" << endl;
	}
}


void GnssSatellite::Print(bool wHeader)
{
	if (wHeader) {
		cout << "   ------------------------------------------------------------------------------" << endl;
		cout << "    Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr" << endl;
		cout << "   ------------------------------------------------------------------------------" << endl;
	}
	cout << "   " << dec << "  " << GNSS_ID_STRING[(int)fGnssId] << "   " << setw(3) << (int)fSatId << "    ";
//	cout << "   " << dec << "  " << (int)fGnssId << "   " << setw(3) << (int)fSatId << "    ";
	//    cout<<setfill(' ');
	cout << setw(3) << (int)fCnr << "      " << setw(3) << (int)fElev << "       " << setw(3) << (int)fAzim;
	cout << "   " << setw(6) << fPrRes << "    " << fQuality << "   " << string((fUsed) ? "Y" : "N");
	cout << "    " << fHealth << "   " << fOrbitSource << "   " << (int)fSmoothed << "    " << (int)fDiffCorr;
	cout << endl;
}

void GnssSatellite::Print(int index, bool wHeader)
{
	if (wHeader) {
		cout << "   ----------------------------------------------------------------------------------" << endl;
		cout << "   Nr   Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr" << endl;
		cout << "   ----------------------------------------------------------------------------------" << endl;
	}
	cout << "   " << dec << setw(2) << index + 1 << "  " << GNSS_ID_STRING[(int)fGnssId] << "   " << setw(3) << (int)fSatId << "    ";
//	cout << "   " << dec << setw(2) << index + 1 << "  " << (int)fGnssId << "   " << setw(3) << (int)fSatId << "    ";
	//    cout<<setfill(' ');
	cout << setw(3) << (int)fCnr << "      " << setw(3) << (int)fElev << "       " << setw(3) << (int)fAzim;
	cout << "   " << setw(6) << fPrRes << "    " << fQuality << "   " << string((fUsed) ? "Y" : "N");
	cout << "    " << fHealth << "   " << fOrbitSource << "   " << (int)fSmoothed << "    " << (int)fDiffCorr;;
	cout << endl;
}


const string GnssSatellite::GNSS_ID_STRING[] = { " GPS","SBAS"," GAL","BEID","IMES","QZSS","GLNS"," N/A" };
