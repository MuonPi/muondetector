#include <ratebuffer.h>
#include <iostream>

RateBuffer::RateBuffer(QObject *parent) : QObject(parent)
{
	
}

void RateBuffer::onSignal(unsigned int gpio) {
	auto now = std::chrono::steady_clock::now();
	if ( buffermap[gpio].eventbuffer.empty() ) {
		buffermap[gpio].eventbuffer.push(now);
		emit throttledSignal(gpio);
		return;
	}

	while ( 	!buffermap[gpio].eventbuffer.empty() 
			&& ( now - buffermap[gpio].eventbuffer.front() > fBufferTime) )
	{
		buffermap[gpio].eventbuffer.pop();
	}
	
	if ( !buffermap[gpio].eventbuffer.empty() ) {
		auto last_event_time = buffermap[gpio].eventbuffer.back();
		if ( now - last_event_time < MAX_DEADTIME ) {
//			std::cout << "now-last:"<<(now-last_event_time)/1us<<" dt="<<buffermap[gpio].current_deadtime.count()<<std::endl;
			if ( buffermap[gpio].current_deadtime < MAX_DEADTIME ) buffermap[gpio].current_deadtime++;
			if ( now - last_event_time < buffermap[gpio].current_deadtime ) {
				buffermap[gpio].eventbuffer.push(now);
				return;
			}
		} else {
			unsigned long deadtime = buffermap[gpio].current_deadtime.count();
			if ( deadtime>0 ) {
				deadtime--;
				buffermap[gpio].current_deadtime -= std::chrono::microseconds(1);
			}
		}
	}
	buffermap[gpio].eventbuffer.push(now);
	emit throttledSignal(gpio);
}

auto RateBuffer::avgRate(unsigned int gpio) const -> double
{
	auto it = buffermap.find(gpio);
	if (  it == buffermap.end() ) return 0.;
	if ( it->second.eventbuffer.empty() ) return 0.;
	auto end = std::chrono::steady_clock::now();
	auto start = end - fBufferTime;
	if ( start > it->second.eventbuffer.front() ) start = it->second.eventbuffer.front();
	double span = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	return ( it->second.eventbuffer.size() / span );
}

auto RateBuffer::currentDeadtime(unsigned int gpio) const -> std::chrono::microseconds
{
	auto it = buffermap.find(gpio);
	if (  it == buffermap.end() ) return std::chrono::microseconds(0);
	return it->second.current_deadtime;
}
