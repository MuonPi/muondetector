#ifndef _CONFIG_H_
#define _CONFIG_H_


#define MUONPI_BUILD_TIME				__DATE__
#define MUONPI_VERSION					"1.2.1"
#define MUONPI_DEFAULT_HW_VERSION		2
#define MUONPI_DEFAULT_CONFIG_FILE		"/etc/muondetector/muondetector.conf"

#define MUONPI_MQTT_SERVER				"116.202.96.181:1883"
#define MUONPI_MQTT_TIMEOUT_MS			30000
#define MUONPI_MQTT_QOS					1

#define MUONPI_LOG_INTERVAL_MINUTES		1

#define MUONPI_UPLOAD_REMINDER_MINUTES	5
#define MUONPI_UPLOAD_TIMEOUT_MS		600000UL
#define MUONPI_UPLOAD_URL				"balu.physik.uni-giessen.de:/cosmicshower"
#define MUONPI_UPLOAD_PORT				35221

#define OLED_UPDATE_PERIOD 				2000

#define ADC_SAMPLEBUFFER_SIZE 			50
#define ADC_PRETRIGGER 					10
#define TRACE_SAMPLING_INTERVAL 		5  // free running adc sampling interval in ms
#define PARAMETER_MONITOR_INTERVAL		5000

#define RATE_SCAN_ITERATIONS			10
#define RATE_SCAN_TIME_INTERVAL_MS		400

#endif // _CONFIG_H_
