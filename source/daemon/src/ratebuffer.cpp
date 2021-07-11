#include <ratebuffer.h>


RateBuffer::RateBuffer(QObject *parent) : QObject(parent)
{
	
}

void RateBuffer::onSignal(unsigned int gpio) {
	auto now = std::chrono::steady_clock::now();
	buffermap[gpio].eventbuffer.push(now);
	while ( 	buffermap[gpio].eventbuffer.size() > 1 
			&& ( now - buffermap[gpio].eventbuffer.front() > fBufferTime) )
	{
		buffermap[gpio].eventbuffer.pop();
	}
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
