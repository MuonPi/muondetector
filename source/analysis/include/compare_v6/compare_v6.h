#ifndef COMPARE_V6_H
#define COMPARE_V6_H

#include <string>
#include <vector>
#include <istream>

#define MAX_ERR_LIMIT 100000.


struct timestamp_t {
	timespec ts;
	double err;
	double length;
};


static unsigned long int oldBalken = 0;

void prozentanzeige(unsigned long int prozent, std::string& ausgabe);

long long int ts_diff_ns(timespec& ts1, timespec& ts2);

void skipColumn(std::istream& data, int col);

bool areTimestampsInOrder(std::string dateiName, bool verbose, unsigned long int& lineCounter, int tsCol);

void readToVector(std::ifstream& inputstream, std::vector<timestamp_t>& oneVector, int& maxValues, int tsCol, int errCol);

void Usage();

void compareAlgorithm(std::vector<unsigned int> it, std::vector<std::vector<timestamp_t> >& values, double& matchKriterium, std::ofstream& output, std::vector<unsigned int>& rewriteToVectors);

int compare(std::string outputDateiName, std::vector<std::string> dateiName, double matchKriterium = 0.001/*micro seconds*/, int maxTimestampsAtOnce = 10000, int column1 = 0, int errColumn = -1, unsigned long int overallLines = 0, bool verbose = false);
#endif // COMPARE_V6_H
