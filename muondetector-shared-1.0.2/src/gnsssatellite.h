#include <string>
//#include <custom_io_operators.h>
#include <QString>
#include <QDataStream>

#ifndef GNSSSATELLITE_H
#define GNSSSATELLITE_H

struct GnssConfigStruct {
	uint8_t gnssId;
	uint8_t resTrkCh;
	uint8_t maxTrkCh;
	uint32_t flags;
};


class GnssSatellite {
public:
	GnssSatellite() {}
	GnssSatellite(int gnssId, int satId, int cnr, int elev, int azim, float prRes, uint32_t flags)
	: fGnssId(gnssId), fSatId(satId), fCnr(cnr), fElev(elev), fAzim(azim), fPrRes(prRes)
	{
		fQuality = (int)(flags & 0x07);
		if (flags & 0x08) fUsed = true; else fUsed = false;
		fHealth = (int)(flags >> 4 & 0x03);
		fOrbitSource = (flags >> 8 & 0x07);
		fSmoothed = (flags & 0x80);
		fDiffCorr = (flags & 0x40);
	}

	GnssSatellite(int gnssId, int satId, int cnr, int elev, int azim, float prRes,
	 int quality, int health, int orbitSource, bool used, bool diffCorr, bool smoothed)
	: fGnssId(gnssId), fSatId(satId), fCnr(cnr), fElev(elev), fAzim(azim), fPrRes(prRes),
	  fQuality(quality), fHealth(health), fOrbitSource(orbitSource), fUsed(used), fDiffCorr(diffCorr), fSmoothed(smoothed)
	{ }

	//GnssSatellite(const std::string& ubxNavSatSubMessage);

	~GnssSatellite() {}

	static const std::string GNSS_ID_STRING[];

	static void PrintHeader(bool wIndex);
	void Print(bool wHeader);
	void Print(int index, bool wHeader);

	static bool sortByCnr(const GnssSatellite &sat1, const GnssSatellite &sat2)
	{ return sat1.getCnr() > sat2.getCnr(); }


	inline int getCnr() const { return fCnr; }
	
	friend QDataStream& operator << (QDataStream& out, const GnssSatellite& sat);
	friend QDataStream& operator >> (QDataStream& in, GnssSatellite& sat);


public:
	int fGnssId=0, fSatId=0, fCnr=0, fElev=0, fAzim=0;
	float fPrRes=0.;
	int fQuality=0, fHealth=0;
	int fOrbitSource=0;
	bool fUsed=false, fDiffCorr=false, fSmoothed=false;
};

#endif // GNSSSATELLITE_H
