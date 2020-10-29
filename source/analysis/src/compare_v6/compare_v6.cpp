/*                   Stand 18.07.2018
	compare_v5 sortiert und vergleicht die Zeilen mehrerer Textdateien,
	indem es zeitlich korrelierte Eintraege erfasst
	und zusammen mit den Zeitdifferenzen in eine Datei schreibt
	Ziel: mehr als 2 Eingabe-Dateien (mehrere RasPi-Stationen) -> erfuellt
	Ziel: Einstellbarkeit der match-Kriterien (ab wann gelten  -> erfuellt
	Eintraege als Zeitlich korreliert?) und versehen der
	Coincidents mit einem Guetefaktor  -> momentan Guetefaktor == Anzahl Coincidents (an einem Zeitpunkt) -> (erfuellt)
*/
//  Geschrieben von <Marvin.Peter@physik.uni-giessen.de>, Teilelemente sind geklaut von <Lukas.Nies@physik.uni-giessen.de>

//#define USING_MAIN_IN_MAIN_CPP // uncomment to ignore main() in qtcreator

// On Linux compile with 'g++ -std=gnu++11 -o compare compare_v5.cpp -O'
// Vielleicht kann man mit openmp noch etwas mehr optimieren durch multithreading aber eher nicht so viel, weshalb diese Option verworfen wurde

#include "compare_v6.h"

#define DO_NOT_USE_COMPARE_MAIN_CPP

#include <iostream>
#include <sstream>
#include <math.h>
#include <algorithm>
#include <time.h>
#include <iomanip>
#include <limits>
#ifndef DO_NOT_USE_COMPARE_MAIN_CPP
#include <unistd.h>
#endif
#include <fstream>
#include <iterator>
#include <utility>


// max allowed timing error of a single station in ns

void prozentanzeige(unsigned long int prozent, std::string& ausgabe)
{
    unsigned int width = 50;
    unsigned int balken { static_cast<unsigned int>(prozent) * width / 100 };
    char a = '|';
	if (balken > oldBalken)
	{
		std::string prozentbalken = "";
		std::stringstream convert;
		convert << prozent;
		prozentbalken += "\r ";
		prozentbalken += ausgabe;
		prozentbalken += " |";
		if (prozent < 100)
		{
			//sonst length_error
			prozentbalken += std::string(balken, a);
			prozentbalken += std::string(width - balken, ' ');
		}
		else
		{
			prozentbalken += std::string(width, a);
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
		std::cout << prozentbalken << std::flush;// <<endl;
		//cout<<"\r[%3d%%]"<<prozentbalken; 
		oldBalken = balken;
	}
}

long long int ts_diff_ns(timespec& ts1, timespec& ts2)
{
	long long int diff = 0;
	diff = ts2.tv_sec - ts1.tv_sec;
    diff *= static_cast<long long int>(1e9);
	diff += ts2.tv_nsec - ts1.tv_nsec;
	return diff;
}

void skipColumn(std::istream& data, int col)
{
	for (int i = 0; i < col; ++i)
		data.ignore(std::numeric_limits<std::streamsize>::max(), ' '); // col Leerzeichen-getrennte Werte uberspringen
}

bool areTimestampsInOrder(std::string dateiName, bool verbose, unsigned long int& lineCounter, int tsCol)
{   //ueberpruefe ob die Timestamps in der Datei mit Dateinamen der als string uebergeben wird zeitlich geordnet sind
	std::ifstream dateiInputstream(dateiName.c_str());
	if (!dateiInputstream.good())
	{
		std::cout << "failed to read from file (in check Timestamp order section of code)" << std::endl;
		exit(EXIT_FAILURE);
	}
	timespec oldValue = { 0, 0 };
	timespec newValue = { 0, 0 };
	lineCounter = 0;
	bool isInOrder = true;
	std::string oneLine;
	while (!dateiInputstream.eof())
	{
		//skipvalues();
		lineCounter++;
		if (tsCol > 0)
		{
			skipColumn(dateiInputstream, tsCol);
		}
		getline(dateiInputstream, oneLine);
		std::stringstream str(oneLine);
		long int v1, v2;
		char c;

		str >> v1 >> c >> v2;
		newValue.tv_sec = v1;

		while (v2<1000000000L) v2*=10;
		v2/=10;		
		newValue.tv_nsec = v2;

		//cout<<"v1="<<v1<<" v2="<<v2<<endl;

		std::cout.precision(9);
		std::cout << std::fixed;
		//cout<<" diff(old-new)= "<<ts_diff_ns(newValue, oldValue)<<" ns"<<endl;
		if (ts_diff_ns(oldValue, newValue) >= 0)
			//		if (oldValue<=newValue)
		{
			oldValue = newValue;
		}
		else
		{
			if (verbose)
			{
				std::cout << "line " << lineCounter << " in " << dateiName << " is not in order: " << std::endl;
				//cout<<"..."<<endl<<oldValue<<endl<<newValue<<endl<<"..."<<endl<<endl;
			}
			isInOrder = false;
			oldValue = newValue;
		}
	}
	return isInOrder;
}//end of areTimestampsInOrder																							


void readToVector(std::ifstream& inputstream, std::vector<timestamp_t>& oneVector, int& maxValues, int tsCol, int errCol)
{
	// ueberschreibt einen Vektor mit neuen Werten aus der Datei (aus dem inputstream)
	// es werden maximal "maxValues" viele Eintraege hinzugefuegt,
	// es wird gestoppt falls keine weiteren Daten im InputFile sind
	oneVector.clear();
	std::string oneLine;
	timestamp_t oneValue;
    maxValues = abs(maxValues); // maxValues<0 should never be true
    for (int i = 0; i < maxValues; i++)
	{
		if (inputstream.eof())
		{
			break;
		}
		

//		std::string text = "Let me split this into words";
		getline(inputstream, oneLine);
 
		std::istringstream iss(oneLine);
        std::vector<std::string> results((std::istream_iterator<std::string>(iss)),
                                 std::istream_iterator<std::string>());

/*
		for (int i=0; i<results.size(); i++)
			cout<<results[i]<<" ";
		cout<<endl;
*/
		if (results.size()<2) continue;		

		timespec ts1, ts2;
		double v3=0.;
		
		std::string helperString="";
        tsCol<0?tsCol=0:true;
        std::istringstream tokenStream(results[static_cast<unsigned>(tsCol)]);
		std::getline(tokenStream, helperString,'.');
		try {		
			ts1.tv_sec = std::stol(helperString);
			std::getline(tokenStream, helperString,'.');
			bool flag=false;			
			if (helperString.size()<9) flag=true;
			while (helperString.size()<9) helperString+='0';
			ts1.tv_nsec = std::stol(helperString);

			tokenStream.clear();
            tokenStream.str(results[static_cast<unsigned>(tsCol+1)]);
			std::getline(tokenStream, helperString,'.');
			ts2.tv_sec = std::stol(helperString);
			std::getline(tokenStream, helperString,'.');
			if (helperString.size()<9) flag=true;
			while (helperString.size()<9) helperString+='0';		
			ts2.tv_nsec = std::stol(helperString);

            if (errCol >= 0){ // if errCol not set then errCol<0 and then ignore gps timing error estimate
                v3=std::stod(results[static_cast<unsigned>(errCol)]);
            }
//			if (flag) cout<<"read ts1_s="<<ts1.tv_sec<<" ts1_ns="<<ts1.tv_nsec<<" ts2_s="<<ts2.tv_sec<<" ts2_ns="<<ts2.tv_nsec<<" err="<<v3<<endl;
		
			oneValue.ts=ts1;
			oneValue.err = v3;
			oneValue.length = ts_diff_ns(ts1, ts2);
		
			oneVector.push_back(oneValue);

	} catch (...) {	}
	continue;

	}
}// end of readToVector(..																		 )

void Usage()
{
	std::cout << std::flush
	 << "timestamp compare program (v5)\n"
	 << "sort <File1>, ... <File n> unless in chronological order\n"
	 << "Compares <File1>, ... <File n> and searches for coincident timestamps.\n"
	 << "mutual time differences are output under consideration of single timing errors (if available).\n"
	 << "The resulting output is written in format\n"
	 << "[%Y.%m.%d %H:%M:%S] ... [difference1] ... [difference n] ... [coincidents]\n"
	 << "to file <output>. <file1> and <file2> should provide unix timestamps and error in ns (if available):\n"
	 << "[Timestamp] [err]		\n\n"
	 << "Usage:    compare [-vh?][-o<output> -b<interval>] file1 file2 ... filen\n"
	 << "		Optionen:\n"
	 << "		-h?		  : shows this help page.\n"
	 << "		-c 		  : column from which the timestamp is extracted (standard col 0). format is unix timestamp with floating point precision up to 1ns\n"
	 << "		-e 		  : column from which the timing error (in ns) is to be extracted (default = -1  to ignore and assume error=0) \n"
	 << "		-v		  : increase verbosity level\n"
	 << "		-o <output>	  : path/name of the output file\n"
	 << "		-b <time interval>  : coincidence range in [us] (default 1.0us).\n"
	 << "     	-m <maxValues>: number of timestamps which are read into vectors simultaneously. default 10k\n\n"
	 << "written by <Marvin.Peter@physik.uni-giessen.de> and <Hans-Georg.Zaunick@exp2.physik.uni-giessen.de>\n"
	 << "(fragments are reused with permission of <Lukas.Nies@physik.uni-giessen.de>\n"
	 << "---status 18.07.2018---bugs inclusive\n\n"<<std::flush;
}//end of Usage()

void compareAlgorithm(std::vector<unsigned int> it,
	std::vector<std::vector<timestamp_t> >& values,
	double& matchKriterium,
	std::ofstream& output,
	std::vector<unsigned int>& rewriteToVectors)
{
	time_t t;
	/*
		Algorithmus:    - schaue welcher Eintrag am kleinsten ist
						- finde ein Match aus den anderen Vektoren (alle Werte durchgehen), Index merken!
						- nutze aus, dass keine Werte kleiner diesem Match fuer ein Match in Frage kommen
	*/
	rewriteToVectors.clear();
    std::vector<bool> dateiEmpty;
    dateiEmpty.resize(it.size());
    for (unsigned int i = 0; i < it.size(); i++)
	{
		// remember which data vectors in "value" were empty at the beginning
		if (values[i].size() == 0)
		{
			dateiEmpty[i] = true;
		}
		else {
			dateiEmpty[i] = false;
		}
	}
	bool done = false;
	while (!done)
	{
        //welcher hat den kleinsten Wert? (values und it haben die gleiche Laenge)
        //it enthaelt die aktuellen Indizes bei denen die einzelnen Vektoren gelesen werden sollen
		rewriteToVectors.clear();
        for (unsigned int i = 0; i < it.size(); i++)
		{
			if (dateiEmpty[i] == false) {
				// if the vector was not empty at start but is now, it's index is put in "rewriteToVectors"
                if (it[i] >= values[i].size())
				{
					rewriteToVectors.push_back(i);
				}
			}
		}
		if (!rewriteToVectors.empty()) {
			return;
		}
		unsigned int indexSmallest = 0;
        for (unsigned int i = 0; i < it.size(); i++)
		{
			// look for index of first non empty data vector in "values" and set indexSmallest to this value
			// indexSmallest should later be set to the number of the vector (file) that holds the smallest time value at
            // it's individual iterator position right now
			if (dateiEmpty[indexSmallest] == true)
			{
				indexSmallest++;
			}else {
				break;
			}
		}
        if (indexSmallest >= it.size())
		{   // if all vectors in "values" are empty -> all files are empty, return to main program and it should stop soon
			return;
		}
        for (unsigned int i = indexSmallest+1; i < it.size(); i++)
		{
			if (dateiEmpty[i] == false)
			{
                // really look for smallest time value at the it positions of the individual vectors (representing the different files) in "values"
                // then put the index of the vector (file) that holds the smallest time value at it's iterator position
                // (each vector in "values" represents a different file and has it's corresponding iterator in "iterator")
                long long int diff = ts_diff_ns(values[i][it[i]].ts, values[indexSmallest][it[indexSmallest]].ts);
				if (diff > 0)
				{
					indexSmallest = i;
				}
			}
		}
		int coincidents = 0; // coincidents will later show the number of files that participate at a coincident event
        for (unsigned int i = 0; i < it.size(); i++)
		{
			//hier wird entschieden ob es ein Coincidence-Ereignis ist und es wird gleich in die Datei geschrieben
			if (dateiEmpty[i] == false) {
                long long int tdiff = ts_diff_ns(values[i][it[i]].ts, values[indexSmallest][it[indexSmallest]].ts);
                long long int err1 = static_cast<long long int>(values[i][it[i]].err);
                long long int err2 = static_cast<long long int>(values[indexSmallest][it[indexSmallest]].err);
                long long int len1 = static_cast<long long int>(values[i][it[i]].length);
                long long int len2 = static_cast<long long int>(values[indexSmallest][it[indexSmallest]].length);
				
				tdiff = abs(tdiff); // [ns]
                long long int coinc_window = static_cast<long long int>(matchKriterium * 1e9);
				coinc_window += err1+err2;
				
				if ((indexSmallest != i) && (tdiff <= coinc_window) && (err1<MAX_ERR_LIMIT) && (err2<MAX_ERR_LIMIT))
				{
					// if the difference is smaller than or equal as the criterium: a coincident event has been found hooray!
					// since we choose the criterium to be the maximum physically possible time difference for two coincident events
					// in this specific set of data, e.g. time that light travels between the two farthest away detector stations + errors of time measurement,
					// we can increment not only the index of the smallest but also the index of the coincidental files
					// (we don't lose data) -> we only lose data if we are very unlucky
                    // for example: we have 3 files. every file has an individual iterator that always points at the smallest time value that has not already
					// been checked for coincidence. file 1 has the smallest time value. we look at an interval from time 1 to time 1 + some interval.
                    // if nothing can be found -> increment iterator 1
                    // if a coincidence is found -> increment iterators of all at the coincidence participating files. Why?
					// if file 1 and file 3 have coincidence we know that time of file 2 will be greater than time 1 + interval
					// only in the very unlikely case that the real coincidence of the cosmic shower is between file 2 and file 3
					// AND file 1 is a random event exactly in the time range time 3 - interval up to time 3 - (time 2 - time  3)
					// we would lose data
					// so should we increase the interval to exclude this error is a question for another time. To be continued...
					if (coincidents < 1)
					{
                        t = values[indexSmallest][it[indexSmallest]].ts.tv_sec;
                        struct tm *tm = localtime(&t);
						char date[20];
						strftime(date, sizeof(date), "%Y.%m.%d %H:%M:%S", tm);
                        output << date << "  " << values[indexSmallest][it[indexSmallest]].ts.tv_sec << ".";
						output.width(9);
                        output << std::setfill('0') << values[indexSmallest][it[indexSmallest]].ts.tv_nsec << "  ";
						coincidents++;
					}
					rewriteToVectors.push_back(i);
                    output << ts_diff_ns(values[std::min(i,indexSmallest)][it[std::min(i,indexSmallest)]].ts, values[std::max(i,indexSmallest)][it[std::max(i,indexSmallest)]].ts) << "   ";
					//output << indexSmallest << "-" << i <<"  ";
                    output << static_cast<int>(sqrt(err1*err1+err2*err2))<< "    err: "<<err1 << " " << err2 <<" ns  ";
					output << "    len: "<<len1 << " " << len2 <<" ns  ";
					coincidents++;
                    it[i]++;
				}
			}
		}
		if (coincidents > 0)
		{
			output << coincidents << std::endl;
		}
        it[indexSmallest]++;
	}
}//end of Algorithmus																																			  

int compare(std::string outputDateiName, std::vector <std::string> dateiName, double matchKriterium, int maxTimestampsAtOnce, int column1, int errColumn, unsigned long int overallLines, bool verbose)
{
    time_t start, end, algorithmTime;
    int maxTimestampsInVector = maxTimestampsAtOnce / 4;
	// Erstellen der iostreams
	std::vector<std::ifstream*> data;
	for (size_t i = 0; i < dateiName.size(); i++)
	{
		std::ifstream* in = new std::ifstream;
		data.push_back(in);
		(*data[i]).open(dateiName[i].c_str());
        //data enthaelt alle ifstreams, einen fuer jede Datei
	}
	std::ofstream output(outputDateiName.c_str());
	output.precision(9);
	output << std::fixed;

	//Prozentanzeige:
	unsigned long int prozent = 0;
	unsigned long int prozentCounter = 0;
	unsigned long int oldCounter = 0;
	std::string ausgabeAlgorithmString = "comparing...";
	//Prozentanzeige


	if (verbose)
	{
		std::cout << "\nlaunch comparison of " << overallLines << " timestamps in " << dateiName.size() <<" files....\n\n" << std::flush;
	}
	std::vector<std::vector<timestamp_t> > values;
	values.resize(dateiName.size());
	//values [index der Datei] [werte in timespec]
    std::vector<unsigned int> it;
	std::vector<unsigned int> rewriteToVectors;
	for (unsigned int i = 0; i < dateiName.size(); i++)
	{
        it.push_back(0);
        //hier werden die Vectors mit einer "festen Laenge" also Anzahl der Dateien gebaut
        //it und der aeusserste Container von "values" sind korelliert mit dem "dateiName" vector,
        //haben also die gleiche Anzahl Eintraege wie es Dateien gibt
        //it gibt an, welcher index in der jeweiligen Datei (an welcher stelle in Datei i gerade gelesen wird)
	}
    start = time(nullptr);
	prozentanzeige(0, ausgabeAlgorithmString);

	for (unsigned int i = 0; i < dateiName.size(); i++)
	{
		readToVector(*data[i], values[i], maxTimestampsInVector, column1, errColumn);
	}
    compareAlgorithm(it, values, matchKriterium, output, rewriteToVectors);
	try
	{
		prozentCounter++;
		if (0.5*prozentCounter / maxTimestampsInVector > 0.5*oldCounter / maxTimestampsInVector)
		{

            prozent = static_cast<unsigned long int>(static_cast<long double>(100.0*prozentCounter)
                                                     *(maxTimestampsInVector / static_cast<long double>(overallLines)));
			prozentanzeige(prozent, ausgabeAlgorithmString);
			oldCounter = prozentCounter;
		}
	}
	catch (int e)
	{
        std::cout << "percent counter error: " << prozentCounter << " error code "<<e<<'\n'<<std::flush;
		exit(EXIT_FAILURE);

	}
	// bis hier hin: alle Vektoren vollschreiben und ein mal durchlaufen lassen

	// ab hier: Schleife wird so oft ausgefuehrt bis alle Dateien keine Eintraege mehr haben
	// gleichzeitig Einlesen und bearbeiten im Optimalfall
	// vom Algorithmus erhaelt man immer die Information, welcher Vektor als erstes abgearbeitet ist,
	// dieser wird dann sofort ersetzt und mit diesem und den anderen Vektoren weitergemacht,
	// der abgearbeitete Vektor wird neu vollgeschrieben mit Werten aus der Datei
	bool allDataEOF = false;
	while (!allDataEOF)
	{
		for (auto i : rewriteToVectors) {
            it[i] = 0;
			readToVector(*data[i], values[i], maxTimestampsInVector, column1, errColumn);
		}
        compareAlgorithm(it, values, matchKriterium, output, rewriteToVectors);
		try
		{
			prozentCounter++;
			if (0.5*prozentCounter / maxTimestampsInVector > 0.5*oldCounter / maxTimestampsInVector)
			{

                prozent = static_cast<unsigned long int>(static_cast<long double>(100.0*prozentCounter)
                                                         *(maxTimestampsInVector / static_cast<long double>(overallLines)));
				prozentanzeige(prozent, ausgabeAlgorithmString);
				oldCounter = prozentCounter;
			}
		}
		catch (int e)
		{
            std::cout << "percent counter error: " << prozentCounter << " error code "<<e<<'\n'<<std::flush;
			exit(EXIT_FAILURE);

		}
		allDataEOF = true;
		for (unsigned int i = 0; i < data.size(); i++)
		{
			if (!(*data[i]).eof())
			{
				allDataEOF = false;
			}
		}
	} //end of while
	// ab hier: letzte reste die noch in den Vektoren sind, werden abgearbeitet
    for (size_t i = 0; i < dateiName.size() * 2; i++)
	{
		for (auto j : rewriteToVectors) {
            it[j] = 0;
			readToVector(*data[j], values[j], maxTimestampsInVector, column1, errColumn);
		}
        compareAlgorithm(it, values, matchKriterium, output, rewriteToVectors);
		try
		{
			prozentCounter++;
			if (0.5*prozentCounter / maxTimestampsInVector > 0.5*oldCounter / maxTimestampsInVector)
			{

                prozent = static_cast<unsigned long int>(static_cast<long double>(100.0*prozentCounter)
                                                         *(maxTimestampsInVector / static_cast<long double>(overallLines)));
				prozentanzeige(prozent, ausgabeAlgorithmString);
				oldCounter = prozentCounter;
			}
		}
		catch (int e)
		{
            std::cout << "percent counter error: " << prozentCounter << " error code "<<e<<'\n'<<std::flush;
			exit(EXIT_FAILURE);
		}
	}//end of for  
	prozentanzeige(100, ausgabeAlgorithmString);
	std::cout <<"\n\n";
    end = time(nullptr);
	algorithmTime = end - start;
	if (verbose)
	{
		std::cout << "finalized program. duration: " << algorithmTime << "s\n";
	}
    return static_cast<int>(true);
}// end of compare

#ifndef DO_NOT_USE_COMPARE_MAIN_CPP
int main(int argc, char*argv[])
{
    std::string outputDateiName = "";
    std::vector <std::string> dateiName;
    time_t start;
	time_t end;
	time_t checkTimestampOrderTime;
    double matchKriterium = 0.001;//[us]
    int maxTimestampsAtOnce = 10000;

    // Einlesen der Zusatzinformationen aus der Konsole
    bool verbose = false;
    int ch;
    int column1 = 0;
    int errColumn = -1;
//	int column2 = 0;
    double b = 1.0;
    bool notSorted = true;
    while ((ch = getopt(argc, argv, "svm:c:e:o:b:h?")) != EOF)
    {
        switch (static_cast<char>(ch))
        {
        case 's':  notSorted = false;
        [[fallthrough]]; case 'o':
            outputDateiName = optarg;
            //printf("Output wurde gewaehlt. %s.c_str()\n", out.c_str());
            break;
        case 'h':
            Usage();
            return -1;
            break;
        case 'v': verbose = true;
            break;
        case 'b': b = atof(optarg);
            break;
        case 'c': column1 = atoi(optarg); //momentan nicht individuell fuer jede Datei
            break;
        case 'e': errColumn = atoi(optarg); //momentan nicht individuell fuer jede Datei
            break;
        case 'm': maxTimestampsAtOnce = atoi(optarg);
            break;
        }
    }
    if (outputDateiName.empty()) {
        perror("no output file selected!\n");
        Usage();
        return -1;
    }
    
    matchKriterium = b * 1e-6; //[us]
    if (argc - optind < 2)
    {
        perror("wrong input. too few arguments!\n");
        Usage();
        return -1;
    }
    if (verbose)
    {
        std::cout << "launched program....\n"
        		<< "\nreading at most " << maxTimestampsAtOnce << " values at once\n\n"
        		<< "coincidence window: " << b << " us\n"<<std::flush;
    }
    size_t index = 0;
    for (int i { optind }; i < argc; i++)
    {
        //
        dateiName.push_back("");
        std::string temp = argv[i];
        dateiName[index] = temp;
        if (verbose)
        {
            std::cout << index << ". file name is " << dateiName[index].c_str() << ".\n";
        }
        index++;
    }


    // Ueberpruefung der Dateien auf Reihenfolge
    std::cout << "\ncheck files....\n"<<std::flush;
    if (verbose)
    {
        std::cout<<"\n\n";
    }

    start = time(nullptr);
    std::vector<unsigned long int> lines;
    if (notSorted)
    {
        for (unsigned int i = 0; i < dateiName.size(); i++)
        {
            lines.push_back(0);
            if (!areTimestampsInOrder(dateiName[i], verbose, lines[i], column1))
            {
                if (verbose)
                {
                    // falls die Timestamps nicht in der richtigen Reihenfolge gespeichert sind,
                    // werden sie automatisch mit Unix sort sortiert, ob dann auch gleich ueberschrieben
                    // laesst sich einstellen, momentan ja
                    std::cout << "sort " << dateiName[i] << " with Unix sort\n\n";
                }
                std::string systemOut = "sort -n ";
                systemOut += dateiName[i];
                systemOut += " > ";
                systemOut += "tmp";
                char *outs = &systemOut[0];
                system(outs);
                systemOut = "mv -v tmp ";
                systemOut += dateiName[i];
                outs = &systemOut[0];
                system(outs);
            }
        }
    }
    else {
        // lese oder schaetze zeilenmenge!!!
        for (unsigned int i = 0; i < dateiName.size(); i++)
        {
            lines[i] = 10000;

        }
    }
    unsigned long int overallLines = 0;
    for (unsigned int i = 0; i < lines.size(); i++)
    {
        // zaehlt alle Zeilen in allen Dateien (schon waehrend der Reihenfolgen-Ueberpruefung
        // und addiert sie. Ist wichtig fuer den Ladebalken
        overallLines += lines[i];
    }
    end = time(nullptr);
    checkTimestampOrderTime = end - start;
    if (verbose)
    {
        std::cout << "\ncheck for sort order concluded. duration: " << checkTimestampOrderTime << "s\n";
    }
    return compare(outputDateiName,dateiName,matchKriterium,maxTimestampsAtOnce,column1,errColumn,overallLines,verbose);
}//end of int main()
#endif
