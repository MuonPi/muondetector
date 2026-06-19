#include "hardware/i2c/as7343.h"

#include <chrono>
#include <iomanip>
#include <iostream>

int main() {
    AS7343 as7343{"/dev/i2c-1", 0x39};

    std::cout << "Starting measurement..." << std::endl;
    AS7343::Config config{};
    config.autoSmuxMode = AS7343::AUTO_SMUX_MODE::_18Ch;
    config.fifoMap = 0x7e; // Enable CH0..CH5. VIS and FD are part of the auto-SMUX channel data.
    config.gain = AS7343::GAIN::_16x;
    config.atime = 0x09;
    config.astep = 3596; // ~100 ms per 6-channel SMUX step: (9 + 1) * (3596 + 1) * 2.78 us
    config.ledAct = true;
    config.ledDrive = 0b0100;
    as7343.reset();
    usleep(10000);
    as7343.init(config);

    auto spectrum = as7343.readSpectrum();
    // std::cout << as7343.getConfig();
}
