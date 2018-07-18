/*                   Stand 18.07.2018
	compare_v5 sortiert und vergleicht die Zeilen mehrerer Textdateien,
	indem es zeitlich korrelierte Eintraege erfasst
	und zusammen mit den Zeitdifferenzen in eine Datei schreibt
	Ziel: mehr als 2 Eingabe-Dateien (mehrere RasPi-Stationen) -> erfuellt
	Ziel: Einstellbarkeit der match-Kriterien (ab wann gelten  -> erfuellt
	Einträge als Zeitlich korreliert?) und versehen der
	Coincidents mit einem Guetefaktor  -> momentan Guetefaktor == Anzahl Coincidents (an einem Zeitpunkt) -> (erfuellt)
*/
//  Geschrieben von <Marvin.Peter@physik.uni-giessen.de>, Teilelemente sind geklaut von <Lukas.Nies@physik.uni-giessen.de>

// On Linux compile with 'g++ -o compare_v5 compare_v5.cpp -O'
// Vielleicht kann man mit openmp noch etwas mehr optimieren durch multithreading aber eher nicht so viel, weshalb diese Option verworfen wurde
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <math.h>
#include <time.h>
#include <iomanip>
#include <limits>
#include <unistd.h>
#include <fstream>
#include <utility>

using namespace std;

struct timestamp_t {
	timespec ts;
	double err;
};


unsigned long int oldBalken = 0;
void prozentanzeige(unsigned long int prozent, string& ausgabe)
{
	int width = 50;
	int balken;    char a = '|';
	balken = prozent * width / 100;
	if (balken > oldBalken)
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

long long int ts_diff_ns(timespec& ts1, timespec& ts2)
{
	long long int diff = 0;
	diff = ts2.tv_sec - ts1.tv_sec;
	diff *= 1e9;
	diff += ts2.tv_nsec - ts1.tv_nsec;
	return diff;
}

void skipColumn(istream& data, int col)
{
	for (int i = 0; i < col; ++i)
		data.ignore(numeric_limits<streamsize>::max(), ' '); // col Leerzeichen-getrennte Werte uberspringen
}

bool areTimestampsInOrder(string dateiName, bool verbose, unsigned long int& lineCounter, int tsCol)
{   //ueberpruefe ob die Timestamps in der Datei mit Dateinamen der als string uebergeben wird zeitlich geordnet sind
	ifstream dateiInputstream(dateiName.c_str());
	if (!dateiInputstream.good())
	{
		cout << "failed to read from file (in check Timestamp order section of code)" << endl;
		exit(EXIT_FAILURE);
	}
	timespec oldValue = { 0, 0 };
	timespec newValue = { 0, 0 };
	lineCounter = 0;
	bool isInOrder = true;
	string oneLine;
	while (!dateiInputstream.eof())
	{
		//skipvalues();
		lineCounter++;
		if (tsCol > 0)
		{
			skipColumn(dateiInputstream, tsCol);
		}
		getline(dateiInputstream, oneLine);
		stringstream str(oneLine);
		long int v1, v2;
		char c;
		str >> v1 >> c >> v2;
		newValue.tv_sec = v1;
		newValue.tv_nsec = v2;

		//cout<<"v1="<<v1<<" v2="<<v2<<endl;

		cout.precision(9);
		cout << fixed;
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
				cout << "Line " << lineCounter << " in " << dateiName << " is not in order: " << endl;
				//cout<<"..."<<endl<<oldValue<<endl<<newValue<<endl<<"..."<<endl<<endl;
			}
			isInOrder = false;
			oldValue = newValue;
		}
	}
	return isInOrder;
}//end of areTimestampsInOrder																							


void readToVector(ifstream& inputstream, vector<timestamp_t>& oneVector, int& maxValues, int tsCol, int errCol)
{
	// ueberschreibt einen Vektor mit neuen Werten aus der Datei (aus dem inputstream)
	// es werden maximal "maxValues" viele Eintraege hinzugefuegt,
	// es wird gestoppt falls keine weiteren Daten im InputFile sind
	oneVector.clear();
	string oneLine;
	timestamp_t oneValue;
	for (unsigned int i = 0; i < maxValues; i++)
	{
		if (inputstream.eof())
		{
			break;
		}
		
		if (errCol<0 && tsCol > 0)
		{
			skipColumn(inputstream, tsCol);
		} else if (errCol>=0 && tsCol > 0)
		{
			skipColumn(inputstream, min(errCol, tsCol));
		}
		getline(inputstream, oneLine);
		stringstream str(oneLine);
		long int v1, v2;
		double v3=0.;
		char c;
		
		if (errCol<0) str >> v1 >> c >> v2;
		else if (errCol>=0 && errCol<tsCol)	{
			str >> v3;
			string dummystring;
			for (int i=0; i<abs(errCol-tsCol)-1; i++) str >> dummystring;
			str >> v1 >> c >> v2;
		}
		else if (errCol>=0 && errCol>tsCol) {
			str >> v1 >> c >> v2;
			string dummystring;
			for (int i=0; i<abs(errCol-tsCol)-1; i++) str >> dummystring;
			str >> v3;
		}
		
		//cout<<"read ts_i="<<v1<<" ts_ns="<<v2<<" err="<<v3<<endl;
		oneValue.ts.tv_sec = v1;
		oneValue.ts.tv_nsec = v2;
		oneValue.err = v3;

		oneVector.push_back(oneValue);
	}
}// end of readToVector(..																		 )

void Usage()
{
	cout << endl;
	cout << "Compare_v5" << endl;
	cout << "Sortiert <File1>, ... <File n> falls nicht schon sortiert." << endl;
	cout << "Vergleicht <File1>, ... <File n> jeweils miteinander und sucht nach sog. coincidents" << endl;
	cout << "Timestamps und bildet aus beiden die Differenz. Das Ergebnis wird in" << endl;
	cout << "der Form:" << endl;
	cout << "[%Y.%m.%d %H:%M:%S] ... [Differenz1] ... [Differenz n] ... [coincidents]" << endl;
	cout << "in <output> gespeichert. <File1> und <File2> sollte die Form haben:" << endl;
	cout << "[Timestamp]		" << endl << endl;
	cout << "Benutzung:    " << "compare" << " [-vh?][-o<output> -b<Bereich>] File1 File2" << endl;
	cout << "		Optionen:" << endl;
	cout << "		-h?		  : shows this help page." << endl;
	cout << "		-c 		  : column from which the timestamp is extracted (standard col 0). format is unix timestamp with floating point precision up to 1ns" << endl;
	cout << "		-e 		  : column from which the timing error (in ns) is to be extracted (default = -1  to ignore and assume error=0) " << endl;
	cout << "		-v		  : increase verbosity level" << endl;
	cout << "		-o <output>	  : path/name of the output file" << endl;
	cout << "		-b <time interval>  : coincidence range in [us] (default 1.0us)." << endl;
	cout << "     	-m <maxValues>: number of timestamps which are read into vectors simultaneously. default 10k" << endl;
	cout << endl;
	cout << "Geschrieben von <Marvin.Peter@physik.uni-giessen.de>" << endl;
	cout << "(Teilelemente sind geklaut von <Lukas.Nies@physik.uni-giessen.de>" << endl;
	cout << "---Stand 18.07.2018---Bugs inclusive" << endl << endl;
}//end of Usage()

void compareAlgorithm(vector<unsigned int>& iterator,
	vector<vector<timestamp_t> >& values,
	double& matchKriterium,
	ofstream& output,
	vector<unsigned int>& rewriteToVectors)
{
	time_t t;
	/*
		Algorithmus:    - schaue welcher Eintrag am kleinsten ist
						- finde ein Match aus den anderen Vektoren (alle Werte durchgehen), Index merken!
						- nutze aus, dass keine Werte kleiner diesem Match fuer ein Match in Frage kommen
	*/
	rewriteToVectors.clear();
	bool dateiEmpty[iterator.size()];
	for (unsigned int i = 0; i < iterator.size(); i++)
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
		//welcher hat den kleinsten Wert? (values und iterator haben die gleiche Laenge)
		//iterator enthaelt die aktuellen Indizes bei denen die einzelnen Vektoren gelesen werden sollen
		rewriteToVectors.clear();
		for (unsigned int i = 0; i < iterator.size(); i++)
		{
			if (dateiEmpty[i] == false) {
				// if the vector was not empty at start but is now, it's index is put in "rewriteToVectors"
				if (iterator[i] >= values[i].size())
				{
					rewriteToVectors.push_back(i);
				}
			}
		}
		if (!rewriteToVectors.empty()) {
			return;
		}
		unsigned int indexSmallest = 0;
		for (unsigned int i = 0; i < iterator.size(); i++)
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
		if (indexSmallest >= iterator.size())
		{   // if all vectors in "values" are empty -> all files are empty, return to main program and it should stop soon
			return;
		}
		for (unsigned int i = indexSmallest+1; i < iterator.size(); i++)
		{
			if (dateiEmpty[i] == false)
			{
				// really look for smallest time value at the iterator positions of the individual vectors (representing the different files) in "values"
				// then put the index of the vector (file) that holds the smallest time value at it's iterator position
				// (each vector in "values" represents a different file and has it's corresponding iterator in "iterator")
				long long int diff = ts_diff_ns(values[i][iterator[i]].ts, values[indexSmallest][iterator[indexSmallest]].ts);
				if (diff > 0)
				{
					indexSmallest = i;
				}
			}
		}
		int coincidents = 0; // coincidents will later show the number of files that participate at a coincident event
		for (unsigned int i = 0; i < iterator.size(); i++)
		{
			//hier wird entschieden ob es ein Coincidence-Ereignis ist und es wird gleich in die Datei geschrieben
			if (dateiEmpty[i] == false) {
				long long int tdiff = ts_diff_ns(values[i][iterator[i]].ts, values[indexSmallest][iterator[indexSmallest]].ts);
				long long int err1 = values[i][iterator[i]].err;
				long long int err2 = values[indexSmallest][iterator[indexSmallest]].err;
				
				tdiff = abs(tdiff); // [ns]
				long long int coinc_window = matchKriterium * 1e9;
				coinc_window += err1+err2;
				
				if ((indexSmallest != i) && (tdiff <= coinc_window))
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
						t = values[indexSmallest][iterator[indexSmallest]].ts.tv_sec;
						struct tm *tm = localtime(&t);
						char date[20];
						strftime(date, sizeof(date), "%Y.%m.%d %H:%M:%S", tm);
						output << date << "  " << values[indexSmallest][iterator[indexSmallest]].ts.tv_sec << ".";
						output.width(9);
						output << setfill('0') << values[indexSmallest][iterator[indexSmallest]].ts.tv_nsec << "  ";
						coincidents++;
					}
					rewriteToVectors.push_back(i);
					output << -ts_diff_ns(values[i][iterator[i]].ts, values[indexSmallest][iterator[indexSmallest]].ts) << "   ";
					output << indexSmallest << "-" << i <<"  ";
					output << "(+-"<<err1 << "/" << err2 <<"ns)  ";
					coincidents++;
					iterator[i]++;
				}
			}
		}
		if (coincidents > 0)
		{
			output << coincidents << endl;
		}
		iterator[indexSmallest]++;
	}
}//end of Algorithmus																																			  

int main(int argc, char*argv[])
{
	string outputDateiName;
	vector <string> dateiName;
	time_t start, end, readToVectorTime, checkTimestampOrderTime, algorithmTime;
	double matchKriterium = 0.001;//[us]
	timestamp_t oneValue;
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
		switch ((char)ch)
		{
		case 's':  notSorted = false;
		case 'o':  outputDateiName = optarg;
			//printf("Output wurde gewaehlt. %s.c_str()\n", out.c_str());
			break;
		case 'h': Usage();
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
	int maxTimestampsInVector = maxTimestampsAtOnce / 4;
	matchKriterium = b * 1e-6; //[us]
	if (argc - optind < 2)
	{
		perror("Falsche Eingabe, zu wenige Argumente!\n");
		Usage();
		return -1;
	}
	if (verbose)
	{
		cout << "Programm gestarted...." << endl;
		cout << endl << "lese maximal " << maxTimestampsAtOnce << " Werte gleichzeitig ein" << endl << endl;
		cout << "Koinzidenzfenster: " << b << " us" << endl;
	}
	int index = 0;
	for (int i = optind; i < argc; i++)
	{
		//
		dateiName.push_back("");
		string temp = argv[i];
		dateiName[index] = temp;
		if (verbose)
		{
			cout << index << ". Dateiname ist " << dateiName[index].c_str() << "." << endl;
		}
		index++;
	}


	// Ueberpruefung der Dateien auf Reihenfolge
	cout << endl << "check files...." << flush;
	if (verbose)
	{
		cout << endl << endl;
	}

	start = time(0);
	vector<unsigned long int> lines;
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
					cout << "sort " << dateiName[i] << " with Unix sort" << endl << endl;
				}
				string systemOut = "sort -g ";
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
		// lese oder schätze zeilenmenge!!!
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
	end = time(0);
	checkTimestampOrderTime = end - start;
	if (verbose)
	{
		cout << endl << "Ueberpruefung auf reihenfolge abgeschlossen, Dauer: " << checkTimestampOrderTime << "s" << endl;
	}

	// Erstellen der iostreams
	vector<ifstream*> data;
	for (size_t i = 0; i < dateiName.size(); i++)
	{
		ifstream* in = new ifstream;
		data.push_back(in);
		(*data[i]).open(dateiName[i].c_str());
		//data enthaelt alle ifstreams, einen fuer jede Datei
	}
	ofstream output(outputDateiName.c_str());
	output.precision(9);
	output << fixed;

	//Prozentanzeige:
	unsigned long int prozent = 0;
	unsigned long int prozentCounter = 0;
	unsigned long int oldCounter = 0;
	string ausgabeAlgorithmString = "vergleiche...";
	//Prozentanzeige


	if (verbose)
	{
		cout << endl << "Starte Vergleich von " << overallLines << " Timestamps in " << dateiName.size() <<" Files...." << endl << endl;
	}
	vector<vector<timestamp_t> > values;
	values.resize(dateiName.size());
	//values [index der Datei] [werte in timespec]
	vector<unsigned int> iterator;
	vector<unsigned int> rewriteToVectors;
	for (unsigned int i = 0; i < dateiName.size(); i++)
	{
		iterator.push_back(0);
		//hier werden die Vectors mit einer "festen Laenge" also Anzahl der Dateien gebaut
		//iterator und der aeußerste Container von "values" sind korelliert mit dem "dateiName" vector,
		//haben also die gleiche Anzahl Eintraege wie es Dateien gibt
		//iterator gibt an, welcher index in der jeweiligen Datei (an welcher stelle in Datei i gerade gelesen wird)
	}
	start = time(0);
	prozentanzeige(0, ausgabeAlgorithmString);

	for (unsigned int i = 0; i < dateiName.size(); i++)
	{
		readToVector(*data[i], values[i], maxTimestampsInVector, column1, errColumn);
	}
	compareAlgorithm(iterator, values, matchKriterium, output, rewriteToVectors);
	try
	{
		prozentCounter++;
		if (0.5*prozentCounter / maxTimestampsInVector > 0.5*oldCounter / maxTimestampsInVector)
		{

			prozent = (100.0*prozentCounter*(maxTimestampsInVector / (long double)(overallLines)));
			prozentanzeige(prozent, ausgabeAlgorithmString);
			oldCounter = prozentCounter;
		}
	}
	catch (int e)
	{
		cout << "berechnung von prozent falsch, prozentCounter " << prozentCounter << endl;
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
			iterator[i] = 0;		
			readToVector(*data[i], values[i], maxTimestampsInVector, column1, errColumn);
		}
		compareAlgorithm(iterator, values, matchKriterium, output, rewriteToVectors);
		try
		{
			prozentCounter++;
			if (0.5*prozentCounter / maxTimestampsInVector > 0.5*oldCounter / maxTimestampsInVector)
			{

				prozent = (100.0*prozentCounter*(maxTimestampsInVector / (long double)(overallLines)));
				prozentanzeige(prozent, ausgabeAlgorithmString);
				oldCounter = prozentCounter;
			}
		}
		catch (int e)
		{
			cout << "berechnung von prozent falsch, prozentCounter " << prozentCounter << endl;
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
	for (int i = 0; i < dateiName.size() * 2; i++)
	{
		for (auto i : rewriteToVectors) {
			iterator[i] = 0;
			readToVector(*data[i], values[i], maxTimestampsInVector, column1, errColumn);
		}
		compareAlgorithm(iterator, values, matchKriterium, output, rewriteToVectors);
		try
		{
			prozentCounter++;
			if (0.5*prozentCounter / maxTimestampsInVector > 0.5*oldCounter / maxTimestampsInVector)
			{

				prozent = (100.0*prozentCounter*(maxTimestampsInVector / (long double)(overallLines)));
				prozentanzeige(prozent, ausgabeAlgorithmString);
				oldCounter = prozentCounter;
			}
		}
		catch (int e)
		{
			cout << "berechnung von prozent falsch, prozentCounter " << prozentCounter << endl;
			exit(EXIT_FAILURE);
		}
	}//end of for  
	prozentanzeige(100, ausgabeAlgorithmString);
	cout << endl << endl;
	end = time(0);
	algorithmTime = end - start;
	if (verbose)
	{
		cout << "Das Programm ist fertig, Dauer: " << algorithmTime << "s" << endl;
	}
	return (int)true;
}//end of int main()																										
