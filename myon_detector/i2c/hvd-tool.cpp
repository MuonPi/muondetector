#include "hvd-tool.h"
using namespace std;

int powerOfTen(int orderOfMagnitude) {
	if (orderOfMagnitude == 0) {
		return 1;
	}
	int value = 10;
	for (int i = 1; i < orderOfMagnitude; i++) {
		value = value * 10;
	}
	return value;
}

double Stats::getMean() {
	double mean = 0.;
	if (!nrEntries()) return mean;
	for (deque<double>::iterator it = buf.begin(); it != buf.end(); it++) {
		mean += *it;
	}
	mean /= (double)nrEntries();
	return mean;
}

inline double Stats::getRMS() {
	double sum = accumulate(begin(buf), end(buf), 0.0);
	double m = sum / buf.size();
	double accum = 0.0;
	for_each(begin(buf), end(buf), [&](const double d) {
		accum += (d - m) * (d - m);
	});

	double stdev = sqrt(accum / (buf.size() - 1));
	return stdev;
}

int setnew_wiper(double voltalt, double voltin, double hvmeas, int dac0, int dac1) {
	double compare0, compare1;
	int ddac;
	ddac = dac0 - dac1;
	if (ddac == 0) { ddac = 1; }
	compare0 = voltalt - hvmeas;
	compare1 = hvmeas - voltin;
	if (fabs(compare1) < 1) {
		ddac = 0;
		if (fabs(hvmeas - voltin) > 0.06) {
			if (fabs(hvmeas - voltin) > 0.15) {
				if (hvmeas < voltin) { dac1 = dac1 - 2; }
				else { dac1 = dac1 + 2; }
			}
			else {
				if (hvmeas < voltin) { dac1--; }
				else { dac1++; }
			}
		}
	}
	else {
		ddac = (ddac / compare0)*compare1;
		dac1 = dac1 - ddac;
	}
	if (dac1 > 1023) { dac1 = 1023; };
	if (dac1 < 0) { dac1 = 0; };
	return dac1;
}

void print(int lineNumber, bool channels[NUMBER_OF_CHANNELS], bool timestamp, double temperature, int dac[NUMBER_OF_CHANNELS],
	double hvmeas[NUMBER_OF_CHANNELS], double imeas[NUMBER_OF_CHANNELS])
{
	// function for printing data in a specified way
	printf("%d  %6.2f", lineNumber, temperature);
	for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
		if (channels[i]) { printf(" %d %7.3f %6.3f", dac[i], hvmeas[i], imeas[i]); }
	}
	if (timestamp) {

		time_t now;
		time(&now);
		printf(" %d\n", now);
	}
	else {
		printf("\n");
	}
	fflush(stdout);
}

void print(int lineNumber, bool channels[NUMBER_OF_CHANNELS], bool timestamp, double temperature, int dac[NUMBER_OF_CHANNELS],
	double hvmeas[NUMBER_OF_CHANNELS], double vStatsRMS[NUMBER_OF_CHANNELS], double imeas[NUMBER_OF_CHANNELS],
	double iStatsRMS[NUMBER_OF_CHANNELS])
{
	// function for printing data in a specified way
	printf("%d  %6.2f", lineNumber, temperature);
	for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
		if (channels[i]) { printf(" %d %7.3f %4.3f %6.3f %4.3f", dac[i], hvmeas[i], vStatsRMS[i], imeas[i], iStatsRMS[i]); }
	}
	if (timestamp) {

		time_t now;
		time(&now);
		printf(" %d\n", now);
	}
	else {
		printf("\n");
	}
	fflush(stdout);
}

void print(int lineNumber, bool channels[NUMBER_OF_CHANNELS], bool timestamp, double temperature, int dac,
	double hvmeas[NUMBER_OF_CHANNELS], double vStatsRMS[NUMBER_OF_CHANNELS], double imeas[NUMBER_OF_CHANNELS],
	double iStatsRMS[NUMBER_OF_CHANNELS])
{
	// function for printing data in a specified way
	printf("%d  %6.2f  %d", lineNumber, temperature, dac);
	for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
		if (channels[i]) { printf(" %7.3f %4.3f %6.3f %4.3f", hvmeas[i], vStatsRMS[i], imeas[i], iStatsRMS[i]); }
	}
	if (timestamp) {

		time_t now;
		time(&now);
		printf(" %d\n", now);
	}
	else {
		printf("\n");
	}
	fflush(stdout);
}

void usage()
{ // this gets printed if you make a mistake with your input or if you start the program with -h or -? option
	cout << endl;
	cout << "hvd-tool" << endl;
	cout << "tool for hv-distribution-board for PANDA electromagnetic calorimeter APDs" << endl;
	cout << "for more information check https://panda.gsi.de/" << endl;
	cout << "and/or contact \"AG Brinkmann, 2. Physikalisches Institut\", Justus-Liebig-Universität Gießen" << endl;
	cout << "options:" << endl;
	cout << "    -h / -?     : shows this page for help and usage" << endl;
	cout << "    -t          : timestamp" << endl;
	cout << "    -c <number> : select channel (\"-c 123\" activates channels 1, 2 and 3 for example" << endl;
	cout << "    -s          : activates \"set\"-mode that tries to regulate to set voltage, default is 380V" << endl;
	cout << "    -u <number> : set voltage (only applies if mode \"-s\" for set and then \"-u <voltage>\" is used" << endl;
	cout << "    -r	         : set readonly, reads all or just one channel depending on additional -c input without changing settings" << endl;
	cout << "    -m <word>	 : set a mode. This can be some programmed testrun or something else" << endl;
	cout << "                  modes that are currently implemented:" << endl;
	cout << "                     \"fullscan\" [int step size] [int repeat]   runs full scan through wiper values ([] means optional)" << endl;
	cout << "    -i <number> : set number of iterations for stats, each measurement consists of <i> single measurements standard is 40" << endl;
	cout << endl; cout << "---last edited: 13. September 2017---" << endl << endl;
}//end of Usage()


int main(int argc, char*argv[]) {
	double voltage = 380;
	int channel = 0; // 0 means all channels, 1 to 4 are numbers of channels
	int ch_temp; // just random temp variable to loop through getopt
	int iterationsForStats = 40;
	string scanmode = "";
	bool timestamp = false;
	bool readonly = false;
	bool setAndShow = false;
	bool channels[NUMBER_OF_CHANNELS];
	for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
		channels[i] = false;
	}
	while ((ch_temp = getopt(argc, argv, "u:sc:tm:ri:h?")) != EOF)
	{
		switch ((char)ch_temp)
		{
		case 'u': voltage = atof(optarg);
			cout << "voltage = " << voltage << endl;
			break;
		case 's': setAndShow = true;
			break;
		case 'c':
			channel = atoi(optarg);
			if (channel < 0 || channel>87654321) {
				channel = 0;
				cout << "not a valid channel, please enter valid channel" << endl;
				exit(1);
			}
			break;
		case 't':
			timestamp = true;
			break;
		case 'm':
			scanmode = optarg;
			break;
		case 'r':
			readonly = true;
			break;
		case 'i':
			iterationsForStats = atoi(optarg);
			break;
		case 'h':
			usage();
			return -1;
			break;
		case '?':
			usage();
			return -1;
			break;
		}
	}
	if (channel != 0) {
		for (int i = 7; i >= 0; i--) {
			//cout << "channel = " << channel << "  ";
			int temp = channel / powerOfTen(i);
			//cout << "temp = " << temp << endl;
			if (temp >= 10) {
				cout << "error, check your code" << endl;
			}
			else {
				if (temp > 0) {
					channels[temp - 1] = true;
					channel = channel % powerOfTen(i);
				}
			}
		}
	}
	else {
		for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
			channels[i] = true;
		}
	}



	if (i2cdetect() != 0)
	{
		// if i2cdetect() returns not 0 there are missing devices or devices not in place
		// or i2c does not function properly -> exit with error
		exit(1);
	}




	// until here: only getting all information from input (argc/argv/optarg etc. things)
	// and checking if all devices are connected via i2c.

	// following: some variable declaration and instantiation of adc, poti, lm75, eeprom etc.
	// things that have to be done for every mode, (readonly, set and regulate, fullscan)

	const char* i2cBus = "/dev/i2c-1";
	int16_t val;			// Stores the 16 bit value of our ADC conversion

	ADS1115 adc1(i2cBus, ADC1_ADDR);	// Open the I2C device
	ADS1115 adc2(i2cBus, ADC2_ADDR);	// Open the I2C device

	LM75 lm75(i2cBus, LM75_ADDR);

	//X9119 potis[4];

	X9119 poti1(i2cBus, POTI1_ADDR);
	X9119 poti2(i2cBus, POTI2_ADDR);
	X9119 poti3(i2cBus, POTI3_ADDR);
	X9119 poti4(i2cBus, POTI4_ADDR);
	X9119 potis[NUMBER_OF_CHANNELS];
	potis[0] = poti1;
	potis[1] = poti2;
	potis[2] = poti3;
	potis[3] = poti4;

	EEPROM24AA02 eeprom(i2cBus, EEP_ADDR);

	adc1.setPga(ADS1115::PGA6V);
	adc2.setPga(ADS1115::PGA6V);
	adc1.setAGC(true);
	adc2.setAGC(true);
	adc1.setRate(ADS1115::RATE64);
	adc2.setRate(ADS1115::RATE64);


	double v1[4], v2[4];
	int16_t adcval1[4], adcval2[4];
	double hvmeas[4];
	double imeas[4];

	for (int i = 0; i < 4; i++) {
		adc1.readVoltage(i, adcval1[i], v1[i]);
		adc2.readVoltage(i, adcval2[i], v2[i]);
	}

	if (channels[0]) { hvmeas[0] = v1[1] * VMEAS_FACTOR; }
	if (channels[1]) { hvmeas[1] = v1[3] * VMEAS_FACTOR; }
	if (channels[2]) { hvmeas[2] = v2[1] * VMEAS_FACTOR; }
	if (channels[3]) { hvmeas[3] = v2[3] * VMEAS_FACTOR; }

	if (channels[0]) { imeas[0] = v1[0] * IMEAS_FACTOR + IMEAS_OFFSET; }  //change for board1. 
	if (channels[1]) { imeas[1] = v1[2] * IMEAS_FACTOR + IMEAS_OFFSET; } //board2: 1.130 : 0.136 : -0.885 : 2.761 ??
	if (channels[2]) { imeas[2] = v2[0] * IMEAS_FACTOR + IMEAS_OFFSET; }
	if (channels[3]) { imeas[3] = v2[2] * IMEAS_FACTOR + IMEAS_OFFSET; }

	printf("ADC1 ");
	for (int i = 0; i < 4; i++) {
		if (channels[i]) { printf("  ch%d: %d", i + 1, adc1.readADC(i)); }
	}
	printf("\n");

	printf("ADC2 ");
	for (int i = 0; i < 4; i++) {
		if (channels[i]) { printf("  ch%d: %d", i + 1, adc2.readADC(i)); }
	}
	printf("\n");

	printf("*******************************\n");
	for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
		if (channels[i]) {
			printf("HV%d :     %7.2f V \n", i + 1, hvmeas[i]);
			printf("Current%d: %6.1f nA \n", i + 1, imeas[i]);
		}
	}

	printf("*******************************\n");
	for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
		if (channels[i]) { printf("R%dMeas: %7.2f MOhm \n", i + 1, 1000.*fabs(hvmeas[i] / imeas[i])); }//use amount
	}
	printf("LM75 temperature: %6.2f C\n", lm75.getTemperature());
	//  printf(" (last read took %6.2f ms)\n", lm75.getLastTimeInterval());

	for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
		if (channels[i]) {
			printf("X9119 Poti Channel %d WCR 0: %d\n", i + 1, potis[i].readWiperReg());
		}
	}
	struct timeval t0, tmeas;
	gettimeofday(&t0, NULL);
	// until here: only declaration of variables and filling the header of the data output
	//<----------------------------------------------------------------------------------------------------


	//<----------------------------------------------------------------------------------------------------
	// if mode: "readonly" then it should only read and give out the data without writing anything to the board
	// maybe make N to be selectable
	// maybe simplify this readonly code which is very similar to already existent code, make both just one bit of code
	// or put it in a function!
	if (readonly) {

		int pgaSetting = ADS1115::PGA256MV;

		adc1.setPga(ADS1115::PGA6V);
		adc2.setPga(ADS1115::PGA6V);

		printf("nr  T(C)");
		for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
			if (channels[i]) { printf("  dac%d HV%d(V) dHV%d(V) I%d(nA) dI%d(nA)", i + 1, i + 1, i + 1, i + 1, i + 1); }
		}
		printf("\n");

		const int N = iterationsForStats;
		while (true) {
			Stats vStats[NUMBER_OF_CHANNELS];
			Stats iStats[NUMBER_OF_CHANNELS];
			for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
				vStats[i].maxBufSize = N;
				iStats[i].maxBufSize = N;
			}
			int dac[] = { 0,0,0,0 };

			int line = 0;
			for (int j = 0; j < N; j++) {
				for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
					if (channels[i]) {
						adc1.readVoltage(i, adcval1[i], v1[i]);
						adc2.readVoltage(i, adcval2[i], v2[i]);
					}
				}

				for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
					if (channels[i]) { dac[i] = potis[i].readWiperReg(); }
					//readWiperReg is bugged, don't know why it does not work
				}


				if (channels[0]) { hvmeas[0] = v1[1] * VMEAS_FACTOR; }
				if (channels[1]) { hvmeas[1] = v1[3] * VMEAS_FACTOR; }
				if (channels[2]) { hvmeas[2] = v2[1] * VMEAS_FACTOR; }
				if (channels[3]) { hvmeas[3] = v2[3] * VMEAS_FACTOR; }

				if (channels[0]) { imeas[0] = v1[0] * IMEAS_FACTOR + IMEAS_OFFSET; }
				if (channels[1]) { imeas[1] = v1[2] * IMEAS_FACTOR + IMEAS_OFFSET; }
				if (channels[2]) { imeas[2] = v2[0] * IMEAS_FACTOR + IMEAS_OFFSET; }
				if (channels[3]) { imeas[3] = v2[2] * IMEAS_FACTOR + IMEAS_OFFSET; }

				for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
					if (channels[i]) { vStats[i].add(hvmeas[i]); }
					if (channels[i]) { iStats[i].add(imeas[i]); }
				}
			}

			for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
				if (channels[i]) { hvmeas[i] = vStats[i].getMean(); }
				if (channels[i]) { imeas[i] = iStats[i].getMean(); }
			}
			double vStatsRMS[NUMBER_OF_CHANNELS];
			double iStatsRMS[NUMBER_OF_CHANNELS];
			for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
				if (channels[i]) { vStatsRMS[i] = vStats[i].getRMS(); }
				if (channels[i]) { iStatsRMS[i] = iStats[i].getRMS(); }
			}
			print(line, channels, timestamp, lm75.getTemperature(), dac, hvmeas, vStatsRMS, imeas, iStatsRMS);
			line++;
		}
	}// end of "if(readonly)" */


	//<----------------------------------------------------------------------------------------------------
	// this is for setting a voltage and read 
	if (setAndShow && !readonly) {
		//use the regulator function to keep voltage stable
		for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
			if (channels[i]) { potis[i].writeWiperReg(1023); }
		}
		usleep(65535);

		int pgaSetting = ADS1115::PGA256MV;

		adc1.setPga(ADS1115::PGA6V);
		adc2.setPga(ADS1115::PGA6V);

		printf("nr  T(C)");
		for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
			if (channels[i]) { printf("  dac%d HV%d[V] I%d[nA] ", i + 1, i + 1, i + 1); }
		}
		printf("\n");

		int dac0[] = { 1023 , 1023 , 1023 , 1023 }; //600 old value (first value is 1023-23=1000)
		int dac1[] = { 1000 , 1000 , 1000 , 1000 };
		int line = 0;
		double voltin = voltage;

		double voltalt[] = { 0.0 , 0.0 , 0.0 , 0.0 }; //array for old voltage values	
		//double voltaelter[] = { 0.0, 0.0, 0.0, 0.0 };
		for (int i = 0; i < 4; i++) {
			voltalt[i] = hvmeas[i];			//fill with the first measurements
		}


		while (true) {				//endlosschleife, maybe put an easy way to stop it like pressing q in

			for (int k = 0; k < 2; k++) {
				//doing the adjustment 2 times because if changes are small the voltage measurement shows a weird behaviour
				for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
					if (channels[i]) { potis[i].writeWiperReg(dac1[i]); } //set poti to new dac value 
				}
			}

			const int N = iterationsForStats;
			double tsum = 0.;
			Stats vStats[NUMBER_OF_CHANNELS];
			Stats iStats[NUMBER_OF_CHANNELS];
			for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
				vStats[i].maxBufSize = N;
				iStats[i].maxBufSize = N;
			}

			for (int j = 0; j < N; j++) {

				for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
					if (channels[i]) {
						adc1.readVoltage(i, adcval1[i], v1[i]);
						adc2.readVoltage(i, adcval2[i], v2[i]);
					}
				}

				if (channels[0]) { hvmeas[0] = v1[1] * VMEAS_FACTOR; }		//get new voltage measurement
				if (channels[1]) { hvmeas[1] = v1[3] * VMEAS_FACTOR; }
				if (channels[2]) { hvmeas[2] = v2[1] * VMEAS_FACTOR; }
				if (channels[3]) { hvmeas[3] = v2[3] * VMEAS_FACTOR; }

				if (channels[0]) { imeas[0] = v1[0] * IMEAS_FACTOR + IMEAS_OFFSET; }
				if (channels[1]) { imeas[1] = v1[2] * IMEAS_FACTOR + IMEAS_OFFSET; }
				if (channels[2]) { imeas[2] = v2[0] * IMEAS_FACTOR + IMEAS_OFFSET; }
				if (channels[3]) { imeas[3] = v2[2] * IMEAS_FACTOR + IMEAS_OFFSET; }

				//hvsum+=hvmeas;
				for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
					if (channels[i]) { vStats[i].add(hvmeas[i]); } //v1 v2 v3 v4 like a variable not like it meant to be (see imeas hvmeas)
					if (channels[i]) { iStats[i].add(imeas[i]); }
				}
			}

			//    hvmeas=hvsum/N+VMEAS_OFFSET;
			for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
				if (channels[i]) { hvmeas[i] = vStats[i].getMean(); }
				if (channels[i]) { imeas[i] = iStats[i].getMean(); }
			}
			//print out the data
			print(line, channels, timestamp, lm75.getTemperature(), dac1, hvmeas, imeas);
			line++;

			for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
				if (channels[i]) {
					int buffer = dac1[i];
					dac1[i] = setnew_wiper(voltalt[i], voltin, hvmeas[i], dac0[i], dac1[i]);
					dac0[i] = buffer;
					//if (1022-dac[i] < 0 ){cout<<"ERROR CHANNEL "<<i<<" set wiper to save value"<<endl; dac[i]=990; }
				}
			}
			for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
				if (channels[i]) {
					//voltaelter[i] = voltalt[i];
					voltalt[i] = hvmeas[i];
				}
			}
		}
	}// end of setAndShow


	//<----------------------------------------------------------------------------------------------------
	// fullscan means the old scan through all wiper positions
	if (scanmode == "fullscan" && !readonly) {
		//loop to scan through all wiper positions -> HV scan
		for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
			if (channels[i]) { potis[i].writeWiperReg(1023); }
		}
		usleep(65535);

		int pgaSetting = ADS1115::PGA256MV;

		adc1.setPga(ADS1115::PGA6V);
		adc2.setPga(ADS1115::PGA6V);

		int dac = 1023; //1023
		int count = 0;
		int line = 0;
		int step = 4;
		int repeat = 2;
		int factor = -1;
		if (argc - optind == 0) {
			//no given arguments, set the repetitions and the steprange to standartvalues 
			//(already done with declaration)
		}
		else if (argc - optind == 1) {
			//one given argument set the repetitions to a standart value
			step = -fabs(strtol(argv[optind], NULL, 10));
		}
		//steprange and number of repetitions given by the first arguments 
		else if (argc - optind == 2) {
			step = -fabs(strtol(argv[optind], NULL, 10));
			repeat = fabs(strtol(argv[optind + 1], NULL, 10)) + 1;
		}
		else {
			cout << "wrong number of arguments, stop" << endl;
			usage();
			exit(1);
		}

		printf("step = %d\nrepeat = %d\n", step, repeat);

		printf("nr  T(C)");
		for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
			if (channels[i]) { printf("  dac%d HV%d[V] dHV%d[V] I%d[nA] dI%d[nA]", i + 1, i + 1, i + 1, i + 1, i + 1); }
		}
		if (timestamp) {
			printf("  time[s]\n");
		}


		while (count < repeat) {

			for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
				if (channels[i]) { potis[i].writeWiperReg(dac); }
			}
			const int N = iterationsForStats;
			double tsum = 0.;
			Stats vStats[NUMBER_OF_CHANNELS];
			Stats iStats[NUMBER_OF_CHANNELS];
			for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
				vStats[i].maxBufSize = N;
				iStats[i].maxBufSize = N;
			}
			for (int j = 0; j < N; j++) {
				for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
					adc1.readVoltage(i, adcval1[i], v1[i]);
					adc2.readVoltage(i, adcval2[i], v2[i]);
				}

				hvmeas[0] = v1[1] * VMEAS_FACTOR;
				hvmeas[1] = v1[3] * VMEAS_FACTOR;
				hvmeas[2] = v2[1] * VMEAS_FACTOR;
				hvmeas[3] = v2[3] * VMEAS_FACTOR;

				imeas[0] = v1[0] * IMEAS_FACTOR + IMEAS_OFFSET;
				imeas[1] = v1[2] * IMEAS_FACTOR + IMEAS_OFFSET;
				imeas[2] = v2[0] * IMEAS_FACTOR + IMEAS_OFFSET;
				imeas[3] = v2[2] * IMEAS_FACTOR + IMEAS_OFFSET;

				//hvsum+=hvmeas;
				for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
					vStats[i].add(hvmeas[i]); //v1 v2 v3 v4 like a variable not like it meant to be (see imeas hvmeas)
					iStats[i].add(imeas[i]);
				}
			}

			//    hvmeas=hvsum/N+VMEAS_OFFSET;
			for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
				hvmeas[i] = vStats[i].getMean();
				imeas[i] = iStats[i].getMean();
			}
			double vStatsRMS[NUMBER_OF_CHANNELS];
			double iStatsRMS[NUMBER_OF_CHANNELS];
			for (int i = 0; i < NUMBER_OF_CHANNELS; i++) {
				if (channels[i]) { vStatsRMS[i] = vStats[i].getRMS(); }
				if (channels[i]) { iStatsRMS[i] = iStats[i].getRMS(); }
			}
			print(line, channels, timestamp, lm75.getTemperature(), dac, hvmeas, vStatsRMS, imeas, iStatsRMS);
			line++;
			//the following lines are needed for the long term messearument
			//the if question asks if one of the turning points is reached. If so the loop counter count increaases
			dac += step;
			if (dac <= 0) {
				dac = 0;
				step = -step;
				count++;
			}
			if (dac > 1023) {
				dac = 1023;
				step = -step;
				count++;
			}
			//till here !!!!
		}
		// poti to safe position
		poti1.writeWiperReg(1023);
		poti2.writeWiperReg(1023);
		poti3.writeWiperReg(1023);
		poti4.writeWiperReg(1023);
		poti1.writeWiperReg(1023);
		poti2.writeWiperReg(1023);
		poti3.writeWiperReg(1023);
		poti4.writeWiperReg(1023);
	}//end of fullscan
	return 0;
}
