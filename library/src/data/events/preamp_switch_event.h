#ifndef PREAMP_SWITCH_EVENT_H
#define PREAMP_SWITCH_EVENT_H

struct PreampSwtichEvent {
    std::uint8_t channel{0};
    bool state{false};
};

#endif // PREAMP_SWITCH_EVENT_H