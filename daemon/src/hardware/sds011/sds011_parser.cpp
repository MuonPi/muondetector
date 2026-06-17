#include "sds011_parser.h"

#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

void Sds011Parser::reset() {
    state_ = State::SYNC1;
    ck_calc = 0;
}

void Sds011Parser::ck_update(std::uint8_t b) {
    ck_calc += b;
}

void Sds011Parser::feed(const std::uint8_t* data, std::size_t size,
                        std::function<void(Sds011Msg&&)> on_msg) {
    for (size_t i = 0; i < size; i++) {
        const uint8_t b = data[i];

        switch (state_) {

            // ---------------- SYNC 0xAA ----------------
            case State::SYNC1:
                if (b == 0xAA)
                    state_ = State::SYNC2;
                break;

            // ---------------- SYNC 0xC0 ----------------
            case State::SYNC2:
                if (b == 0xC0) {
                    state_ = State::DAT0;
                    msgType_ = Type::DATA;
                } else if (b == 0xC5) {
                    state_ = State::DAT0;
                    msgType_ = Type::ACK;
                } else if (b == 0xAA) {
                    state_ = State::SYNC2;
                } else {
                    state_ = State::SYNC1;
                }
                break;

            // ---------------- DATA ----------------
            case State::DAT0:
                data_[0] = b;
                ck_calc = 0;
                ck_update(b);
                state_ = State::DAT1;
                break;

            case State::DAT1:
                data_[1] = b;
                ck_update(b);
                state_ = State::DAT2;
                break;

            case State::DAT2:
                data_[2] = b;
                ck_update(b);
                state_ = State::DAT3;
                break;

            case State::DAT3:
                data_[3] = b;
                ck_update(b);
                state_ = State::DAT4;
                break;

            case State::DAT4:
                data_[4] = b;
                ck_update(b);
                state_ = State::DAT5;
                break;

            case State::DAT5:
                data_[5] = b;
                ck_update(b);
                state_ = State::CHKSM;
                break;

            case State::CHKSM:
                if (ck_calc == b) {
                    state_ = State::TAIL;
                } else {
                    reset();
                }
                break;

            // ---------------- TAIL ----------------
            case State::TAIL:
                if (b == 0xAB) {
                    if (msgType_ == Type::DATA) {
                        on_msg(build_data_msg());
                    }
                    if (msgType_ == Type::ACK) {
                        on_msg(build_ack_msg());
                    }
                }
                reset();
                break;
        }
    }
}

auto Sds011Parser::build_data_msg() -> Sds011Event {

    std::uint16_t pm2dot5 = static_cast<std::uint16_t>(data_.at(0));
    pm2dot5 |= (static_cast<std::uint16_t>(data_.at(1)) << 8);

    std::uint16_t pm10dot0 = static_cast<std::uint16_t>(data_.at(2));
    pm10dot0 |= (static_cast<std::uint16_t>(data_.at(3)) << 8);

    std::uint16_t id = static_cast<std::uint16_t>(data_.at(4));
    id |= (static_cast<std::uint16_t>(data_.at(5)) << 8);

    return Sds011Event{.id = id, .pm2dot5 = pm2dot5, .pm10dot0 = pm10dot0};
}

auto Sds011Parser::build_ack_msg() -> Sds011StatusEvent {
    Sds011StatusEvent msg;
    msg.ackType = data_.at(0);
    switch (data_.at(0)) {
        case 2U:
            msg.queryOnlyMode = (data_.at(2) == 1);
            break;
        case 5U:
            msg.id = static_cast<std::uint16_t>(data_.at(4)) |
                     (static_cast<std::uint16_t>(data_.at(5)) << 8);
            break;
        case 6U:
            msg.sleep = (data_.at(2) == 0);
            break;
        case 7U:
            msg.firmwareDate = parseFirmware();
            break;
        case 8U:
            msg.modeByte = data_.at(2);
            break;
        default:
            break;
    }
    return msg;
}

auto Sds011Parser::parseFirmware() -> std::string {
    std::stringstream sstr;
    sstr << "20" << std::setw(2) << std::setfill('0')
         << std::to_string(static_cast<unsigned>(data_.at(1)));
    sstr << "-" << std::setw(2) << std::setfill('0')
         << std::to_string(static_cast<unsigned>(data_.at(2)));
    sstr << "-" << std::setw(2) << std::setfill('0')
         << std::to_string(static_cast<unsigned>(data_.at(3)));
    return sstr.str();
}