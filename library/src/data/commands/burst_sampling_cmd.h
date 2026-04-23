#ifndef BURST_SAMPLING_H
#define BURST_SAMPLING_H

struct StartBurstSampling {
    std::size_t frequencyHz;
    std::size_t samples;
};

struct StopBurstSampling {};

#endif // BURST_SAMPLING_H