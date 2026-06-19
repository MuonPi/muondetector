#include "hardware/i2c/as7331.h"

#include <iomanip>
#include <iostream>

int main() {
    AS7331 as7331{"/dev/i2c-1", 0x74};
    as7331.reset();

    as7331.setGain(AS7331::GAIN::_2048x);

    std::cout << "Reading status after reset: " << std::endl;
    auto status = as7331.opStatus();
    std::cout << AS7331::toString(status) << std::endl;

    // std::cout << as7331.getConfig();

    // as7331.setStandby(false);
    // as7331.setBreakTime(0xff);

    // std::cout << as7331.getConfig();

    std::cout << "Starting measurement..." << std::endl;
    std::cout << "UVA UVB UVC\n";
    while (true) {
        as7331.startMeasurement();
        // usleep(1000);
        // status = as7331.opStatus();
        // std::cout << AS7331::toString(status) << std::endl;

        status = as7331.opStatus();
        while (status.nData == false) { // Wait for data latched
            usleep(1000);
            status = as7331.opStatus();
        }
        auto uva = as7331.readUVA();
        auto uvb = as7331.readUVB();
        auto uvc = as7331.readUVC();

        std::cout << uva.value() << " " << uvb.value() << " " << uvc.value() << std::endl;
        sleep(1);
    }
    std::cout << as7331.getConfig();
}