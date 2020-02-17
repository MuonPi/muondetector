#ifndef MUONDETECTOR_STRUCTS_H
#define MUONDETECTOR_STRUCTS_H

#include <muondetector_shared_global.h>
#include <sys/types.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <QString>
#include <QDataStream>
#include <QList>


struct CalibStruct {
public:
    enum {	CALIBFLAGS_NO_CALIB=0x00, CALIBFLAGS_COMPONENTS=0x01,
            CALIBFLAGS_VOLTAGE_COEFFS=0x02, CALIBFLAGS_CURRENT_COEFFS=0x04};

    enum {	FEATUREFLAGS_NONE=0x00, FEATUREFLAGS_GNSS=0x01,
            FEATUREFLAGS_ENERGY=0x02, FEATUREFLAGS_DETBIAS=0x04, FEATUREFLAGS_PREAMP_BIAS=0x08};

    CalibStruct()=default;
    CalibStruct(const std::string& a_name, const std::string& a_type, uint8_t a_address, const std::string& a_value)
    : name(a_name), type(a_type), address(a_address), value(a_value)
    { }
    ~CalibStruct()=default;
    CalibStruct(const CalibStruct& s)
    : name(s.name), type(s.type), address(s.address), value(s.value)
    { }
    //friend QDataStream& operator << (QDataStream& out, const CalibStruct& calib);
    //friend QDataStream& operator >> (QDataStream& in, CalibStruct& calib);
    std::string name="";
    std::string type="";
    uint16_t address=0;
    std::string value="";
};


struct GeodeticPos {
    uint32_t iTOW;
    int32_t lon; // longitude 1e-7 scaling (increase by 1 means 100 nano degrees)
    int32_t lat; // latitude 1e-7 scaling (increase by 1 means 100 nano degrees)
    int32_t height; // height above ellipsoid
    int32_t hMSL; // height above main sea level
    uint32_t hAcc; // horizontal accuracy estimate
    uint32_t vAcc; // vertical accuracy estimate
};


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

	//static const std::string GNSS_ID_STRING[];

	static void PrintHeader(bool wIndex);
	void Print(bool wHeader) const;
	void Print(int index, bool wHeader) const;

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

//inline const std::string GnssSatellite::GNSS_ID_STRING[] = { " GPS","SBAS"," GAL","BEID","IMES","QZSS","GLNS"," N/A" };
//const MUONDETECTORSHARED std::string GNSS_ID_STRING[] = { " GPS","SBAS"," GAL","BEID","IMES","QZSS","GLNS"," N/A" };
//const MUONDETECTORSHARED std::string GnssSatellite::GNSS_ID_STRING[];
//inline const std::string GNSS_ID_STRING()
static const QList<QString> GNSS_ID_STRING = { " GPS","SBAS"," GAL","BEID","IMES","QZSS","GLNS"," N/A" };
//enum {  };


inline void GnssSatellite::PrintHeader(bool wIndex)
{
	if (wIndex) {
		std::cout << "   ----------------------------------------------------------------------------------" << std::endl;
		std::cout << "   Nr   Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr" << std::endl;
		std::cout << "   ----------------------------------------------------------------------------------" << std::endl;
	}
	else {
		std::cout << "   -----------------------------------------------------------------" << std::endl;
		std::cout << "    Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr" << std::endl;
		std::cout << "   -----------------------------------------------------------------" <<std:: endl;
	}
}


inline void GnssSatellite::Print(bool wHeader) const
{
	if (wHeader) {
		std::cout << "   ------------------------------------------------------------------------------" << std::endl;
		std::cout << "    Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr" << std::endl;
		std::cout << "   ------------------------------------------------------------------------------" << std::endl;
	}
	std::cout << "   " << std::dec << "  " << GNSS_ID_STRING[(int)fGnssId].toStdString() << "   " << std::setw(3) << (int)fSatId << "    ";
//	cout << "   " << dec << "  " << (int)fGnssId << "   " << setw(3) << (int)fSatId << "    ";
	//    cout<<setfill(' ');
	std::cout << std::setw(3) << (int)fCnr << "      " << std::setw(3) << (int)fElev << "       " << std::setw(3) << (int)fAzim;
	std::cout << "   " << std::setw(6) << fPrRes << "    " << fQuality << "   " << std::string((fUsed) ? "Y" : "N");
	std::cout << "    " << fHealth << "   " << fOrbitSource << "   " << (int)fSmoothed << "    " << (int)fDiffCorr;
	std::cout << std::endl;
}

inline void GnssSatellite::Print(int index, bool wHeader) const
{
	if (wHeader) {
		std::cout << "   ----------------------------------------------------------------------------------" << std::endl;
		std::cout << "   Nr   Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr" << std::endl;
		std::cout << "   ----------------------------------------------------------------------------------" << std::endl;
	}
	std::cout << "   " << std::dec << std::setw(2) << index + 1 << "  " << GNSS_ID_STRING[(int)fGnssId].toStdString() << "   " << std::setw(3) << (int)fSatId << "    ";
//	cout << "   " << dec << setw(2) << index + 1 << "  " << (int)fGnssId << "   " << setw(3) << (int)fSatId << "    ";
	//    cout<<setfill(' ');
	std::cout << std::setw(3) << (int)fCnr << "      " << std::setw(3) << (int)fElev << "       " << std::setw(3) << (int)fAzim;
	std::cout << "   " << std::setw(6) << fPrRes << "    " << fQuality << "   " << std::string((fUsed) ? "Y" : "N");
	std::cout << "    " << fHealth << "   " << fOrbitSource << "   " << (int)fSmoothed << "    " << (int)fDiffCorr;;
	std::cout << std::endl;
}






//extern inline QDataStream& operator << (QDataStream& out, const CalibStruct& calib);
//extern inline QDataStream& operator >> (QDataStream& in, CalibStruct& calib);
inline QDataStream& operator << (QDataStream& out, const CalibStruct& calib)
{
    out << QString::fromStdString(calib.name) << QString::fromStdString(calib.type)
     << (quint16)calib.address << QString::fromStdString(calib.value);
    return out;
}


inline QDataStream& operator >> (QDataStream& in, CalibStruct& calib)
{
    QString s1,s2,s3;
    quint16 u;
    in >> s1 >> s2;
    in >> u;
    in >> s3;
    calib.name = s1.toStdString();
    calib.type = s2.toStdString();
    calib.address = (uint16_t)u;
    calib.value = s3.toStdString();
    return in;
}

#endif // MUONDETECTOR_STRUCTS_H
