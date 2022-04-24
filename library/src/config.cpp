#include "config.h"
#include <cstring>

namespace MuonPi::Version {
auto Version::string() const -> std::string
{
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch) + ((additional.empty()) ? ("") : ("-" + additional)) + ((hash.empty()) ? ("") : ("-" + hash));
}

}
