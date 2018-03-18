#include "gnsssatellite.h"
using namespace std;

bool GnssSatellite::sortByCnr(const GnssSatellite &sat1, const GnssSatellite &sat2)
{
	return sat1.getCnr() > sat2.getCnr();
}




GnssSatellite::GnssSatellite(const std::string& ubxNavSatSubMessage) {
	const string mess(ubxNavSatSubMessage);

	fGnssId = mess[0];
	fSatId = mess[1];
	fCnr = mess[2];
	fElev = mess[3];
	fAzim = mess[4];
	fAzim += mess[5] << 8;
	int16_t prRes = mess[6];
	prRes += mess[7] << 8;
	fPrRes = prRes / 10.;

	fFlags = (int)mess[8];
	fFlags += ((int)mess[9]) << 8;
	fFlags += ((int)mess[10]) << 16;
	fFlags += ((int)mess[11]) << 24;

	fQuality = (int)(fFlags & 0x07);
	if (fFlags & 0x08) fUsed = true; else fUsed = false;
	fHealth = (int)(fFlags >> 4 & 0x03);
	fOrbitSource = (fFlags >> 8 & 0x07);
	fSmoothed = (fFlags & 0x80);
	fDiffCorr = (fFlags & 0x40);
}

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
	//    cout<<setfill(' ');
	cout << setw(3) << (int)fCnr << "      " << setw(3) << (int)fElev << "       " << setw(3) << (int)fAzim;
	cout << "   " << setw(6) << fPrRes << "    " << fQuality << "   " << string((fUsed) ? "Y" : "N");
	cout << "    " << fHealth << "   " << fOrbitSource << "   " << (int)fSmoothed << "    " << (int)fDiffCorr;;
	cout << endl;
}
const string GnssSatellite::GNSS_ID_STRING[] = { " GPS","SBAS"," GAL","BEID","IMES","QZSS","GLNS" };
