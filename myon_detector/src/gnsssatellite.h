#include <string>
#include "custom_io_operators.h"

#ifndef GNSSSATELLITE_H
#define GNSSSATELLITE_H

class GnssSatellite {
public:
	GnssSatellite();
    GnssSatellite(int gnssId, int satId, int cnr, int elev, int azim, float prRes, uint32_t flags)
    {
        fGnssId = gnssId;
        fSatId = satId;
        fCnr = cnr;
        fElev = elev;
        fAzim = azim;
        fPrRes = prRes;
        fFlags = flags;
    }

	GnssSatellite(const std::string& ubxNavSatSubMessage);

	~GnssSatellite() {}

	static const std::string GNSS_ID_STRING[];

	static void PrintHeader(bool wIndex);
	void Print(bool wHeader);
	void Print(int index, bool wHeader);

	static bool sortByCnr(const GnssSatellite &sat1, const GnssSatellite &sat2);

	inline int getCnr() const { return fCnr; }

private:
	int fGnssId, fSatId, fCnr, fElev, fAzim;
	uint32_t fFlags;
	int fQuality, fHealth;
	bool fUsed, fDiffCorr, fSmoothed;
	float fPrRes;
	int fOrbitSource;
};

#endif // GNSSSATELLITE_H
