#ifndef RATEBUFFER_H
#define RATEBUFFER_H
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <gpio_mapping.h>
#include <chrono>
#include <map>
#include <queue>
#include <list>

constexpr double MAX_AVG_RATE { 1000. };
constexpr unsigned long MAX_BURST_MULTIPLICITY { 10 };
using namespace std::literals;
constexpr std::chrono::microseconds MAX_BUFFER_TIME { 60 * 1s };

class RateBuffer : public QObject
{
	Q_OBJECT

public:
	struct BufferItem {
		std::queue<	std::chrono::time_point<std::chrono::steady_clock>, 
					std::list<std::chrono::time_point<std::chrono::steady_clock>> >
			eventbuffer { };
		unsigned long multiplicity_countdown { MAX_BURST_MULTIPLICITY };
		std::chrono::microseconds current_deadtime { 0 };
	};

	RateBuffer(QObject *parent = nullptr);
    ~RateBuffer() = default;
	void setRateLimit( double max_cps );
	[[nodiscard]] auto currentRateLimit() const -> double { return fRateLimit; }
	void clear() { buffermap.clear(); }
	
	[[nodiscard]] auto avgRate(unsigned int gpio) const -> double;
signals:
	void throttledSignal(unsigned int gpio);
	
public slots:
	void onSignal(unsigned int gpio);

private:
	double fRateLimit { MAX_AVG_RATE };
	std::chrono::microseconds fBufferTime { MAX_BUFFER_TIME };
	std::map<unsigned int, BufferItem> buffermap { };
	
};

#endif // RATEBUFFER_H
