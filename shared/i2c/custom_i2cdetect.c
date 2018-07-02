/*
custom_i2cdetect is directly copied from i2c-tools-3.1.2 i2cdetect.c and edited
it now contains "i2cdetect()" as a function which does the same as main() did before except a few changes.
Now it can be called by the main program and it's the only purpose of custom_i2cdetect right now.
*/

#include "custom_i2cdetect.h"


static int scan_i2c_bus(int file, int mode, int first, int last)
{
	// called by i2cdetect() to scan through all busses
	int i, j;
	int res;

	printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");

	int deviceStatus[8];
	bool deviceError = false;
	// deviceError set to true means the program should exit with error
	// because at least one of the devices that should be there is not.
	// Also should it be printed which devices are not there.
	for (int k = 0; k<8; k++)
	{
		deviceStatus[k] = 0;
	}
	for (i = 0; i < 128; i += 16) {
		printf("%02x: ", i);
		for(j = 0; j < 16; j++) {
			fflush(stdout);

			/* Skip unwanted addresses */
			if (i+j < first || i+j > last) {
				printf("   ");
				continue;
			}

			/* Set slave address */
			if (ioctl(file, I2C_SLAVE, i+j) < 0) {
				if (errno == EBUSY) {
					printf("UU ");
					continue;
				} else {
					fprintf(stderr, "Error: Could not set "
						"address to 0x%02x: %s\n", i+j,
						strerror(errno));
					return -1;
				}
			}

			/* Probe this address */
			/*switch (mode) {
			default:*/
			if ((i+j >= 0x30 && i+j <= 0x37)
				|| (i+j >= 0x50 && i+j <= 0x5F)){
				res = i2c_smbus_read_byte(file);
			}else{
				res = i2c_smbus_write_quick(file,
						I2C_SMBUS_WRITE);
			}
			//}

			if (res<0) {
				printf("-- ");
				/* there is no device at this address (address is j+i)
				   so switch j+i and if the address matches one of the expected
				   addresses used by one of the components on the board
				   the program should exit with an error
				*/
				switch (j+i)
				{
				case ADC1_ADDR:
					// if it should exit with error set deviceError true
					// and set corresponding deviceStatus to the missing device
					// so that it can be printed that this device is missing
					deviceStatus[0] = ADC1_ADDR;
					deviceError = true;
					break;
				case ADC2_ADDR:
					deviceStatus[1] = ADC2_ADDR;
					deviceError = true;
					break;
				case LM75_ADDR:
					deviceStatus[2] = LM75_ADDR;
					deviceError = true;
					break;
				case POTI1_ADDR:
					deviceStatus[3] = POTI1_ADDR;
					deviceError = true;
					break;
				case POTI2_ADDR:
					deviceStatus[4] = POTI2_ADDR;
					deviceError = true;
					break;
				case POTI3_ADDR:
					deviceStatus[5] = POTI3_ADDR;
					deviceError = true;
					break;
				case POTI4_ADDR:
					deviceStatus[6] = POTI4_ADDR;
					deviceError = true;
					break;
				case EEP_ADDR:
					deviceStatus[7] = EEP_ADDR;
					deviceError = true;
					break;
				default:
					break;
				}
			}
			else 
			{
				printf("%02x ", i+j);
			}
		}
		printf("\n");
	}
	if (deviceError)
	{
		// print missing devices if there are any
		// and exit with error which is meant by "return 1"
		printf("\n Following addresses are not connected: \n");
		for (int k = 0; k<8; k++)
		{
			if (deviceStatus[k]!=0)
			{
				switch (deviceStatus[k])
				{
				case ADC1_ADDR:
					printf("ADC1_ADDR");
					break;
				case ADC2_ADDR:
					printf("ADC2_ADDR");
					break;
				case LM75_ADDR:
					printf("LM75_ADDR");
					break;
				case POTI1_ADDR:
					printf("POTI1_ADDR");
					break;
				case POTI2_ADDR:
					printf("POTI2_ADDR");
					break;
				case POTI3_ADDR:
					printf("POTI3_ADDR");
					break;
				case POTI4_ADDR:
					printf("POTI4_ADDR");
					break;
				case EEP_ADDR:
					printf("EEP_ADDR");
					break;
				default:
					break;
				}
				printf(" = %0x \n",+deviceStatus[k]);
			}
		}
		return 1;
	}
	return 0;
}

struct func
{
	// from original i2cdetect code, not important
	long value;
	const char* name;
};


static const struct func all_func[] = {
	// from original i2cdetect code, not important
	// from original i2cdetect code, not important
	{ .value = I2C_FUNC_I2C,
	  .name = "I2C" },
	{ .value = I2C_FUNC_SMBUS_QUICK,
	  .name = "SMBus Quick Command" },
	{ .value = I2C_FUNC_SMBUS_WRITE_BYTE,
	  .name = "SMBus Send Byte" },
	{ .value = I2C_FUNC_SMBUS_READ_BYTE,
	  .name = "SMBus Receive Byte" },
	{ .value = I2C_FUNC_SMBUS_WRITE_BYTE_DATA,
	  .name = "SMBus Write Byte" },
	{ .value = I2C_FUNC_SMBUS_READ_BYTE_DATA,
	  .name = "SMBus Read Byte" },
	{ .value = I2C_FUNC_SMBUS_WRITE_WORD_DATA,
	  .name = "SMBus Write Word" },
	{ .value = I2C_FUNC_SMBUS_READ_WORD_DATA,
	  .name = "SMBus Read Word" },
	{ .value = I2C_FUNC_SMBUS_PROC_CALL,
	  .name = "SMBus Process Call" },
	{ .value = I2C_FUNC_SMBUS_WRITE_BLOCK_DATA,
	  .name = "SMBus Block Write" },
	{ .value = I2C_FUNC_SMBUS_READ_BLOCK_DATA,
	  .name = "SMBus Block Read" },
	{ .value = I2C_FUNC_SMBUS_BLOCK_PROC_CALL,
	  .name = "SMBus Block Process Call" },
	{ .value = I2C_FUNC_SMBUS_PEC,
	  .name = "SMBus PEC" },
	{ .value = I2C_FUNC_SMBUS_WRITE_I2C_BLOCK,
	  .name = "I2C Block Write" },
	{ .value = I2C_FUNC_SMBUS_READ_I2C_BLOCK,
	  .name = "I2C Block Read" },
	{ .value = 0, .name = "" }
};

static void print_functionality(unsigned long funcs)
{
	// from original i2cdetect code, not important
	int i;
	for (i = 0; all_func[i].value; i++) {
		printf("%-32s %s\n", all_func[i].name,
		       (funcs & all_func[i].value) ? "yes" : "no");
	}
}

/*
 * Print the installed i2c busses. The format is those of Linux 2.4's
 * /proc/bus/i2c for historical compatibility reasons.
 
static void print_i2c_busses(void)
{
	struct i2c_adap *adapters;
	int count;

	adapters = gather_i2c_busses();
	if (adapters == NULL) {
		fprintf(stderr, "Error: Out of memory!\n");
		return;
	}

	for (count = 0; adapters[count].name; count++) {
		printf("i2c-%d\t%-10s\t%-32s\t%s\n",
			adapters[count].nr, adapters[count].funcs,
			adapters[count].name, adapters[count].algo);
	}

	free_adapters(adapters);
}
// from original i2cdetect code, not important

*/

const int i2cdetect()
{
	// can be called by main program, used to check devices (are all connected devices in place?). Soon to be further improved!
	char *end;
	int i2cbus, file, res;
	char filename[20];
	unsigned long funcs;
	int mode = MODE_AUTO;
	int first = 0x03, last = 0x77;
	int flags = 0;
	int yes = 1, version = 0, list = 0;

	if (I2C_BUS < 0) {
		//help();
		printf("set I2C_BUS not possible (<0)");
		exit(1);
	}

	file = open_i2c_dev(I2C_BUS, filename, sizeof(filename), 0);
	if (file < 0) {
		exit(1);
	}

	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		fprintf(stderr, "Error: Could not get the adapter "
			"functionality matrix: %s\n", strerror(errno));
		close(file);
		exit(1);
	}

	/*
	if (!yes) {
		char s[2];

		fprintf(stderr, "WARNING! This program can confuse your I2C "
			"bus, cause data loss and worse!\n");

		fprintf(stderr, "I will probe file %s%s.\n", filename,
			mode==MODE_QUICK?" using quick write commands":
			mode==MODE_READ?" using read byte commands":"");
		fprintf(stderr, "I will probe address range 0x%02x-0x%02x.\n",
			first, last);

		fprintf(stderr, "Continue? [Y/n] ");
		fflush(stderr);
		if (!fgets(s, 2, stdin)
		 || (s[0] != '\n' && s[0] != 'y' && s[0] != 'Y')) {
			fprintf(stderr, "Aborting on user request.\n");
			exit(0);
		}
	}
	// from original i2cdetect code, not important
	*/
	res = scan_i2c_bus(file, mode, first, last);
	// res will be either 1 or 0
	// 0 means all ok
	// 1 means there are missing devices
	close(file);

	return(res?1:0);
}
