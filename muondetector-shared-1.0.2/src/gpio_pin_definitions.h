#ifndef GPIO_PIN_DEFINITIONS_H
#define GPIO_PIN_DEFINITIONS_H
#include <muondetector_shared_global.h>

// define pins on the raspberry pi, UBIAS_EN is the power on/off pin for bias voltage
// PREAMP_1/2 enables the DC voltage to power the preamp through the signal cable
// Attention: the GPIO pin definitions are referring to wiringPi standard except the EVT input pins
// which are in pigpio standard
#define UBIAS_EN 7  // pigpio 4
#define PREAMP_1 28  // pigpio 20
#define PREAMP_2 29  // pigpio 21
#define EVT_AND 5
#define EVT_XOR 6
#define GAIN_HL 0	// pigpio 17
#define ADC_READY 23
#define TIMEPULSE 18


#endif // GPIO_PIN_DEFINITIONS_H
