#include "config.h"

namespace MuonPi::Version {
auto Version::string() const -> std::string
{
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

}
