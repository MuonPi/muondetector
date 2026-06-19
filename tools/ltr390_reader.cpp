#include "hardware/i2c/ltr390uv01.h"

#include <iomanip>
#include <iostream>

int main() {
    LTR390UV01 ltr390{"/dev/i2c-1", 0x53};
    auto devID = ltr390.id();
    auto status = ltr390.mainStatus();
    while (status.dataStatus == false) {
        usleep(50000);
        status = ltr390.mainStatus();
    }
    std::cout << "powerOnStatus: " << (status.powerOnStatus ? "true" : "false") << "\n";
    std::cout << "interruptStatus: " << (status.interruptStatus ? "true" : "false") << "\n";
    std::cout << "dataStatus: " << (status.dataStatus ? "true" : "false") << "\n";
    std::cout << std::flush;
    std::cout << std::setw(2) << std::setfill('0') << std::hex << ltr390.read() << std::endl;
}