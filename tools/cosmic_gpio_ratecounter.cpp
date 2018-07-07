#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <pigpio.h>
#include "i2cdevices.h"

using namespace std;

#define HW_VERSION 1
const static string VERSION_STR = "V1.0";

#define LM75_ADDR 0x4f
#define ADC_ADDR 0x48
#define DAC_ADDR 0x60
#define IO_ADDR 0x41


#define BIAS_ON 1  // set to 1 if SiPM bias voltage must be switched on to operate, otherwise 0

#define UBIAS_EN 4
#define PREAMP_1 20
#define PREAMP_2 21
#define EVT_AND 5
#define EVT_XOR 6
#define GAIN_HL 17

#define UBIAS_DAC_VOLTAGE 0.7    // corresponds to 32V Ubias
#define THR1_DEFAULT 0.01
#define THR2_DEFAULT 0.01

#define MEAS_INTERVAL_DEFAULT 10

enum scanpar_t {SP_NONE=0, SP_THR1, SP_THR2, SP_UBIAS};
const string scanpar_strings[] = {"NONE", "THR1", "THR2", "UBIAS"};

/*
extern "C" {
	// to make g++ know that custom_i2cdetect is written in c and not cpp (makes the linking possible)
	// You may want to check out on this here: (last visit 29. of May 2017)
	// https://stackoverflow.com/questions/1041866/in-c-source-what-is-the-effect-of-extern-c
#include "custom_i2cdetect.h"
}
*/

static long int scaler_and = 0;
static long int scaler_xor = 0;
static bool sample_trigger = false;
static bool sample_valid = false;
static bool decay_trigger = false;

static uint32_t curr_ts = 0;
static uint32_t last_ts = 0;


//void __attribute__ ((interrupt)) gpioInterrupt(int gpio,int level, uint32_t tick)
void gpioInterrupt(int gpio,int level, uint32_t tick)
{
   if(level==1)
   {
	  if (sample_trigger) sample_valid = false;
	  else sample_valid = true;
	  sample_trigger = true;

      if (gpio==EVT_AND) {
		  scaler_and++;
	  }
      else if (gpio==EVT_XOR) {
		  scaler_xor++;
		  last_ts = curr_ts;
		  curr_ts = tick;
		  if (curr_ts-last_ts < 100) decay_trigger=true;
	  }
   }
}

void Usage(const char* progname)
{
	cout<<"GPIO based rate counter for Cosmic Detector Unit"<<endl;
	cout<<"HW version: "<<HW_VERSION<<endl;
	cout<<"SW version: "<<VERSION_STR<<" (built "<<string(__DATE__)<<" "<<string(__TIME__)<<")"<<endl;
	cout<<"		"<<endl;
	cout<<"Options:"<<endl;
//	cout<<"0	1	2	3	4	5	6	7	8"<<endl;
	cout<<"	-h -?		: 	shows this help"<<endl;
	cout<<"	-p <PARAM>=<val>:	set specified parameter to value val"<<endl;
	cout<<"				PARAM can be: THR1, THR2 or UBIAS"<<endl;
	cout<<"	-P <PARAM>	: 	specify parameter for parameter scan"<<endl;
	cout<<"			  	PARAM can be: THR1, THR2 or UBIAS"<<endl;
	cout<<"				implies options -s and -i"<<endl;
	cout<<"	-s <min:max>	: 	specify scan range for parameter scan"<<endl;
	cout<<"				implies options -P and -i"<<endl;
	cout<<"	-i <increment>	: 	specify scan increment for parameter scan"<<endl;
	cout<<"				implies options -P and -s"<<endl;
	cout<<"	-b <On|Off>	: 	switch on/off preamp voltage"<<endl;
	cout<<"	-g		: 	additional gain switch for peak detector"<<endl;
	cout<<"	-a		: 	measure single hit analog amplitudes (Ch1) of ADC (in V) and output them on stdout"<<endl;
	cout<<"				if -v option is also specified: print out up to 10 consecutive ADC samples"<<endl;
	cout<<"	-d		: 	decay mode - print out succesive XOR hits which are delayed by less than 100 us"<<endl;
	cout<<"	-t <seconds>	: 	time for a single rate measurement in seconds"<<endl;
	cout<<"	-n <number>	: 	number of measurements to do in total"<<endl;
	cout<<"				or number of measurements for each scan parameter value"<<endl;
	cout<<"	-v		: 	increases verbosity level"<<endl;
	cout<<endl;
	cout<<"Examples:"<<endl;
	cout<<"1) set Ubias and THR1 to the specified values and make one measurement over 10s:"<<endl;
	cout<<"\t"<<string(progname)<<" -p UBIAS=0.6 -p THR1=0.02"<<endl<<endl;
	cout<<"2) set Ubias to specified value and make a scan for THR1 from 0.05 to 0.1"<<endl;
	cout<<"   in steps of 0.01 with one minute measurements each:"<<endl;
	cout<<"\t"<<string(progname)<<" -p UBIAS=0.6 -P THR1 -s 0.05:0.1 -i 0.01 -t 60"<<endl;
	cout<<""<<endl<<endl;
}//end of Usage()

inline bool caseInsCharCompareN(char a, char b) {
   return(toupper(a) == toupper(b));
}
bool caseInsCompare(const string& s1, const string& s2) {
   return((s1.size() == s2.size()) &&
          equal(s1.begin(), s1.end(), s2.begin(), caseInsCharCompareN));
}

int main(int argc, char** argv) {
	
	int meas_interval_sec = MEAS_INTERVAL_DEFAULT;
	double thr1 = THR1_DEFAULT;
	double thr2 = THR2_DEFAULT;
	signed long int N = 1;
	bool do_scan = false;
	double scanstart = 0.;
	double scanend = 0.;
	double scanpar_increment = 1.;
	scanpar_t scanpar = SP_NONE;
	struct parset_t {
		scanpar_t par;
		double val;	
	};
	vector<parset_t> parset_list;
	bool power_on = BIAS_ON;
	bool high_gain = false;
	char preamps_on = -1;
	bool show_amplitudes = false;
	bool decay_mode = false;
	
	int verbosity = 0;
	
	//cout<<"prog name is "<<string(argv[0])<<endl;
	
	int ch;
    while ((ch = getopt(argc, argv, "n:s:p:P:t:i:gb:avhd?")) != EOF)
    {
       string str, parname;
       size_t pos;
       double val;
       
       switch ((char)ch) {
		case 'n':
			N = atoi(optarg);
			break;
		case 's':
			char* pEnd;
			scanstart = strtod(optarg, &pEnd);
			scanend = strtod(pEnd+1, NULL);
			do_scan=true;
//			cout<<"start="<<d1<<endl;
//			cout<<"end="<<d2<<endl;
			break;
		case 'p':
			str = string(optarg);
			pos = str.find("=");
			parname = str;
			if (pos!=string::npos) {
				parname = str.substr(0,pos);
			} else {
				cerr<<"error: erronuos value assignment for parameter to set"<<endl;
				return -1;
			}
			val = strtod(str.substr(pos+1).c_str(), NULL);
			//cout<<"scanpar: "<<str<<endl;
			parset_t parset;
			for (int i=SP_NONE+1; i<=SP_UBIAS; i++) {
				if (parname==scanpar_strings[i]) {
					parset.par=(scanpar_t)i;
					parset.val=val;
					parset_list.push_back(parset);
					cout<<"set parameter: "<<scanpar_strings[i]<<"="<<val<<endl;
				}
				//cout<<"SP: "<<scanpar_strings[i]<<endl;
			} 
			break;
		case 'P':
			str = string(optarg);
			//cout<<"scanpar: "<<str<<endl;
			for (int i=SP_NONE+1; i<=SP_UBIAS; i++) {
				if (str==scanpar_strings[i]) {
					if (scanpar!=SP_NONE) {
						cerr<<"error: only single scan parameter allowed"<<endl;
						return -1;
					}
					cout<<"scan parameter is "<<scanpar_strings[i]<<endl;
					scanpar = (scanpar_t)i;
				}
				//cout<<"SP: "<<scanpar_strings[i]<<endl;
			
			} 
			break;
		case 't':
			meas_interval_sec = strtod(optarg, NULL);
			break;
		case 'i':
			scanpar_increment = strtod(optarg, NULL);
			break;
		case 'g':
			high_gain = true;
			break;
		case 'a':
			show_amplitudes = true;
			break;
		case 'd':
			decay_mode = true;
			break;
		case 'b':
			str = string(optarg);
			if (caseInsCompare(str, string("On"))) preamps_on = 1;
			else if (caseInsCompare(str, string("Off"))) preamps_on = 0;
			//cout<<"premps_on="<<(int)preamps_on<<endl;
			break;
		case 'v':
			verbosity++;
			break;
		case 'h':
		case '?':
			Usage(argv[0]);
			return 0;
		default:
			cerr<<"error: parameter not recognized:  -"<<char(ch)<<endl;
			Usage(argv[0]);
			return -1;
		}
    }
    argc -= optind;
    argv += optind;

    //printf("Anzahl an Argumenten: %d\n", argc);

    if (argc > 0)
    {
		perror("too many arguments!\n");
    	Usage(argv[0]);
    	return -1;
    }

	if (do_scan && scanstart>scanend) {
        cerr<<"error: scan boundaries not understandable"<<endl;
        return -1;
	}
	
	if (do_scan && (scanpar<=SP_NONE || scanpar>SP_UBIAS)) {
        cerr<<"error: scan parameter invalid"<<endl;
        return -1;
	}

	//return 0;


	if (gpioCfgClock(1,0,0)<0)
    {
        cerr<<"error configuring GPIO clock"<<endl;
        return -1;
    }
    if (gpioInitialise()<0)
    {
        cerr<<"error initializing GPIO interface"<<endl;
        return -1;
    }

	gpioSetMode(UBIAS_EN, PI_OUTPUT);
	gpioWrite(UBIAS_EN, power_on);
	gpioSetMode(PREAMP_1, PI_OUTPUT);
	gpioSetMode(PREAMP_2, PI_OUTPUT);
	if (preamps_on==0 ||  preamps_on==1) {
		gpioWrite(PREAMP_1, preamps_on);
		gpioWrite(PREAMP_2, preamps_on);
	}
	gpioSetMode(GAIN_HL, PI_OUTPUT);
	gpioWrite(GAIN_HL, high_gain);

	gpioSetMode(EVT_AND, PI_INPUT);
	gpioSetMode(EVT_XOR, PI_INPUT);
    
    gpioSetAlertFunc(EVT_XOR, gpioInterrupt);
    gpioSetAlertFunc(EVT_AND, gpioInterrupt);
	
	LM75 tempSensor("/dev/i2c-1", LM75_ADDR);
	ADS1115 adc("/dev/i2c-1", ADC_ADDR);
	adc.setPga(ADS1115::PGA4V);
	adc.setRate(ADS1115::RATE475);
	adc.setAGC(false);
	PCA9536 pca("/dev/i2c-1", IO_ADDR);
	pca.setOutputPorts(0x03);
	pca.setOutputState(0x00);
	MCP4728 dac("/dev/i2c-1", DAC_ADDR);
	

	// set parameters in the beginning, if required
	for (int i=0; i<parset_list.size(); i++) {
		switch (parset_list[i].par) {
			case SP_THR1:
				dac.setVoltage(0, parset_list[i].val);
				break;
			case SP_THR2:
				dac.setVoltage(1, parset_list[i].val);
				break;
			case SP_UBIAS:
				dac.setVoltage(2, parset_list[i].val);
				break;
			case SP_NONE:
			default:
				break;
		}
	}
	
	
	bool measurement = true;
	double scanpar_value = scanstart-scanpar_increment;
	while (measurement) {
		if (do_scan) {
			scanpar_value += scanpar_increment;
			if (scanpar_value>scanend) {
				measurement=false;
				break;
			}
			switch (scanpar) {
				case SP_THR1:
					dac.setVoltage(0, scanpar_value);
					break;
				case SP_THR2:
					dac.setVoltage(1, scanpar_value);
					break;
				case SP_UBIAS:
					dac.setVoltage(2, scanpar_value);
					break;
				case SP_NONE:
				default:
					break;
			}
		}
		
		cout<<"# time temp scanpar AND-rate XOR-rate err(AND) err(XOR) ADC2 UBIAS"<<endl;
		
		for (int n=0; n<N || N==-1; n++) {
			// reset scalers
			scaler_and = scaler_xor = 0;
			sample_trigger = sample_valid = false;
			struct timespec tStart, tEnd;
			long int deltaT=0;
			clock_gettime(CLOCK_REALTIME, &tStart);
//      	while (deltaT = tStart.tv_sec-tLast.tv_sec);
			while (deltaT<meas_interval_sec) {      
				struct timespec tCurr;
				clock_gettime(CLOCK_REALTIME, &tCurr);
				deltaT = -tStart.tv_sec+tCurr.tv_sec;
				if (sample_trigger && sample_valid) {
					if (show_amplitudes) {
//						cout<<"hit: "<<adc.readVoltage(0)<<" (readout took "<<adc.getReadWaitDelay()/1000.<<" ms, pga="<<(int)adc.getPga(0)<<")"<<endl;
						if (!verbosity)	{
							double val = adc.readVoltage(0);
							if (sample_valid)
								cout<<adc.readVoltage(0)<<endl;
						} else {
							int n=0;
							while (n<5 && sample_valid) {
								double val = adc.readVoltage(0);
								if (!sample_valid) break;
								cout<<val<<" ";
								n++;
							}
							if (n) cout<<endl;
						}
					}
					sample_trigger = false;
				} else if (sample_trigger && !sample_valid) {
					sample_trigger = false;
					sample_valid = false;
				}
				if (decay_mode && decay_trigger) {
					uint32_t diff=curr_ts-last_ts;
//					cout<<"pileup hit: "<<"ts1="<<last_ts<<"  ts2="<<curr_ts<<"  diff="<<diff<<endl;
					cout<<"delayed hit "<<"ts1= "<<last_ts<<"  ts2= "<<curr_ts<<"  diff= "<<diff<<endl;
					decay_trigger=false;
				}
				usleep(5000L);
			}
			//sleep(MEAS_INTERVAL_DEFAULT);
			long int and_latch = scaler_and;
			long int xor_latch = scaler_xor;
			double and_err = sqrt(and_latch);
			double xor_err = sqrt(xor_latch);
			double temperature = tempSensor.getTemperature();
			double ubias = 0.;
			for (int i=0; i<8; i++) ubias+=adc.readVoltage(2);
			ubias /= 8.;			
			
			cout<<tStart.tv_sec<<" "<<temperature<<" "
				<<scanpar_value<<" "
				<<and_latch/(double)meas_interval_sec<<" "
				<<xor_latch/(double)meas_interval_sec<<" "
				<<and_err/(double)meas_interval_sec<<" "
				<<xor_err/(double)meas_interval_sec<<" "
				<<ubias<<" "<<ubias*10.4<<endl;
		}
		if (!do_scan) measurement=false;
	}
	
/*
			double voltage = 0;
			double channel = 0;
			cout << "Set channel:" << endl;
			cin >> channel;
			if ((int)channel < 4 && (int)channel >= 0) {
				cout << "Set voltage:" << endl;
				double voltage = 0;
				cin >> voltage;
				if (voltage >= 0) {
					cout << "set voltage to " << voltage << endl;
					dac.setVoltage((uint8_t)channel, voltage);
				}
			}

*/	
	
	gpioTerminate();
	return 0;
	
}
