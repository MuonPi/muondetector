#ifndef PIGPIODHANDLER_H
#define PIGPIODHANDLER_H

#include <QObject>
#include <QVector>
#include <QDateTime>
#include <QElapsedTimer>
#include <QTimer>
#include <memory>

#include <memory>
#include <thread>
#include <mutex>
#include "utility/gpio_mapping.h"
#include <gpio_pin_definitions.h>
#include <muondetector_structs.h>

#include <gpiod.h>

class PigpiodHandler : public QObject {
    Q_OBJECT

public:
	enum PinBias : std::uint8_t {
		BiasDisabled = 0x00,
		PullDown = 0x01,
		PullUp = 0x02,
		ActiveLow = 0x04,
		OpenDrain = 0x08,
		OpenSource = 0x10
	};
	enum class EventEdge {
		RisingEdge, FallingEdge, BothEdges
	};
	
	static constexpr unsigned int UNDEFINED_GPIO { 256 };
	
	explicit PigpiodHandler(std::vector<unsigned int> gpioPins, QObject *parent = nullptr);
	~PigpiodHandler();
	
	QDateTime startOfProgram, lastSamplingTime; // the exact time when the program starts (Utc)
	QElapsedTimer elapsedEventTimer;

	double clockMeasurementSlope=0.;
	double clockMeasurementOffset=0.;
	uint64_t gpioTickOverflowCounter=0;
	quint64 lastTimeMeasurementTick=0;
	
	bool isInhibited() const { return inhibit; }
	
signals:
	void event(unsigned int gpio_pin, EventTime timestamp);

public slots:
	void start();
	void stop();
    bool initialised();
    bool setPinInput(unsigned int gpio);
    bool setPinOutput(unsigned int gpio, bool initState);
	
	bool setPinBias(unsigned int gpio, std::uint8_t bias_flags);
    bool setPinState(unsigned int gpio, bool state);
    bool registerInterrupt(unsigned int gpio, EventEdge edge);
    bool unRegisterInterrupt(unsigned int gpio);
	void setInhibited(bool inh=true) { inhibit=inh; }

private:
	bool isInitialised { false };

	QTimer gpioClockTimeMeasurementTimer;

	void measureGpioClockTime();
	void reloadInterruptSettings();
	[[gnu::hot]] void eventHandler( struct gpiod_line *line );
	[[gnu::hot]] void processEvent( unsigned int gpio, std::shared_ptr<gpiod_line_event> line_event );
	
	bool inhibit  { false };
	int verbose { 0 };
	gpiod_chip* fChip { nullptr };
	std::map<unsigned int, gpiod_line*> fInterruptLineMap { };
	std::map<unsigned int, gpiod_line*> fLineMap { };
	bool fThreadRunning { false };
	std::map<unsigned int, std::unique_ptr<std::thread>> fThreads { };
	std::mutex fMutex;
};


#endif // PIGPIODHANDLER_H
