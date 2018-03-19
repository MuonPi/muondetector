/*
Simple u-blox UBX command interface
compile with:
g++ -O -c serial.cpp && g++ -O -c ublox.cpp && g++ -O -c uconfig.cpp && g++ -O uconfig.o ublox.o serial.o -o uconfig
g++ -O -std=gnu++11 -c serial.cpp && g++ -std=gnu++11 -O -c ublox.cpp && g++ -std=gnu++11 -O -c uconfig.cpp && g++ -std=gnu++11 -O -pthread uconfig.o ublox.o serial.o -o uconfig
*/

#include <stdio.h>
#include <inttypes.h>  // uint8_t, etc
#include <string>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
//#include <curses.h>
#include <signal.h>
#include <unistd.h>
#include <vector>
#include <algorithm>

#include "ublox.h"

using namespace std;


static void finish(int sig);


std::ostream& operator<<(std::ostream& os, const std::chrono::time_point<std::chrono::system_clock>& timestamp)
{
	//  std::chrono::time_point<std::chrono::system_clock> timestamp = std::chrono::system_clock::now();
	std::chrono::microseconds mus = std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch());
	std::chrono::seconds secs = std::chrono::duration_cast<std::chrono::seconds>(mus);
	std::chrono::microseconds subs = mus - secs;

	os << secs.count() << "." << setw(6) << setfill('0') << subs.count() << " " << setfill(' ');
	return os;
}

std::ostream& operator<<(std::ostream& os, const timespec& ts)
{
	//  std::chrono::time_point<std::chrono::system_clock> timestamp = std::chrono::system_clock::now();
	os << ts.tv_sec << "." << setw(9) << setfill('0') << ts.tv_nsec << " " << setfill(' ');
	return os;
}

void printTimestamp()
{
	std::chrono::time_point<std::chrono::system_clock> timestamp = std::chrono::system_clock::now();
	std::chrono::microseconds mus = std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch());
	std::chrono::seconds secs = std::chrono::duration_cast<std::chrono::seconds>(mus);
	std::chrono::microseconds subs = mus - secs;


	// 	t1 = std::chrono::system_clock::now();
	// 	t2 = std::chrono::duration_cast<std::chrono::seconds>(t1);



		//double subs=timestamp-(long int)timestamp;
	cout << secs.count() << "." << setw(6) << setfill('0') << subs.count() << " " << setfill(' ');
}

void Usage(const char* progname)
{
	cout << "U-Blox GPS polling and configuration program" << endl;
	cout << "2016-2017 HG Zaunick <zaunick@exp2.physik.uni-giessen.de>" << endl;
	cout << endl;
	cout << " Usage :  " << progname << " [-vdlLch?][-n <nr_cycles>] device" << endl;
	cout << "    available options:" << endl;
	cout << "     -b <baud rate> :   set serial baud rate, default 9600" << endl;
	cout << "     -v             :   increase verbosity" << endl;
	cout << "     -n <cycles>    :   time to listen to messages before exit in s, -1 for endless loop" << endl;
	cout << "     -d             :   dump raw gps device output to stdout" << endl;
	cout << "     -l             :   show list of currently available satellites" << endl;
	cout << "     -L             :   show list of all satellites in range" << endl;
	cout << "     -p <ccmm>      :   poll message with class ID cc and message ID mm" << endl;
	cout << "                        e.g. -p 0120  for message classID=0x01, messageID=0x20" << endl;
	cout << "     -c             :   show GNSS configs" << endl;
	//    cout<<"     -t <cmd>       :   display timing information"<<endl;
	//    cout<<"        command <cmd> selects type of info:"<<endl;
	//    cout<<"        cmd=1       : nav-clock"<<endl;
	//    cout<<"        cmd=2       : time-tp"<<endl;
	cout << "     -h / -?        :   show usage (this screen) and exit" << endl;
	cout << "     device is the serial port to use (default /dev/gps0)" << endl;
}



int main(int argc, char** argv)
{

	char* progname = argv[0];

	(void)signal(SIGINT, finish);      /* arrange interrupts to terminate */

  //   initscr(); 
  //   nl();
  //   clear();
  //   refresh();

	string gpsdevname = "/dev/gps0";
	int verbose = 0;
	long int N = 0;
	int baudrate = 9600;
	bool allSats = false;
	bool listSats = false;
	bool dumpRaw = false;
	bool poll = false;
	std::vector<UbxMessage> pollMsgVector;
	bool showGnssConfig = false;
	int timingCmd = 0;
	unsigned int cmID = 0x0000;
	UbxMessage msg;
	int ch;
	while ((ch = getopt(argc, argv, "dvb:p:lLcn:h?")) != EOF)
	{
		switch ((char)ch)
		{
		case 'v':
			//cout<<"verbose"<<endl;
			verbose++;
			break;
		case 'b':
			baudrate = atol(optarg);
			if (verbose > 2) cout << "baud rate:" << baudrate << endl;
			break;
		case 'd':
			//cout<<"debug"<<endl;
			dumpRaw = true;
			break;

		case 'p':
			cmID = stoi(optarg, nullptr, 16);
			msg.classID = (cmID & 0xff00) >> 8;
			msg.messageID = (cmID & 0x00ff);
			pollMsgVector.push_back(msg);
			if (verbose > 2) cout << "poll message 0x" << hex << setfill('0') << setw(4) << cmID << dec << endl;
			break;
		case 'l':
			listSats = true;
			if (verbose > 2) cout << "show available sats" << endl;
			break;
		case 'L':
			listSats = true;
			allSats = true;
			if (verbose > 2) cout << "show all sats" << endl;
			break;
		case 'n':
			N = atol(optarg);
			if (verbose > 2) cout << "nr of read cycles:" << N << endl;
			break;
			// 	 case 't':
			//             timingCmd=atoi(optarg);
			//             if (verbose>2) cout<<"timing command:"<<timingCmd<<endl;
			//             break;
		case 'c':
			showGnssConfig = true;
			break;
		case 'h':
		case '?':
		default:// break;
			Usage(progname);
			return -1;
		}
	}

	argc -= optind;
	argv += optind;
	if (argc > 1) {
		cerr << "error: too many arguments" << endl;
		Usage(progname);
	}

	while (argc > 0)
	{
		gpsdevname = *argv;
		if (verbose > 2) cout << gpsdevname << endl;
		--argc;
		++argv;
	}





	Ublox gps(gpsdevname, baudrate);
	gps.setVerbosity(verbose);

	if (!gps.Connect()) {
		cerr << "failed to open serial interface " << gpsdevname << endl;
		return -1;
	}
	//  gps.Print();


	if (showGnssConfig) {
		// 	gps.UBXCfgGNSS();
		// 	gps.UBXCfgNav5();
		// 	gps.UBXMonVer();

		const int measrate = 10;
		gps.UBXSetCfgRate(1000 / measrate, 1);

		gps.UBXSetCfgMsg(MSG_TIM_TM2, 1, 1);	// TIM-TM2
		gps.UBXSetCfgMsg(MSG_TIM_TP, 1, 51);	// TIM-TP
		gps.UBXSetCfgMsg(MSG_NAV_TIMEUTC, 1, 20);	// NAV-TIMEUTC
		gps.UBXSetCfgMsg(MSG_MON_HW, 1, 47);	// MON-HW
		gps.UBXSetCfgMsg(MSG_NAV_SAT, 1, 59);	// NAV-SAT
		gps.UBXSetCfgMsg(MSG_NAV_TIMEGPS, 1, 61);	// NAV-TIMEGPS
		gps.UBXSetCfgMsg(MSG_NAV_SOL, 1, 67);	// NAV-SOL
		gps.UBXSetCfgMsg(MSG_NAV_STATUS, 1, 71);	// NAV-STATUS
		gps.UBXSetCfgMsg(MSG_NAV_CLOCK, 1, 89);	// NAV-CLOCK
		gps.UBXSetCfgMsg(MSG_MON_TXBUF, 1, 97);	// MON-TXBUF
		gps.UBXSetCfgMsg(MSG_NAV_SBAS, 1, 255);	// NAV-SBAS
		gps.UBXSetCfgMsg(MSG_NAV_DOP, 1, 101);	// NAV-DOP
		gps.UBXSetCfgMsg(MSG_NAV_SVINFO, 1, 49);	// NAV-SVINFO


	}

	int i = 0;
	int cycles = 0;
	while (i < N || N == -1) {
		if (dumpRaw) {
			string s;
			int n = gps.ReadBuffer(s);
			if (n > 0) {
				cout << s;
				//cout<<n<<endl;
			}
			continue;
		}
		if (listSats) {
			if (gps.satList.updated) {
				vector<GnssSatellite> sats = gps.satList();
				if (!allSats) {
					std::sort(sats.begin(), sats.end(), GnssSatellite::sortByCnr);
					while (sats.back().getCnr() == 0 && sats.size() > 0) sats.pop_back();
				}
				cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps.nrSats.updateAge()) << "Nr of " << std::string((allSats) ? "visible" : "received") << " satellites: " << sats.size() << endl;
				// read nrSats property without evaluation to prevent separate display of this property
				// in the common message poll below
				int dummy = gps.nrSats();
				GnssSatellite::PrintHeader(true);
				for (int i = 0; i < sats.size(); i++) sats[i].Print(i, false);
			}
		}

		if (gps.nrSats.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps.nrSats.updateAge()) << "Nr of available satellites: " << gps.nrSats() << endl;
		if (gps.timeAccuracy.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps.timeAccuracy.updateAge()) << "time accuracy: " << gps.timeAccuracy() << " ns" << endl;
		if (gps.TPQuantErr.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps.TPQuantErr.updateAge()) << "quant error: " << gps.TPQuantErr() << " ps" << endl;
		if (gps.noise.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps.noise.updateAge()) << "noise: " << gps.noise() << endl;
		if (gps.agc.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps.agc.updateAge()) << "agc: " << gps.agc() << endl;
		if (gps.txBufUsage.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps.txBufUsage.updateAge()) << "TX buf usage: " << gps.txBufUsage() << " %" << endl;
		if (gps.txBufPeakUsage.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps.txBufPeakUsage.updateAge()) << "TX buf peak usage: " << gps.txBufPeakUsage() << " %" << endl;
		if (gps.eventCounter.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps.eventCounter.updateAge()) << "rising edge counter: " << gps.eventCounter() << endl;
		if (gps.clkBias.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps.clkBias.updateAge()) << "clock bias: " << gps.clkBias() << " ns" << endl;
		if (gps.clkDrift.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps.clkDrift.updateAge()) << "clock drift: " << gps.clkDrift() << " ns/s" << endl;
		if (gps.getEventFIFOSize()) {
			struct gpsTimestamp ts = gps.getEventFIFOEntry();
			//cout<<ts.rising_time<<" timestamp event";
			if (ts.rising) cout << ts.rising_time << " timestamp event rising " << " accuracy: " << ts.accuracy_ns << " ns  counter: " << ts.counter << endl;
			if (ts.falling) cout << ts.falling_time << " timestamp event falling " << " accuracy: " << ts.accuracy_ns << " ns  counter: " << ts.counter << endl;
			//	  cout<<" accuracy: "<<ts.accuracy_ns<<" ns  counter: "<<ts.counter<<endl;
		}

		//       switch (timingCmd) {
		// 	case 1:	
		// 	  if (verbose>2) cout<<"timing cmd 1"<<endl;
		// 	  int itow,bias,drift,tAcc,fAcc;
		// 	  if (!gps.UBXNavClock(itow, bias, drift, tAcc, fAcc))
		// 	  {
		// 	    cerr<<"error receiving nav-clock"<<endl;
		// 	    break;
		// 	  }
		// 	  if (i<=1) {
		// 	    cout<<"Nav-Clock timing information:"<<endl;
		// 	    cout<<"--------------------------------------------------------"<<endl;  
		// 	    cout<<" iToW     clk bias     clk drift     time acc   freq acc"<<endl;  
		// 	    cout<<" (s)        (ns)        (ns/s)         (ns)      (ps/s)"<<endl;  
		// 	    cout<<"--------------------------------------------------------"<<endl;  
		// 	  }
		// 	  cout<<setw(6)<<dec<<itow<<"    "<<setw(7)<<bias<<"       "<<setw(4)<<drift<<"          "<<setw(4)<<tAcc<<"        "<<fAcc<<endl;
		// 	  break;
		// 	case 2:
		// 	  if (verbose>2) cout<<"timing cmd 2"<<endl;
		// 	  int quantErr,weekNr;
		// 	  if (!gps.UBXTimTP(itow, quantErr, weekNr))
		// 	  {
		// 	    cerr<<"error receiving tim-tp"<<endl;
		// 	    break;
		// 	  }
		// 	  if (i<=1) {
		// 	    cout<<"Tim-TP timing information:"<<endl;
		// 	    cout<<"-------------------------------------"<<endl;  
		// 	    cout<<" iToW     quantization     week nr   "<<endl;  
		// 	    cout<<" (s)       error (ps)                "<<endl;  
		// 	    cout<<"-------------------------------------"<<endl;  
		// 	  }
		// 	  cout<<setw(6)<<itow<<"     "<<setw(7)<<quantErr<<"          "<<setw(4)<<weekNr<<endl;
		// 	  break;
		// 	default: break;
		//       }
		cout << flush;
		usleep(10000);
		if (++cycles >= 100) {
			cycles = 0;
			i++;
		}
	}


	return 0;

}


static void finish(int sig)
{
	//     endwin();

		/* do your non-curses wrapup here */
	cout << "aborting..bye" << endl;
	exit(0);
}

