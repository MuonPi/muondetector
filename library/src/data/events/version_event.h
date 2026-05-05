#ifndef VERSION_EVENT_H
#define VERSION_EVENT_H

#include "config.h"

struct VersionEvent {
    MuonPi::Version::Version hw_ver;
    MuonPi::Version::Version sw_ver;
};

#endif // VERSION_EVENT_H