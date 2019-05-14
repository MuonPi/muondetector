#ifndef GPIO_PIN_DEFINITIONS_H
#define GPIO_PIN_DEFINITIONS_H
#include <muondetector_shared_global.h>
#include <QMap>
#include <QString>
#include <map>

// define the pins which are used to interface the raspberry pi
// UBIAS_EN is the power on/off pin for bias voltage
// PREAMP_1/2 enables the DC voltage to power the preamp through the signal cable
// EVT_AND, EVT_XOR are the event inputs from AND and XOR gates
// Note: The pin definitions are enum constants and have nothing to do with the actual pin numbers
// of the RPi GPIO header. To be independent of the specific hardware implementation,
// the pin numbers for these signals are defined in pin_mapping.h on the daemon side

enum GPIO_PIN {		UBIAS_EN, 
					PREAMP_1, PREAMP_2, 
					EVT_AND, EVT_XOR, 
					GAIN_HL, ADC_READY, 
					TIMEPULSE, 
					TIME_MEAS_OUT, 
					STATUS1, STATUS2, STATUS3, 
					PREAMP_FAULT, EXT_TRIGGER
				};

enum SIGNAL_DIRECTION { DIR_UNDEFINED, DIR_IN, DIR_OUT, DIR_IO };

struct GpioSignalDescriptor {
	QString name;
	SIGNAL_DIRECTION direction;
};

static const QMap<GPIO_PIN, GpioSignalDescriptor> GPIO_SIGNAL_MAP =
	{	{ UBIAS_EN,			{ "UBIAS_EN", DIR_OUT } },
		{ PREAMP_1,			{ "PREAMP_1", DIR_OUT } },
		{ PREAMP_2,			{ "PREAMP_2", DIR_OUT } },
		{ EVT_AND,			{ "EVT_AND", DIR_IN }  },
		{ EVT_XOR,			{ "EVT_XOR", DIR_IN }  },
		{ GAIN_HL,			{ "GAIN_HL", DIR_OUT } },
		{ ADC_READY,		{ "ADC_READY", DIR_IN } },
		{ TIMEPULSE,		{ "TIMEPULSE", DIR_IN }},
		{ TIME_MEAS_OUT,{ "TIME_MEAS_OUT", DIR_IN } },
		{ STATUS1,			{ "STATUS1", DIR_OUT } },
		{ STATUS2,			{ "STATUS2", DIR_OUT } },
		{ STATUS3,			{ "STATUS3", DIR_OUT } },
		{ PREAMP_FAULT,	{ "PREAMP_FAULT", DIR_IN } },
		{ EXT_TRIGGER,	{ "EXT_TRIGGER", DIR_IN } }
	};


static const QMap<GPIO_PIN, QString> GPIO_PIN_NAMES = 
	{	{ UBIAS_EN,		"UBIAS_EN" },
		{ PREAMP_1,		"PREAMP_1" },
		{ PREAMP_2,		"PREAMP_2" },
		{ EVT_AND,		"EVT_AND"  },
		{ EVT_XOR,		"EVT_XOR"  },
		{ GAIN_HL,		"GAIN_HL"  },
		{ ADC_READY,	"ADC_READY"},
		{ TIMEPULSE,	"TIMEPULSE"},
		{ TIME_MEAS_OUT,"TIME_MEAS_OUT" },
		{ STATUS1,		"STATUS1"  },
		{ STATUS2,		"STATUS2"  },
		{ STATUS3,		"STATUS3"  },
		{ PREAMP_FAULT,	"PREAMP_FAULT" },
		{ EXT_TRIGGER,	"EXT_TRIGGER"  }
	};


#endif // GPIO_PIN_DEFINITIONS_H
