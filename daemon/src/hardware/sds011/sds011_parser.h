#ifndef SDS011_PARSER_H
#define SDS011_PARSER_H

#include "data/events/sds011_event.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <variant>

using Sds011Msg = std::variant<Sds011Event, Sds011StatusEvent>;

class Sds011Parser {
  public:
    void feed(const std::uint8_t* data, std::size_t size, std::function<void(Sds011Msg&&)> on_msg);

  private:
    auto build_data_msg() -> Sds011Event;
    auto build_ack_msg() -> Sds011StatusEvent;
    enum class State { SYNC1, SYNC2, DAT0, DAT1, DAT2, DAT3, DAT4, DAT5, CHKSM, TAIL };
    enum class Type { DATA, ACK };
    void reset();
    void ck_update(std::uint8_t b);
    auto parseFirmware() -> std::string;
    State state_ = State::SYNC1;
    Type msgType_ = Type::DATA;
    std::uint8_t ck_calc{0};
    std::array<std::uint8_t, 6> data_{};
};

#endif // SDS011_PARSER_H